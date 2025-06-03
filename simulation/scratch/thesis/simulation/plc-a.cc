#include <limits>
#include "plc-a.h"
#include "config.h"
#include "../helper/command-line-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("PLC_A");

PLC_A::PLC_A(std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators) : PLC("PLC_A", sensors, actuators) {
    CMD_LOG_FUNCTION(this << Cmd::Preprocess(sensors) << Cmd::Preprocess(actuators));

    // Assign addresses in PLCs registers to sensor data
    SetInputRegisterValue("tankWaterLevel", 0);
    SetInputRegisterValue("bottleFillLevel", 0);
    SetInputRegisterValue("bottlePosition", 0);

    // Assign addresses in PLCs coils to sensor data
    SetDiscreteInputValue("waterLeak", 0);
}

PLC_A::~PLC_A() {
    CMD_LOG_FUNCTION(this);
}

void PLC_A::RunControlLogic() {
    CMD_LOG_FUNCTION(this);

    // State-chart based cycle time (add 1ms per action, min 30ms)
    m_cycleTime = 30;

    // Check for water spillage
    m_cycleTime += 1; // if
    if (GetSensorTimedOut("waterLeak")) {
        CMD_LOG_WARN("No current data for water leak sensor. Closing all valves.");
        m_flowIn = false;
        m_flowOut = false;
        m_cycleTime += 2; // flowIn flowOut
        return;
    }
    m_cycleTime += 1; // if
    if (GetDiscreteInputValue("waterLeak", 0)) {
        CMD_LOG_WARN("Water spillage detected. Closing all valves.");
        m_flowIn = false;
        m_flowOut = false;
        m_cycleTime += 2; // flowIn flowOut
        return;
    }
    
    // Read and convert tankWaterLevel
    m_cycleTime += 2; // read and convert
    uint16_t tankWaterLevelInt = GetInputRegisterValue("tankWaterLevel", 0);
    double tankWaterLevel = ((double) tankWaterLevelInt / std::numeric_limits<uint16_t>::max()) * config::water_tank_capacity_ml; // calculate sensor value from levels

    // Read and convert bottleFillLevel
    m_cycleTime += 2; // read and convert
    uint16_t bottleFillLevelInt = GetInputRegisterValue("bottleFillLevel", 0);
    double bottleFillLevel = ((double) bottleFillLevelInt / std::numeric_limits<uint16_t>::max()) * config::water_bottle_sensor_max;

    // Read and convert bottlePosition
    m_cycleTime += 2; // read and convert
    uint16_t bottlePositionInt = GetInputRegisterValue("bottlePosition", 0);
    double bottlePosition = ((double) bottlePositionInt / std::numeric_limits<uint16_t>::max()) * (config::conveyor_bottle_position_sensor_max - config::conveyor_bottle_position_sensor_min) + config::conveyor_bottle_position_sensor_min;

    // Sensor data logging (for comparative evaluation)
    // CMD_LOG_INFO("bottleFillLevel Sensor Data: " << std::to_string(bottleFillLevel));
    // CMD_LOG_INFO("waterFlowOut Actuator Data: " << m_flowOut);

    m_cycleTime += 1; // if
    if (GetSensorTimedOut("tankWaterLevel")) {
        CMD_LOG_WARN("Water level sensor data outdated. Stopping filling.");
        m_cycleTime += 1; // flowIn
        m_flowIn = false;
    } else {
        m_cycleTime += 1; // if
        m_cycleTime += 1; // else if
        if (tankWaterLevel < config::water_tank_threshold_min_ml) {
            // Check previous value for logging
            m_cycleTime += 1; // if
            if (!m_flowIn) {
                CMD_LOG_INFO("Reached minimum threshold: filling tank");
            }
            // Set new value
            m_cycleTime += 1; // flowIn
            m_flowIn = true;
            m_cycleTime -= 1; // else if not called
        } else if (tankWaterLevel > config::water_tank_threshold_max_ml) {
            // Check previous value for logging
            m_cycleTime += 1; // if
            if (m_flowIn) {
                CMD_LOG_INFO("Reached maximum threshold: stopping filling tank");
            }
            // Set new value
            m_cycleTime += 1; // flowIn
            m_flowIn = false;
        }
    }
    m_cycleTime += 1; // if
    m_cycleTime += 1; // else if
    if (GetSensorTimedOut("bottleFillLevel")) {
        CMD_LOG_WARN("Bottle fill level sensor data outdated. Stopping filling.");
        m_cycleTime += 1; // flowOut
        m_flowOut = false;
        m_cycleTime -= 1; // else if not called
    } else if (GetSensorTimedOut("bottlePosition")) {
        CMD_LOG_WARN("Bottle position sensor data outdated. Stopping filling.");
        m_cycleTime += 1; // flowOut
        m_flowOut = false;
    } else {
        m_cycleTime += 1; // if
        if (bottleFillLevel < config::water_bottle_capacity_ml && -0.9*config::conveyor_bottle_acceptable_distance_variation_mm <= bottlePosition && bottlePosition <= 0.9*config::conveyor_bottle_acceptable_distance_variation_mm) {
            // Check previous value for logging
            m_cycleTime += 1; // if
            if (!m_flowOut) {
                CMD_LOG_INFO("Starting to fill bottle");
            }
            // Set new value
            m_cycleTime += 1; // flowOut
            m_flowOut = true;
        } else {
            // Check previous value for logging
            m_cycleTime += 1; // if
            if (m_flowOut) {
                m_cycleTime += 1; // if
                if (bottleFillLevel >= config::water_bottle_capacity_ml) {
                    m_cycleTime += 1; // fillCount
                    m_fillCount++;
                    CMD_LOG_INFO("Bottle capacity reached: stopping filling bottle. Filled " << m_fillCount << " bottles (" << std::to_string(bottleFillLevel) << "ml).");
                } else {
                    CMD_LOG_WARN("Bottle moved away from filler but has not been fully filled. Stopping filling bottle.");
                }
            }
            // Set new value
            m_cycleTime += 1; // flowOut
            m_flowOut = false;
        }
    }
}

void PLC_A::ControlActuators() {
    CMD_LOG_FUNCTION(this);

    if (m_flowIn) {
        // Open flow in valve
        SendModbusRequest(*m_actuators.at("tankWaterFlowIn"), 502, 0x05, {0x00,0x00,0xFF,0x00}, RequestContext("tankWaterFlowIn",0,1));
    } else {
        // Close flow in valve
        SendModbusRequest(*m_actuators.at("tankWaterFlowIn"), 502, 0x05, {0x00,0x00,0x00,0x00}, RequestContext("tankWaterFlowIn",0,1));
    }

    if (m_flowOut) {
        // Open flow out valve
        SendModbusRequest(*m_actuators.at("tankWaterFlowOut"), 502, 0x05, {0x00,0x00,0xFF,0x00}, RequestContext("tankWaterFlowOut",0,1));
    } else {
        // Close flow out valve
        SendModbusRequest(*m_actuators.at("tankWaterFlowOut"), 502, 0x05, {0x00,0x00,0x00,0x00}, RequestContext("tankWaterFlowOut",0,1));
    }
}

void PLC_A::RequestSensorData() {
    CMD_LOG_FUNCTION(this);

    // Request first register from sensor at tankWaterLevel
    SendModbusRequest(*m_sensors.at("tankWaterLevel"), 502, 0x04, {0x00, 0x00, 0x00, 0x01}, RequestContext("tankWaterLevel",0,1));
    // Request bottleFillLevel
    SendModbusRequest(*m_sensors.at("bottleFillLevel"), 502, 0x04, {0x00, 0x00, 0x00, 0x01}, RequestContext("bottleFillLevel",0,1));
    // Request bottleFillLevel
    SendModbusRequest(*m_sensors.at("bottlePosition"), 502, 0x04, {0x00, 0x00, 0x00, 0x01}, RequestContext("bottlePosition",0,1));
    // Request waterLeak
    SendModbusRequest(*m_sensors.at("waterLeak"), 502, 0x02, {0x00, 0x00, 0x00, 0x01}, RequestContext("waterLeak",0,1));
}