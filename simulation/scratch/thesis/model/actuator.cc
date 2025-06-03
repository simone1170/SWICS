#include "actuator.h"
#include "../helper/command-line-helper.h"
#include "../helper/hex-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("Actuator");

Actuator::Actuator(Ptr<PhysicalSystem> system, std::string identifier, std::vector<std::string> coils, std::vector<std::string> registers)
    : IndustrialDevice(identifier, true), m_system(system), m_coils(coils), m_registers(registers) {
        CMD_LOG_FUNCTION(this << system << identifier << coils << registers);

        SetRecvCallback(MakeCallback(&Actuator::ReceiveModbusData, this));
    }

Actuator::~Actuator() { }

void Actuator::ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context) {
    CMD_LOG_FUNCTION(this << from << modbusHeader << functionCode << data << context);

    if (data.size() < 2) {
        functionCode = functionCode + 0x80;
        data = {0x02};
    } else {
        // Extract address from data
        uint16_t address = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);
        if (functionCode == 0x01) {
            // Read coils
            if (data.size() != 4) {
                CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x04) but message was " << data.size() << " bytes (expected 4 bytes).");
                functionCode = functionCode + 0x80;
                data = {0x03};
            } else {
                uint16_t coilCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                if (coilCount > 0x7D0 || coilCount < 1) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x04) but coil count is invalid (" << coilCount << " not between 0x01 and 0x7D0).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else if (address + coilCount > m_coils.size()) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x04) but coil count is invalid (" << coilCount << " coils requested at address " << address << " which is greater than last addressable coil).");
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else {
                    try {
                        uint8_t byteCount = 1 + (coilCount-1) / 8;
                        data = {byteCount};
                        uint8_t currentByte = 0;
                        for (uint8_t i = 0; i < coilCount; i++) {
                            uint8_t offset = i % 8;
                            if (m_system->GetActuatorInput(m_coils[address+i]) == 1) {
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
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x04) but failed to convert boolean values to bytes.");
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
                uint16_t registerCount = static_cast<uint16_t>(data[3]) | (static_cast<uint16_t>(data[2]) << 8);
                if (registerCount > 0x7D || registerCount < 1) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but register count is invalid (" << registerCount << " not between 0x01 and 0x7D).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                } else if (address + registerCount > m_registers.size()) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but register count is invalid (" << registerCount << " registers requested at address " << address << " which is greater than last addressable register).");
                    functionCode = functionCode + 0x80;
                    data = {0x02};
                } else {
                    try {
                        uint8_t byteCount = 2 * registerCount;
                        data = {byteCount};
                        for (uint8_t i = 0; i < registerCount; i++) {
                            uint16_t value = m_system->GetActuatorInput(m_registers[address+i]);
                            std::vector<uint8_t> valueVct = {(uint8_t)((value >> 8) & 0xff), (uint8_t)((value >> 0) & 0xff)};
                            data.insert(data.end(), valueVct.begin(), valueVct.end());
                        }
                    } catch (...) {
                        CMD_LOG_ERROR(m_identifier << ": Got Request to read holding registers (0x03) but failed to build response data.");
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
                    m_system->SetActuatorInput(m_coils[address],1);
                } else if (data[2] == 0x00 && data[3] == 0x00) {
                    m_system->SetActuatorInput(m_coils[address],0);
                } else {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to write coil (0x05) but value is not boolean (got " << hex::toHex({data[2], data[3]}) << ", expected 0x00 0x00 or 0xFF 0x00).");
                    functionCode = functionCode + 0x80;
                    data = {0x03};
                }
            }
        } else if (functionCode == 0x06) {
            // Write single register
            if (address >= m_registers.size()) {
                CMD_LOG_ERROR(m_identifier << ": Got Request to write register (0x06) but address was too large (" << address << " > " << m_registers.size() << ").");
                functionCode = functionCode + 0x80;
                data = {0x02};
            } else if (data.size() != 4) {
                CMD_LOG_ERROR(m_identifier << ": Got Request to write register (0x06) but message was " << data.size() << " bytes (expected 4 bytes).");
                functionCode = functionCode + 0x80;
                data = {0x03};
            } else {
                uint16_t value = (static_cast<uint16_t>(data[2] << 8) | static_cast<uint16_t>(data[3])); // First two bytes following address
                m_system->SetActuatorInput(m_registers[address],value);
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
                            m_system->SetActuatorInput(m_coils[address+offset],coilValue);
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
            // Write multiple registers
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
            } else if (address + registerCount > m_registers.size()) {
                functionCode = functionCode + 0x80;
                data = {0x02};
            } else {
                try {
                    for (uint8_t i = 0; i < registerCount; i++) {
                        uint16_t value = (static_cast<uint16_t>(data[5+2*i] << 8) | static_cast<uint16_t>(data[5+5*i+1])); // Copy two value bytes for each register (iterating using i)
                        m_system->SetActuatorInput(m_registers[address],value);
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