#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "ns3/node.h"
#include "ns3/object.h"
#include "physical-system.h"
#include "industrial-device.h"

namespace testbed {
    /// \brief An industrial device that simulates the behavior of an actuator
    ///
    /// Objects of this class interact with the PhysicalSystem to modify physical values
    class Actuator : public IndustrialDevice {
    public:
        /// \brief Instantiates an actuator interacting with the physical system to modify physical values
        /// \param system The PhysicalSystem with which this actuator interacts
        /// \param identifier A name used for identification of log messages
        /// \param coils A list of identifiers of boolean values the actuator can set in the physical system (corresponds to coil addresses in Modbus starting with 0x0000)
        /// \param registers A list of identifiers of double values the actuator can set in the physical system (corresponds to register addresses in Modbus starting with 0x0000)
        Actuator(Ptr<PhysicalSystem> system, std::string identifier, std::vector<std::string> coils, std::vector<std::string> registers);
        virtual ~Actuator();

        /// \brief Callback method called when receiving modbus data from another IndustrialDevice
        /// \param from Address and port of the sender
        /// \param modbusHeader Header information used in the transmission
        /// \param functionCode Function code associated with the request/response
        /// \param data Data provided in the Modbus PDU (exluding function code)
        /// \param context Context provided in request (only set if this call is due to a response)
        void ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context);
    protected:
        /// \brief PhysicalSystem this actuator interacts with
        Ptr<PhysicalSystem> m_system;

        /// \brief Corresponding names of actuator inputs in physical system
        std::vector<std::string> m_coils;

        /// \brief Corresponding names of actuator inputs in physical system
        std::vector<std::string> m_registers;
    };
}

#endif // ACTUATOR_H