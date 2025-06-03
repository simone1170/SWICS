#ifndef SENSOR_H
#define SENSOR_H

#include "ns3/node.h"
#include "ns3/object.h"
#include "physical-system.h"
#include "../helper/config-helper.h"
#include "industrial-device.h"
#include "sensor-property.h"

using namespace ns3;

namespace testbed {
    /// \brief An industrial device that senses physical properties
    class Sensor : public IndustrialDevice {
    public:
        /// \brief Instantiates a sensor reading physical values
        /// \param system The PhysicalSystem from which this sensor senses values
        /// \param identifier A name used for identification in log messages
        /// \param registers A list of properties (double values) the sensor can sense (corresponds to input register addresses in Modbus starting with 0x0000)
        Sensor(Ptr<PhysicalSystem> system, std::string identifier, std::vector<std::string> coils, std::vector<SensorProperty> registers);
        virtual ~Sensor();

        /// \brief Callback method called when receiving modbus data from another IndustrialDevice
        /// \param from Address and port of the sender
        /// \param modbusHeader Header information used in the transmission
        /// \param functionCode Function code associated with the request/response
        /// \param data Data provided in the Modbus PDU (exluding function code)
        /// \param context Context provided in request (only set if this call is due to a response)
        void ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context);

    protected:
        /// \brief PhysicalSystem this sensor interacts with
        Ptr<PhysicalSystem> m_system;

        /// \brief Method to read sensor value at address (input register)
        /// \param address Address to read
        /// \return Value at address
        std::vector<uint8_t> ReadRegisterValue(uint16_t address);

        /// \brief Method to read sensor value at address (discrete input)
        /// \param address Address to read
        /// \return Value at address
        bool ReadCoilValue(uint16_t address);

    private:
        std::vector<std::string> m_coils;
        std::vector<SensorProperty> m_registers;
    };
}

#endif // SENSOR_H