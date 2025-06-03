#ifndef PLC_H
#define PLC_H

#include <unordered_map>
#include <variant>

#include "ns3/core-module.h"
#include "ns3/object.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"
#include "ns3/nstime.h"

#include "../helper/config-helper.h"
#include "sensor.h"
#include "actuator.h"
#include "industrial-device.h"

using namespace ns3;

namespace testbed {
    /// \brief Abstract class implementing usual behavior of PLCs
    class PLC : public IndustrialDevice {
    public:
        /// \brief Instantiates a PLC object
        /// \param identifier identifies the PLC in logging etc
        /// \param sensors a Map of sensor objects this PLC has access to (for reference in requesting data)
        /// \param actuators a Map of actuator objects this PLC has access to (for reference in sending actuator configuration)
        PLC(std::string identifier, std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators);
        virtual ~PLC();

        /// \brief Callback method called when receiving modbus data from another IndustrialDevice
        /// \param from Address and port of the sender
        /// \param modbusHeader Header information used in the transmission
        /// \param functionCode Function code associated with the request/response
        /// \param data Data provided in the Modbus PDU (exluding function code)
        /// \param context Context provided in request (only set if this call is due to a response)
        void ReceiveModbusData(InetSocketAddress from, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> data, RequestContext context);

        /// \brief Method to initiate the standard cyclic behavior of the PLC
        void Cycle();

        /// \brief Retrieves the PLC's identifier
        /// \return the PLC's identifier
        std::string GetIdentifier();

        /// \brief Method to get the number of discrete inputs stored by this PLC
        /// \return the number of discrete inputs stored by this PLC
        size_t GetDiscreteInputCount();

        /// \brief Method to get the address of a discrete input
        /// \param deviceName name of the device (identifier)
        /// \param address address of the discrete input at the source (can be arbitrary if no source device)
        /// \return Address at this PLC
        uint16_t GetDiscreteInputAddress(std::string deviceName, uint16_t address);

        /// \brief Method to get the start of custom discrete inputs (end of timeout discrete inputs)
        /// \return the index where custom discrete inputs start
        uint16_t GetCustomDiscreteInputStart();

        /// \brief Method to get the number of input registers stored by this PLC
        /// \return the number of input registers stored by this PLC
        size_t GetInputRegisterCount();

        /// \brief Method to get the address of an input register
        /// \param deviceName name of the device (identifier)
        /// \param address address of the input register at the source (can be arbitrary if no source device)
        /// \return Address at this PLC
        uint16_t GetInputRegisterAddress(std::string deviceName, uint16_t address);

        /// \brief Method to get the address of an input register
        /// \param name identifier of the coil for better code readability
        /// \return Address at this PLC
        uint16_t GetCoilAddress(std::string name);

        /// \brief Method to get the number of coils stored by this PLC
        /// \return the number of coils stored by this PLC
        size_t GetCoilCount();

        /// \brief Method to get the address of an holding register
        /// \param name identifier of the register for better code readability
        /// \return Address at this PLC
        uint16_t GetHoldingRegisterAddress(std::string name);

        /// \brief Method to get the number of holding registers stored by this PLC
        /// \return the number of holding registers stored by this PLC
        size_t GetHoldingRegisterCount();

    protected:
        /// \brief Map of sensor objects to be accessed by identifiers
        std::unordered_map<std::string, Ptr<Sensor>> m_sensors;
        /// \brief Map of actuator objects to be accessed by identifiers
        std::unordered_map<std::string, Ptr<Actuator>> m_actuators;

        uint32_t m_cycleTime = config::plc_cycle_time_ms;

        /// \brief Run PLC control logic (needs to be implemented by implementing class)
        virtual void RunControlLogic() = 0;

        /// \brief Method to send messages to actuators based on decisions in Cycle() (needs to be implemented by implementing class)
        virtual void ControlActuators() = 0;

        /// \brief Request sensor data from sensors (needs to be implemented by implementing class)
        virtual void RequestSensorData() = 0;

        /// \brief Method to start the PLC application and call corresponding method of IndustrialDevice
        void StartApplication() override;

        /// \brief Method to stop the PLC application and call corresponding method of IndustrialDevice
        void StopApplication() override;

        /// \brief Method handling Modbus request timeouts. Method is called when timeout time has been reached. This method can be overridden by implementations. Sub-classes should always call parent implementation.
        /// \param contextIndex the index of context for the request in m_connectionContext (transactionId, InetSocketAddress)
        void HandleTimeout(std::pair<uint16_t,InetSocketAddress> contextIndex) override;

        /// \brief Method fetching the current timeout state of the given sensor
        /// \param sensorName Name of the sensor used for storing sensor data
        /// \return whether the sensor is timed out
        bool GetSensorTimedOut(std::string sensorName);

        /// \brief Method to add an input register index to the PLCs input registers. Stores name and address of corresponding device for this address. Changes take effect in the next PLC cycle.
        /// \param deviceName Name of the device this value is from/for (e.g., sensor/actuator). Can also be arbitrary value for custom data.
        /// \param address Address of the source/target device. Responses from a sensor will be stored accordingly
        /// \param value Value for the input register
        void SetInputRegisterValue(std::string deviceName, uint16_t address, uint16_t value = 0);

        /// \brief Method to retrieve values stored from/for a device at the given address. Changes to this value only take effect in the next cycle.
        /// \param deviceName Name of the device this value is from/for (e.g., sensor/actuator). Can also be arbitrary value for custom data.
        /// \param address Address of the source/target device. Responses from a sensor will be stored accordingly
        /// \return The current value for the given device at the given address
        uint16_t GetInputRegisterValue(std::string deviceName, uint16_t address);

        /// \brief Method to add a discrete input index to the PLCs discrete inputs. Stores name and address of corresponding device for this address. Changes take effect in the next PLC cycle.
        /// \param deviceName Name of the device this value is from/for (e.g., sensor/actuator). Can also be arbitrary value for custom data.
        /// \param address Address of the source/target device. Responses from a sensor will be stored accordingly
        /// \param value Value for the discrete input
        void SetDiscreteInputValue(std::string deviceName, uint16_t address, bool value = false);

        /// \brief Method to retrieve values stored from/for a device at the given address. Changes to this value only take effect in the next cycle.
        /// \param deviceName Name of the device this value is from/for (e.g., sensor/actuator). Can also be arbitrary value for custom data.
        /// \param address Address of the source/target device. Responses from a sensor will be stored accordingly
        /// \return The current value for the given device at the given address
        bool GetDiscreteInputValue(std::string deviceName, uint16_t address);

        /// \brief Method to add a coil index to the PLCs coils. Stores name and address of corresponding device for this address. Changes take effect in the next PLC cycle.
        /// \param name identifier of the coil for better code readability
        /// \param value Value for the coil
        void SetCoilValue(std::string name, bool value = false);

        /// \brief Method to retrieve values stored from/for a device at the given address. Changes to this value only take effect in the next cycle.
        /// \param name identifier of the coil for better code readability
        /// \return The current value for the given device at the given address
        bool GetCoilValue(std::string name);

        /// \brief Method to add a holding register index to the PLCs holding registers. Stores name and address of corresponding device for this address. Changes take effect in the next PLC cycle.
        /// \param name identifier of the holding register for better code readability
        /// \param value Value for the holding register
        void SetHoldingRegisterValue(std::string name, uint16_t value = 0);

        /// \brief Method to retrieve values stored from/for a device at the given address. Changes to this value only take effect in the next cycle.
        /// \param name identifier of the holding register for better code readability
        /// \return The current value for the given device at the given address
        uint16_t GetHoldingRegisterValue(std::string name);
    private:
        EventId m_cycleEvent;

        std::unordered_map<std::string, std::unordered_map<uint16_t, Time>> m_currentSensorDataRequestTime;

        // Discrete inputs
        std::vector<bool> m_discreteInputs;
        uint16_t m_customDiscreteInputStart;
        std::map<std::pair<std::string, uint16_t>, size_t> m_sensorDiscreteInputsMap; // Maps {sensorName,address} to an index in m_discreteInputs

        // Input registers
        std::vector<uint16_t> m_inputRegisters;
        std::vector<bool> m_discreteInputsCache;
        std::vector<uint16_t> m_inputRegistersCache;
        std::map<std::pair<std::string, uint16_t>, size_t> m_sensorInputRegisterMap; // Maps {sensorName,address} to an index in m_inputRegisters

        // Coils
        std::vector<bool> m_coils;
        std::map<std::string, uint16_t> m_coilMap; // Maps a string to an index in m_coils for better readability of code

        // Holding Registers
        std::vector<uint16_t> m_holdingRegisters;
        std::map<std::string, size_t> m_holdingRegisterMap; // Maps a string to an index in m_holdingRegisters for better readability of code
    };
}

#endif // PLC_H