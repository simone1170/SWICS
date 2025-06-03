#include <cstdlib>
#include <limits>
#include "sensor.h"
#include "../helper/command-line-helper.h"
#include "../helper/hex-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("Sensor");

Sensor::Sensor(Ptr<PhysicalSystem> system, std::string identifier, std::vector<std::string> coils, std::vector<SensorProperty> registers)
    : IndustrialDevice(identifier, true), m_system(system), m_coils(coils), m_registers(registers) {
        CMD_LOG_FUNCTION(this << system << identifier << coils << registers);

        SetRecvCallback(MakeCallback(&Sensor::ReceiveModbusData, this));
    }

Sensor::~Sensor() {
    CMD_LOG_FUNCTION(this);
}

void Sensor::ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context) {
    CMD_LOG_FUNCTION(this << from << modbusHeader << functionCode << data << context);

    if (functionCode == 0x02) {
        // Discrete input
        if (data.size() != 4) {
            CMD_LOG_ERROR(m_identifier << ": Got Request to read coils (0x04) but message was " << data.size() << " bytes (expected 4 bytes).");
            functionCode = functionCode + 0x80;
            data = {0x03};
        } else {
            uint16_t address = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);
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
                        if (ReadCoilValue(address+i)) {
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
    } else if (functionCode == 0x04) {
        // Input register
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
            } else if (address + registerCount > m_registers.size()) {
                CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but register count is invalid (" << registerCount << " registers requested at address " << address << " which is greater than last addressable register).");
                functionCode = functionCode + 0x80;
                data = {0x02};
            } else {
                try {
                    uint8_t byteCount = 2 * registerCount;
                    data = {byteCount};
                    for (uint8_t i = 0; i < registerCount; i++) {
                        std::vector<uint8_t> value = ReadRegisterValue(address+i);
                        data.insert(data.end(), value.begin(), value.end());
                    }
                } catch (...) {
                    CMD_LOG_ERROR(m_identifier << ": Got Request to read input registers (0x04) but failed to build response data.");
                    functionCode = functionCode + 0x80;
                    data = {0x04};
                }
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
        // Send error response if command is unknown
        CMD_LOG_ERROR(m_identifier << ": Got Request for unsupported function code " << hex::toHex(functionCode) << ".");

        functionCode = functionCode + 0x80;
        data = {0x01};
    }
    SendModbusResponse(from, modbusHeader, functionCode, data);
}

std::vector<uint8_t> Sensor::ReadRegisterValue(uint16_t address) {
    CMD_LOG_FUNCTION(this << address);

    double sensorValue = m_system->GetValue(m_registers[address].GetKey());

    double derivation = ((((double) rand()*2) / RAND_MAX) - 1) * m_registers[address].GetFault();

    sensorValue += sensorValue * derivation;

    CMD_LOG_INFO(m_identifier << " reading input register value at address " << address << ": " << sensorValue);

    // Convert to two bytes
    double valuePct = (sensorValue - m_registers[address].GetMinValue()) / (m_registers[address].GetMaxValue() - m_registers[address].GetMinValue());
    uint16_t value = valuePct * std::numeric_limits<uint16_t>::max();

    std::vector<uint8_t> result = {(uint8_t)((value >> 8) & 0xff), (uint8_t)((value >> 0) & 0xff)};

    return result;
}

bool Sensor::ReadCoilValue(uint16_t address) {
    CMD_LOG_FUNCTION(this << address);

    bool sensorValue = m_system->GetValue(m_coils[address]) == 1;

    CMD_LOG_INFO(m_identifier << " reading discrete input value at address " << address << ": " << sensorValue);

    return sensorValue;
}