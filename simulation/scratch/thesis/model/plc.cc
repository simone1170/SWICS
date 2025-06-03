#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include "../helper/string-helper.h"
#include "../helper/command-line-helper.h"
#include "../helper/hex-helper.h"
#include "plc.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE ("PLC");

// Discrete Input address assignment
// Timeout (Sensors) | Timeout (Actuators) | Custom (Starting with sensor data)

// Input Registers address assignment
// Dependent on call order of SetInputRegisterValue

PLC::PLC(std::string identifier, std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators) : IndustrialDevice(identifier, true), m_sensors(sensors), m_actuators(actuators) {
    CMD_LOG_FUNCTION(this << identifier << Cmd::Preprocess(sensors) << Cmd::Preprocess(actuators));

    SetRecvCallback(MakeCallback(&PLC::ReceiveModbusData, this));

    // Initialize sensor timeout values to true, need to get data in order to set them to false
    for (const auto& [key, value] : sensors) {
        SetDiscreteInputValue(sensors[key]->GetIdentifier() + "timeout", 0, true);
    }

    // Initialize actuator timeout values to true, need to get data in order to set them to false
    for (const auto& [key, value] : actuators) {
        SetDiscreteInputValue(actuators[key]->GetIdentifier() + "timeout", 0, true);
    }

    // Value required to know where the timeout part ends
    m_customDiscreteInputStart = m_discreteInputs.size();
}

PLC::~PLC() {
    CMD_LOG_FUNCTION(this);
}

void PLC::ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context) {
    CMD_LOG_FUNCTION(this << from << modbusHeader << functionCode << data << context);

    if (context.GetName() != "") {
        // Reset timeout discrete inputs (case: sensor response)
        for (auto sensor : m_sensors) {
            if (from.GetIpv4() == sensor.second->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()) {
                SetDiscreteInputValue(sensor.first + "timeout", 0, false);
                break;
            }
        }

        // Reset timeout discrete inputs (case: actuator response)
        for (auto actuator : m_actuators) {
            if (from.GetIpv4() == actuator.second->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()) {
                SetDiscreteInputValue(actuator.first + "timeout", 0, false);
                break;
            }
        }

        // Handle response
        if (functionCode == 0x02) {
            try {
                for (uint8_t i = 0; i < context.GetCount(); i++) {
                    if (context.GetTime() >= m_currentSensorDataRequestTime[context.GetName()][context.GetAddress()+i]) {
                        // Update data only if response is newer than current value (beginning at data[1], since data[0] is the byteCount)
                        bool value = (data[(i+8)/8] >> (i%8)) & 0x01;

                        CMD_LOG_INFO("PLC storing data for " << context.GetName() << " at address " << context.GetAddress()+i << ": " << value);
                        SetDiscreteInputValue(context.GetName(), context.GetAddress()+i, value);

                        // Update sensorDataTime
                        m_currentSensorDataRequestTime[context.GetName()][context.GetAddress()+i] = context.GetTime();
                    }
                }
            } catch (...) {
                CMD_LOG_ERROR(m_identifier << ": Error handling received sensor data (byte count mismatch?) data: " << hex::toHex(data));
            }
        } else if (functionCode == 0x04) {
            try {
                for (uint8_t i = 0; i < data[0]/2; i++) {
                    if (context.GetTime() >= m_currentSensorDataRequestTime[context.GetName()][context.GetAddress()+i]) {
                        // Update data only if response is newer than current value
                        CMD_LOG_INFO("PLC storing data for " << context.GetName() << " at address " << context.GetAddress()+i << ": " << hex::toHex(std::vector<uint8_t>{data[2*i+1], data[2*i+2]}));
                        SetInputRegisterValue(context.GetName(), context.GetAddress()+i, ((uint16_t)data[2*i+2]) | data[2*i+1] << 8);

                        // Update sensorDataTime
                        m_currentSensorDataRequestTime[context.GetName()][context.GetAddress()+i] = context.GetTime();
                    } else {
                        CMD_LOG_INFO(m_identifier << " has newer data for " << context.GetName() << " (" << context.GetTime() << " < " << m_currentSensorDataRequestTime[context.GetName()][context.GetAddress()+i] << ")");
                    }
                }
            } catch (...) {
                CMD_LOG_ERROR(m_identifier << ": Error handling received sensor data (byte count mismatch?) data: " << hex::toHex(data));
            }
        } else if (functionCode == 0x05 || functionCode == 0x0F || functionCode == 0x06) {
            // TODO: validate complete list of responses from actuators
            // Doing nothing with response from actuator.
        } else if (functionCode > 0x80) {
            CMD_LOG_ERROR(m_identifier << ": PLC received error response for request " << context.GetName() << " at address " << context.GetAddress() << ": function code " << hex::toHex(functionCode) << " with data " << hex::toHex(data));
        } else {
            CMD_LOG_ERROR(m_identifier << ": Function code " << hex::toHex(functionCode) << " not implemented. Cannot parse response (context: " << context.GetName() <<").");
        }
    } else {
        // Handle request
        if (data.size() < 2) {
            CMD_LOG_ERROR(m_identifier << ": Got request with function code " << hex::toHex(functionCode) << " but data was shorter than two bytes.");
            functionCode = functionCode + 0x80;
            data = {0x02};
        } else {
            // Extract address from data
            uint16_t address = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);

            if (functionCode == 0x01) {
                // Read Coils
                if (data.size() != 4) {
                    CMD_LOG_ERROR(m_identifier << ": Got request to read coils (0x01) but message was " << data.size() << " bytes (expected 4 bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else {
                    uint16_t coilCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                    if (coilCount > 0x7D0 || coilCount < 1) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x01) but count is invalid (" << coilCount << " not between 0x01 and 0x7D0).");
                        functionCode = functionCode + 0x80;
                        data = {0x03};
                    } else if (address + coilCount > m_coils.size()) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x01) but count is invalid (" << coilCount << " coils requested at address " << address << " which is greater than last addressable coil).");
                        functionCode = functionCode + 0x80;
                        data = {0x02};
                    } else {
                        try {
                            uint8_t byteCount = 1 + (coilCount-1) / 8;
                            data = {byteCount};
                            uint8_t currentByte = 0;
                            for (uint8_t i = 0; i < coilCount; i++) {
                                uint8_t offset = i % 8;
                                if (m_coils[address+i]) {
                                    currentByte |= (1 << offset);
                                } else {
                                    currentByte &= ~(1 << offset);
                                }
                                if ((i+1) % 8 == 0) {
                                    data.push_back(currentByte);
                                    currentByte = 0x00;
                                }
                            }
                            // Append last (non-full) byte
                            if (data.size() == byteCount) {
                                data.push_back(currentByte);
                            }
                        } catch (...) {
                            CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x01) but failed to convert boolean values to bytes.");
                            functionCode = functionCode + 0x80;
                            data = {0x04};
                        }
                    }
                }
            } else if (functionCode == 0x02) {
                // Read Discrete inputs
                if (data.size() != 4) {
                    CMD_LOG_ERROR(m_identifier << ": Got request to read discrete inputs (0x02) but message was " << data.size() << " bytes (expected 4 bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else {
                    uint16_t discreteInputCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                    if (discreteInputCount > 0x7D0 || discreteInputCount < 1) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read discrete inputs (0x02) but count is invalid (" << discreteInputCount << " not between 0x01 and 0x7D0).");
                        functionCode = functionCode + 0x80;
                        data = {0x03};
                    } else if (address + discreteInputCount > m_discreteInputs.size()) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read discrete inputs (0x02) but count is invalid (" << discreteInputCount << " discrete inputs requested at address " << address << " which is greater than last addressable coil).");
                        functionCode = functionCode + 0x80;
                        data = {0x02};
                    } else {
                        try {
                            uint8_t byteCount = 1 + (discreteInputCount-1) / 8;
                            data = {byteCount};
                            uint8_t currentByte = 0;
                            for (uint8_t i = 0; i < discreteInputCount; i++) {
                                uint8_t offset = i % 8;
                                if (m_discreteInputs[address+i]) {
                                    currentByte |= (1 << offset);
                                } else {
                                    currentByte &= ~(1 << offset);
                                }
                                if ((i+1) % 8 == 0) {
                                    data.push_back(currentByte);
                                    currentByte = 0x00;
                                }
                            }
                            // Append last (non-full) byte
                            if (data.size() == byteCount) {
                                data.push_back(currentByte);
                            }
                        } catch (...) {
                            CMD_LOG_ERROR(m_identifier << ": Got Request to read discrete inputs (0x02) but failed to convert boolean values to bytes.");
                            functionCode = functionCode + 0x80;
                            data = {0x04};
                        }
                    }
                }
            } else if (functionCode == 0x03) {
                // Read holding registers
                if (data.size() != 4) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but message was " << data.size() << " bytes (expected 4 bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else {
                    uint16_t address = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);
                    uint16_t registerCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);

                    if (registerCount > 0x125 || registerCount < 1) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but register count is invalid (" << registerCount << " not between 0x01 and 0x125).");
                        functionCode = functionCode + 0x80;
                        data = {0x03};
                    } else if (address + registerCount > m_holdingRegisters.size()) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but register count is invalid (" << registerCount << " registers requested at address " << address << " which is greater than last addressable register).");
                        functionCode = functionCode + 0x80;
                        data = {0x02};
                    } else {
                        try {
                            uint8_t byteCount = 2 * registerCount;
                            data = {byteCount};
                            for (uint8_t i = 0; i < registerCount; i++) {
                                uint16_t value = m_holdingRegisters[address+i];
                                std::vector<uint8_t> result = {(uint8_t)((value >> 8) & 0xff), (uint8_t)((value >> 0) & 0xff)};
                                data.insert(data.end(), result.begin(), result.end());
                            }
                        } catch (...) {
                            CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but failed to build response data.");
                            functionCode = functionCode + 0x80;
                            data = {0x04};
                        }
                    }
                }
            } else if (functionCode == 0x04) {
                // Read input registers
                if (data.size() != 4) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but message was " << data.size() << " bytes (expected 4 bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else {
                    uint16_t address = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);
                    uint16_t registerCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);

                    if (registerCount > 0x125 || registerCount < 1) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but register count is invalid (" << registerCount << " not between 0x01 and 0x125).");
                        functionCode = functionCode + 0x80;
                        data = {0x03};
                    } else if (address + registerCount > m_inputRegisters.size()) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but register count is invalid (" << registerCount << " registers requested at address " << address << " which is greater than last addressable register).");
                        functionCode = functionCode + 0x80;
                        data = {0x02};
                    } else {
                        try {
                            uint8_t byteCount = 2 * registerCount;
                            data = {byteCount};
                            for (uint8_t i = 0; i < registerCount; i++) {
                                uint16_t value = m_inputRegisters[address+i];
                                std::vector<uint8_t> result = {(uint8_t)((value >> 8) & 0xff), (uint8_t)((value >> 0) & 0xff)};
                                data.insert(data.end(), result.begin(), result.end());
                            }
                        } catch (...) {
                            CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but failed to build response data.");
                            functionCode = functionCode + 0x80;
                            data = {0x04};
                        }
                    }
                }
            } else if (functionCode == 0x05) {
                // Write single coil
                if (address >= m_coils.size()) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write coil (0x05) but address was too large (" << address << " > " << m_coils.size() << ").");
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else {
                    if (data[2] == 0xFF && data[3] == 0x00) {
                        m_coils[address] = true;
                    } else if (data[2] == 0x00 && data[3] == 0x00) {
                        m_coils[address] = false;
                    } else {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to write coil (0x05) but value is not boolean (got " << hex::toHex({data[2], data[3]}) << ", expected 0x00 0x00 or 0xFF 0x00).");
                        functionCode = functionCode + 0x80;
                        data = {0x03};
                    }
                }
            } else if (functionCode == 0x06) {
                // Write single holding register
                if (address >= m_holdingRegisters.size()) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write register (0x06) but address was too large (" << address << " > " << m_holdingRegisters.size() << ").");
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else if (data.size() != 4) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write register (0x06) but message was " << data.size() << " bytes (expected 4 bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else {
                    uint16_t value = (static_cast<uint16_t>(data[2] << 8) | static_cast<uint16_t>(data[3])); // First two bytes following address
                    m_holdingRegisters[address] = value;
                }
            } else if (functionCode == 0x0F) {
                // Write multiple coils
                uint16_t coilCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                uint8_t byteCount = data[4];
                uint8_t calculatedByteCount = 1 + (coilCount-1) / 8;

                if (coilCount > 2000 || coilCount < 1 || calculatedByteCount != byteCount) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write coils (0x0F) but coil count is invalid (" << ((calculatedByteCount != byteCount) ? ("expected " + std::to_string(calculatedByteCount) + " bytes of values but got " + std::to_string(byteCount)) : (std::to_string(coilCount) + " not between 0x01 and 0xC8")) << ").");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else if (address + coilCount > m_coils.size()) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write coils (0x0F) but address was too large (" << (address+coilCount) << " > " << m_coils.size() << ").");
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else if (data.size() < (uint8_t) (byteCount + 5)) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write coils (0x0F) but message was " << data.size() << " bytes (expected " << (byteCount+5) << " bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x04};
                } else {
                    try {
                        std::vector<uint8_t> values(data.size()-5);
                        values.assign(data.begin()+5, data.end());
                        for (uint8_t i = 0; i < values.size(); i++) {
                            for (uint8_t j = 0; j < 8; j++) {
                                uint8_t offset = i*8 + j;
                                if (offset >= coilCount) {
                                    break;
                                }
                                uint8_t coilValue = (values[i] >> j) & 1;
                                m_coils[address+offset] = coilValue;
                            }
                        }
                        // Set response to only address and quantity
                        data.erase(data.begin()+4, data.end());
                    } catch (...) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to write coils (0x0F) but failed store boolean values.");
                        functionCode = functionCode + 0x80;
                        data = {0x04};
                    }
                }
            } else if (functionCode == 0x10) {
                // Write multiple holding registers
                uint16_t registerCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                uint8_t byteCount = data[4];
                if (registerCount > 123 || registerCount < 1 || byteCount != registerCount * 2) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write registers (0x10) but " << (((uint16_t) byteCount != registerCount * 2) ? ("got a mismatch between register count and the amount of values (" + std::to_string(registerCount) + " " + std::to_string(byteCount) + ")") : ("register count is invalid (" + std::to_string(registerCount) + " not between 0x01 and 0x7B)")) << ".");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else if (data.size() < (uint8_t) (byteCount + 5)) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write registers (0x10) but message was " << data.size() << " bytes (expected " << (byteCount+5) << " bytes).");
                    functionCode = functionCode + 0x80;
                    data = {0x04};
                } else if (address + registerCount > m_holdingRegisters.size()) {
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else {
                    try {
                        for (uint8_t i = 0; i < registerCount; i++) {
                            uint16_t value = (static_cast<uint16_t>(data[5+2*i] << 8) | static_cast<uint16_t>(data[5+5*i+1])); // Copy two value bytes for each register (iterating using i)
                            m_holdingRegisters[address] = value;
                        }
                        // Set response to only address and quantity
                        data.erase(data.begin()+4, data.end());
                    } catch(...) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to write registers (0x10) but failed to store values.");
                        functionCode = functionCode + 0x80;
                        data = {0x04};
                    }
                }
            } else if (functionCode == 0x2B) {
                if (data.size() >= 1 && data[0] == 0x0E) {
                    // Read device identification
                    if (data.size() == 3) {
                        if (data[1] >= 1 && data[1] <= 4) {
                            // Basic identification (same as regular and extended)
                            const std::string vendorName = "5G Testbed Manufacturing";
                            std::string productCode = m_identifier;
                            const std::string majorMinorRevision = "1.0.1";

                            uint8_t objectId = data[2];
                            if (objectId > 3) {
                                // Set objectId to 0 if objectId is unknown according to Modbus application protocol specification
                                objectId = 0;
                            }

                            data.erase(data.begin()+1, data.end()); // Only keep MEI type
                            data.push_back(0x01); // Only provides basic information (replace read device ID code)
                            data.push_back(0x01); // Only provides basic information (conformity level)
                            std::vector<uint8_t> list;

                            const uint16_t maxFrameSize = 252 - data.size();
                            uint8_t moreFollows = 0x00;
                            uint8_t nextObjectId = 0x00;

                            if (objectId < 1) {
                                list.push_back(0x00);
                                list.push_back(vendorName.size());
                                list.insert(list.end(), vendorName.begin(), vendorName.end());
                            }
                            if (objectId < 2 && list.size() + productCode.size() + 2 < maxFrameSize) {
                                list.push_back(0x00);
                                list.push_back(productCode.size());
                                list.insert(list.end(), productCode.begin(), productCode.end());
                            } else {
                                // Set response
                                nextObjectId = 0x01;
                                moreFollows = 0xFF;
                            }
                            if (objectId < 3 && nextObjectId == 0x00 && list.size() + majorMinorRevision.size() + 2 < maxFrameSize) {
                                list.push_back(0x00);
                                list.push_back(majorMinorRevision.size());
                                list.insert(list.end(), majorMinorRevision.begin(), majorMinorRevision.end());
                            } else {
                                // Set response
                                nextObjectId = 0x02;
                                moreFollows = 0xFF;
                            }

                            // Build response
                            data.push_back(moreFollows);
                            data.push_back(nextObjectId);
                            data.push_back(nextObjectId-objectId); // Calculate number of included objects
                            data.insert(data.end(), list.begin(), list.end());
                        } else {
                            CMD_LOG_ERROR(m_identifier << ": Got Request for device identification (0x2B 0x0E) but invalid identification field was requested.");
                            functionCode = functionCode + 0x80;
                            data = {0x03};
                        }
                    } else {
                        CMD_LOG_ERROR(m_identifier << ": Got Request for device identification (0x2B 0x0E) but got " << data.size() << " bytes (expected 3 bytes).");
                        functionCode = functionCode + 0x80;
                        data = {0x02};
                    }
                } else {
                    CMD_LOG_ERROR(m_identifier << ": Got Request for function code 0x2B but not to read device identification (0x0E) (got data " << hex::toHex(data) << ").");
                    functionCode = functionCode + 0x80;
                    data = {0x01};
                }
            } else {
                // Function code not supported
                CMD_LOG_ERROR(m_identifier << ": Got Request for unsupported function code " << hex::toHex(functionCode) << ".");
                functionCode = functionCode + 0x80;
                data = {0x01};
            }
        }

        SendModbusResponse(from, modbusHeader, functionCode, data);
    }
}

// Define cycle
void PLC::Cycle() {
    CMD_LOG_FUNCTION(this);

    // Update sensor data cache (read sensor data)
    m_inputRegistersCache = m_inputRegisters;
    m_discreteInputsCache = m_discreteInputs;

    RunControlLogic();

    // Run control actuator method
    ControlActuators();

    // Request Sensor Data
    RequestSensorData();

    // Rerun method
    m_cycleEvent = Simulator::Schedule(MilliSeconds(m_cycleTime), &PLC::Cycle, this);
}

std::string PLC::GetIdentifier() {
    CMD_LOG_FUNCTION(this);

    return m_identifier;
}

void PLC::StartApplication() {
    CMD_LOG_FUNCTION(this);
    IndustrialDevice::StartApplication();

    Cycle();
}

void PLC::StopApplication() {
    CMD_LOG_FUNCTION(this);

    if (m_cycleEvent.IsRunning()) {
        Simulator::Cancel(m_cycleEvent);
    }
    IndustrialDevice::StopApplication();
}

/// -----------------------------
/// Timeout handling
/// -----------------------------

void PLC::HandleTimeout(std::pair<uint16_t,InetSocketAddress> contextIndex) {
    CMD_LOG_FUNCTION(this << contextIndex.first << contextIndex.second);

    IndustrialDevice::HandleTimeout(contextIndex);
    for (auto sensor : m_sensors) {
        if (contextIndex.second.GetIpv4() == sensor.second->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()) {
            // Indicate timeout in discrete inputs
            SetDiscreteInputValue(sensor.second->GetIdentifier() + "timeout", 0, true);
            return;
        }
    }

    for (auto actuator : m_actuators) {
        if (contextIndex.second.GetIpv4() == actuator.second->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal()) {
            // Indicate timeout in discrete inputs
            SetDiscreteInputValue(actuator.second->GetIdentifier() + "timeout", 0, true);
            return;
        }
    }
}

bool PLC::GetSensorTimedOut(std::string sensorName) {
    CMD_LOG_FUNCTION(this << sensorName);

    try {
        return GetDiscreteInputValue(sensorName + "timeout", 0);
    } catch(...) {
        // If sensor is not found, it would be unsafe to indicate no timeout
        CMD_LOG_ERROR("Timeout state for unknown sensor " << sensorName << "requested.");
        return true;
    }
}

/// -----------------------------
/// Discrete inputs
/// -----------------------------

void PLC::SetDiscreteInputValue(std::string deviceName, uint16_t address, bool value) {
    // If value is not present yet, add to discrete inputs
    auto index = m_sensorDiscreteInputsMap.find(std::make_pair(deviceName, address));
    if (index == m_sensorDiscreteInputsMap.end()) {
        m_discreteInputs.insert(m_discreteInputs.end(), value);
        m_sensorDiscreteInputsMap[std::make_pair(deviceName, address)] = m_discreteInputs.size() - 1;
    } else {
        m_discreteInputs[m_sensorDiscreteInputsMap[std::make_pair(deviceName, address)]] = value;
    }
}

bool PLC::GetDiscreteInputValue(std::string deviceName, uint16_t address) {
    return m_discreteInputsCache[GetDiscreteInputAddress(deviceName, address)];
}

uint16_t PLC::GetDiscreteInputAddress(std::string deviceName, uint16_t address) {
    return m_sensorDiscreteInputsMap[std::make_pair(deviceName, address)];
}

uint16_t PLC::GetCustomDiscreteInputStart() {
    return m_customDiscreteInputStart;
}

size_t PLC::GetDiscreteInputCount() {
    CMD_LOG_FUNCTION(this);

    return m_discreteInputs.size();
}

/// -----------------------------
/// Coils
/// -----------------------------

void PLC::SetCoilValue(std::string name, bool value) {
    // If value is not present yet, add to registers
    auto index = m_coilMap.find(name);
    if (index == m_coilMap.end()) {
        m_coils.insert(m_coils.end(), value);
        m_coilMap[name] = m_coils.size() - 1;
    } else {
        m_coils[m_coilMap[name]] = value;
    }
}

bool PLC::GetCoilValue(std::string name) {
    return m_coils[GetCoilAddress(name)];
}

uint16_t PLC::GetCoilAddress(std::string name) {
    return m_coilMap[name];
}

size_t PLC::GetCoilCount() {
    CMD_LOG_FUNCTION(this);

    return m_coils.size();
}

/// -----------------------------
/// Input registers
/// -----------------------------

void PLC::SetInputRegisterValue(std::string deviceName, uint16_t address, uint16_t value) {
    // If value is not present yet, add to registers
    auto index = m_sensorInputRegisterMap.find(std::make_pair(deviceName, address));
    if (index == m_sensorInputRegisterMap.end()) {
        m_inputRegisters.insert(m_inputRegisters.end(), value);
        m_sensorInputRegisterMap[std::make_pair(deviceName, address)] = m_inputRegisters.size() - 1;
    } else {
        m_inputRegisters[m_sensorInputRegisterMap[std::make_pair(deviceName, address)]] = value;
    }
}

uint16_t PLC::GetInputRegisterValue(std::string deviceName, uint16_t address) {
    return m_inputRegistersCache[GetInputRegisterAddress(deviceName, address)];
}

uint16_t PLC::GetInputRegisterAddress(std::string deviceName, uint16_t address) {
    return m_sensorInputRegisterMap[std::make_pair(deviceName, address)];
}

size_t PLC::GetInputRegisterCount() {
    CMD_LOG_FUNCTION(this);

    return m_inputRegisters.size();
}

/// -----------------------------
/// Holding registers
/// -----------------------------

void PLC::SetHoldingRegisterValue(std::string name, uint16_t value) {
    // If value is not present yet, add to registers
    auto index = m_holdingRegisterMap.find(name);
    if (index == m_holdingRegisterMap.end()) {
        m_holdingRegisters.insert(m_holdingRegisters.end(), value);
        m_holdingRegisterMap[name] = m_holdingRegisters.size() - 1;
    } else {
        m_holdingRegisters[m_holdingRegisterMap[name]] = value;
    }
}

uint16_t PLC::GetHoldingRegisterValue(std::string name) {
    return m_holdingRegisters[GetHoldingRegisterAddress(name)];
}

uint16_t PLC::GetHoldingRegisterAddress(std::string name) {
    return m_holdingRegisterMap[name];
}

size_t PLC::GetHoldingRegisterCount() {
    CMD_LOG_FUNCTION(this);

    return m_holdingRegisters.size();
}