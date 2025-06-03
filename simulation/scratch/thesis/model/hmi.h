#ifndef HMI_H
#define HMI_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"

#include "industrial-device.h"
#include "plc.h"

using namespace ns3;

namespace testbed{
    /// \brief Simulates a HMI that requests all data (input coils and input registers) from all PLCs given. It does not do anything with this requested data except logging a warning that a timeout at the PLC has been detected.
    class HMI : public IndustrialDevice {
    public:
        /// \brief Instantiates a HMI to request all available data from PLCs
        /// \param identifier Name to identify this device in logs, pcap etc.
        /// \param plcs the PLCs from which data should be requested
        HMI(std::string identifier, std::vector<Ptr<PLC>> plcs);
        virtual ~HMI();

        /// \brief Callback method called when receiving modbus data from another IndustrialDevice
        /// \param from Address and port of the sender
        /// \param modbusHeader Header information used in the transmission
        /// \param functionCode Function code associated with the request/response
        /// \param data Data provided in the Modbus PDU (excluding functionCode)
        /// \param context Context provided in the request (only set if this call is due to a response)
        void ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context);
    protected:
        /// \brief Method to start the HMI application and call corresponding method of IndustrialDevice
        void StartApplication() override;

        /// \brief Method to stop the HMI application and call corresponding method of IndustrialDevice
        void StopApplication() override;
    private:
        std::vector<Ptr<PLC>> m_plcs;
        EventId m_cycleEvent;

        /// \brief Cyclic method to request all data periodically
        void Cycle();
        void RequestCoils();
        void RequestDiscreteInputs();
        void RequestHoldingRegisters();
        void RequestInputRegisters();
    };
}

#endif // HMI_H