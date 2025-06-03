#include "hmi.h"
#include "../helper/command-line-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("HMI");

// TODO: does not currently support requesting more registers/coils than fit into one message; response does not use starting address for timeouts -> in very large setup support might be required.

HMI::HMI(std::string identifier, std::vector<Ptr<PLC>> plcs) : IndustrialDevice(identifier, false), m_plcs(plcs) {
    CMD_LOG_FUNCTION(this << plcs)

    SetRecvCallback(MakeCallback(&HMI::ReceiveModbusData, this));
}

HMI::~HMI() {
}

void HMI::ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context) {
    CMD_LOG_FUNCTION(this << from << modbusHeader << functionCode << data << context);

    if (context.GetName() != "") {
        if (functionCode == 0x02) {
            if (data[0] == data.size()-1) {
                for (Ptr<PLC> plc : m_plcs) {
                    if (plc->GetIdentifier() == context.GetName()) {
                        // Check for timeouts
                        for (auto i = 0; i < plc->GetCustomDiscreteInputStart(); i++) {
                            uint8_t bitIndex = i % 8;
                            uint8_t byteIndex = (i-bitIndex+8) / 8; // Add 8 to start in data[1]
                            bool timeout = (data[byteIndex] >> bitIndex) & 1;
                            if (timeout) {
                                CMD_LOG_WARN(m_identifier << ": " << context.GetName() << " indicated a timeout.");
                            }
                        }
                        return;
                    }
                }
                CMD_LOG_ERROR(m_identifier << ": Got response for read discrete inputs (0x02) but was unable to find corresponding PLC for timeout analysis.");
            } else {
                CMD_LOG_ERROR(m_identifier << ": Got response for read discrete inputs (0x02) but got a byte count mismatch (got " << data.size()-1 << " bytes with boolean values but expected " << std::to_string(data[0]) << " bytes).");
            }
        }
    } else {
        CMD_LOG_ERROR(m_identifier << ": Got a request but device is not a server.");
    }
}

void HMI::Cycle() {
    CMD_LOG_FUNCTION(this);

    Simulator::Schedule(MicroSeconds(10), &HMI::RequestCoils, this);
    Simulator::Schedule(MicroSeconds(20), &HMI::RequestDiscreteInputs, this);
    Simulator::Schedule(MicroSeconds(30), &HMI::RequestHoldingRegisters, this);
    Simulator::Schedule(MicroSeconds(40), &HMI::RequestInputRegisters, this);

    m_cycleEvent = Simulator::Schedule(MilliSeconds(config::hmi_cycle_time_ms), &HMI::Cycle, this);
}

void HMI::RequestCoils() {
    for (Ptr<PLC> plc : m_plcs) {
        // Request all coils data from PLC
        uint16_t coilCount = plc->GetCoilCount();
        if (coilCount > 0) {
            SendModbusRequest(*plc, 502, 0x01, {0x00, 0x00, (uint8_t)((coilCount >> 8) & 0xff), (uint8_t)((coilCount >> 0) & 0xff)}, RequestContext(plc->GetIdentifier(), 0, coilCount));
        }
    }  
}

void HMI::RequestDiscreteInputs() {
    for (Ptr<PLC> plc : m_plcs) {
        // Request all discrete input data from PLC
        uint16_t discreteInputCount = plc->GetDiscreteInputCount();
        if (discreteInputCount > 0) {
            SendModbusRequest(*plc, 502, 0x02, {0x00, 0x00, (uint8_t)((discreteInputCount >> 8) & 0xff), (uint8_t)((discreteInputCount >> 0) & 0xff)}, RequestContext(plc->GetIdentifier(), 0, discreteInputCount));
        }
    }  
}

void HMI::RequestHoldingRegisters() {
    for (Ptr<PLC> plc : m_plcs) {
        // Request all holding register data from PLC
        uint16_t holdingRegisterCount = plc->GetHoldingRegisterCount();
        if (holdingRegisterCount > 0) {
            SendModbusRequest(*plc, 502, 0x03, {0x00, 0x00, (uint8_t)((holdingRegisterCount >> 8) & 0xff), (uint8_t)((holdingRegisterCount >> 0) & 0xff)}, RequestContext(plc->GetIdentifier(), 0, holdingRegisterCount));
        }
    }  
}

void HMI::RequestInputRegisters() {
    for (Ptr<PLC> plc : m_plcs) {
        // Request all input register data from PLC
        uint16_t inputRegisterCount = plc->GetInputRegisterCount();
        if (inputRegisterCount > 0) {
            SendModbusRequest(*plc, 502, 0x04, {0x00, 0x00, (uint8_t)((inputRegisterCount >> 8) & 0xff), (uint8_t)((inputRegisterCount >> 0) & 0xff)}, RequestContext(plc->GetIdentifier(), 0, inputRegisterCount));
        }
    }  
}

void HMI::StartApplication() {
    CMD_LOG_FUNCTION(this);

    Cycle();
    IndustrialDevice::StartApplication();
}

void HMI::StopApplication() {
    CMD_LOG_FUNCTION(this);

    if (m_cycleEvent.IsRunning()) {
        Simulator::Cancel(m_cycleEvent);
    }
    IndustrialDevice::StopApplication();
}