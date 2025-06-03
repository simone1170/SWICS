#include "plc-b.h"
#include "config.h"
#include "../helper/command-line-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("PLC_B");

PLC_B::PLC_B(std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators) : PLC("PLC_B", sensors, actuators) {
    CMD_LOG_FUNCTION(this);

    // Assign addresses in PLCs registers to sensor data
    SetInputRegisterValue("bottleFillLevel", 0);
    SetInputRegisterValue("bottlePosition", 0);
    SetCoilValue("hmiHaltProcess", 0);
    SetHoldingRegisterValue("engineSpeed", 100);
}

PLC_B::~PLC_B() {
    CMD_LOG_FUNCTION(this);
}

void PLC_B::RunControlLogic() {
    CMD_LOG_FUNCTION(this);

    // State-chart based cycle time (add 1ms per action, min 30ms)
    m_cycleTime = 30;

    // Check HMI request to halt process
    m_cycleTime += 1; // if
    if (GetCoilValue("hmiHaltProcess")) {
        m_cycleTime += 1; // moving
        m_moving = false;
        if (!m_prevHmiHalt) {
            CMD_LOG_INFO("HMI requested process halt");
        }
        m_prevHmiHalt = true;
        return;
    } else if (m_prevHmiHalt) {
        CMD_LOG_INFO("HMI requested process resumption");
        m_prevHmiHalt = false;
    }

    // Check sensor data available
    for (std::string identifier : {"bottlePosition","bottleFillLevel"}) {
        m_cycleTime += 1; // if
        if (GetSensorTimedOut(identifier)) {
            CMD_LOG_WARN("No current sensor data for " << identifier);
            m_cycleTime += 1; // moving
            m_moving = false;
            return;
        }
        m_cycleTime += 1; // for
    }
    
    m_cycleTime += 2; // read and convert
    uint16_t bottlePositionInt = GetInputRegisterValue("bottlePosition", 0);
    double bottlePosition = ((double) bottlePositionInt / std::numeric_limits<uint16_t>::max()) * (config::conveyor_bottle_position_sensor_max - config::conveyor_bottle_position_sensor_min) + config::conveyor_bottle_position_sensor_min;

    m_cycleTime += 2; // read and convert
    uint16_t bottleFillLevelInt = GetInputRegisterValue("bottleFillLevel", 0);
    double bottleFillLevel = ((double) bottleFillLevelInt / std::numeric_limits<uint16_t>::max()) * config::water_bottle_sensor_max;

    m_cycleTime += 1; // if
    if (GetSensorTimedOut("bottlePosition")) {
        CMD_LOG_WARN("Bottle position sensor data outdated. Stopping motor.");
        m_cycleTime += 1; // moving
        m_moving = false;
    } else {
        m_cycleTime += 1; // if

        // Bottle either not in position, bottle is completely filled & we waited for one cycle to move bottle away (givve time to close valve by PLC_A) or bottle not in position as last bottle has been moved away from filler
        if (bottlePosition > config::conveyor_bottle_acceptable_distance_variation_mm || (m_previousBottleLevel > config::water_bottle_capacity_ml) || (bottleFillLevel <= 1.1 && bottlePosition < -config::conveyor_bottle_acceptable_distance_variation_mm)) {
            m_cycleTime += 1; // if
            if(!m_moving) {
                m_cycleTime += 1; // bottleCount
                m_bottleCount++;
                if ((bottlePosition > config::conveyor_bottle_acceptable_distance_variation_mm) || (bottleFillLevel <= 1.1 && bottlePosition < -config::conveyor_bottle_acceptable_distance_variation_mm)) {
                    CMD_LOG_INFO("Bottle not in position: moving bottle " << m_bottleCount << " towards filler");
                } else if (bottleFillLevel > config::water_bottle_capacity_ml && m_previousBottleLevel > config::water_bottle_capacity_ml) {
                    CMD_LOG_INFO("Bottle filled: moving bottle " << m_bottleCount << " towards filler");
                }
            }
            m_cycleTime += 1; // moving
            m_moving = true;
        } else {
            m_cycleTime += 1; // if
            if(m_moving) {
                CMD_LOG_INFO("Bottle in position: stopping motor");
            }
            m_cycleTime += 1; // moving
            m_moving = false;
        }

        // Store value to detect whether change has taken place
        m_cycleTime += 1; // m_previousBottleLevel
        m_previousBottleLevel = bottleFillLevel;
    }
}

void PLC_B::ControlActuators() {
    CMD_LOG_FUNCTION(this);

    if (m_moving) {
        // Activate motor
        uint16_t engineSpeed = GetHoldingRegisterValue("engineSpeed");
        SendModbusRequest(*m_actuators.at("motorController"), 502, 0x06, {0x00,0x00,(uint8_t)((engineSpeed >> 8) & 0xff), (uint8_t)((engineSpeed >> 0) & 0xff)}); // Set engine speed
        SendModbusRequest(*m_actuators.at("motorController"), 502, 0x0F, {0x00,0x00,0x00,0x02,0x01,0x01});
    } else {
        // Deactivate motor
        SendModbusRequest(*m_actuators.at("motorController"), 502, 0x0F, {0x00,0x00,0x00,0x02,0x01,0x00});
    }
}

void PLC_B::RequestSensorData() {
    CMD_LOG_FUNCTION(this);
    
    // Request first register from sensor at tankWaterLevel
    SendModbusRequest(*m_sensors.at("bottlePosition"), 502, 0x04, {0x00, 0x00, 0x00, 0x01}, RequestContext("bottlePosition",0,1));
    // Request bottleFillLevel
    SendModbusRequest(*m_sensors.at("bottleFillLevel"), 502, 0x04, {0x00, 0x00, 0x00, 0x01}, RequestContext("bottleFillLevel",0,1));
}