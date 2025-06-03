#include "../helper/command-line-helper.h"
#include "bottle-filling-system.h"
#include "config.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("BottleFillingSystem");

BottleFillingSystem::BottleFillingSystem(Ptr<TestbedHelper> testbed) : PhysicalSystem(testbed) {
    CMD_LOG_FUNCTION(this);

    // ----------------------------------------------------------
    // Water tank level simulation
    // ----------------------------------------------------------
    SetActuatorInput("tankWaterFlowIn", 0);
    SetActuatorInput("tankWaterFlowOut", 0);
    RegisterValue("tankWaterLevel", 0, [this](double secondsElapsed) {
        double currentValue = GetValue("tankWaterLevel"); // in ml
        double tankWaterFlowIn = GetActuatorInput("tankWaterFlowIn"); // 0..1
        double tankWaterFlowOut = GetActuatorInput("tankWaterFlowOut"); // 0..1

        // TODO: sanity check values (e.g., tankWaterFlowOut 0..1)

        // Calculate water flow out according to Torricelli's Law: v = sqrt(2 * gravity * h) w/ flow rate = v * outletArea
        double currentWaterTankHeight = currentValue / config::water_tank_capacity_ml * config::water_tank_height_mm; // in mm
        double realisticWaterFlowOut = std::sqrt(2 * (9.81 * 1000) * currentWaterTankHeight) * config::bottle_filler_surface_area_mm_2 * tankWaterFlowOut / 1000; // ml/s leaving the water tank
        
        // Calculate water flow based on pressure and pipe size (Bernoulli equation and flow rate formulas)
        double waterPressure = 1 * 100000;// in pascal (bar * 100,000)
        double waterFlowSpeed = sqrt(2*waterPressure/1000) * 1000; // in mm/s; u = sqrt(2*q/p)
        double realisticWaterFlowIn = waterFlowSpeed * config::water_tank_outlet_surface_area_mm_2 * tankWaterFlowIn / 1000; // ml/s entering the water tank

        double updatedValue = currentValue + (realisticWaterFlowIn - realisticWaterFlowOut) * secondsElapsed;

        // Cannot remove more water from tank than is in tank
        if (currentValue + realisticWaterFlowIn * secondsElapsed < realisticWaterFlowOut * secondsElapsed) {
            realisticWaterFlowOut = currentValue + realisticWaterFlowIn * secondsElapsed;
        }

        if (updatedValue < 0) {
            updatedValue = 0;
        } else if (updatedValue > config::water_tank_capacity_ml) {
            updatedValue = config::water_tank_capacity_ml;
            CMD_LOG_WARN("Water tank overflow");
            SetValue("waterSpillRemaining", config::water_leak_duration_ms);
        }

        // Bottle filling simulation (calculated here to ensure concise inflow/outflow values / prevent race conditions)
        double currentBottleValue = GetValue("bottleFillLevel");
        double bottlePosition = GetValue("bottlePosition");
        double bottleWaterFlow = realisticWaterFlowOut;

        double updatedBottleValue = currentBottleValue;

        // Restore previous bottle fill level if bottle is moved under filler again
        if (m_previousBottleFillLevel != -1 && currentBottleValue == 0) {
            updatedBottleValue = m_previousBottleFillLevel;
        }

        // Increase waterLevel if bottle under filler
        if (-config::conveyor_bottle_acceptable_distance_variation_mm <= bottlePosition && bottlePosition <= config::conveyor_bottle_acceptable_distance_variation_mm) {
            updatedBottleValue += bottleWaterFlow * secondsElapsed;
            
            if (updatedBottleValue > config::water_bottle_capacity_ml * 1.1) {
                updatedBottleValue = config::water_bottle_capacity_ml * 1.1;
                CMD_LOG_WARN("Bottle overflow. Water spilled.");
                SetValue("waterSpillRemaining", config::water_leak_duration_ms);
            }
            SetValue("bottleFillLevel", updatedBottleValue);
        } else {
            // Store for possible restore
            m_previousBottleFillLevel = updatedBottleValue;
            SetValue("bottleFillLevel", 0);

            // Log warning for water spillage
            if (bottleWaterFlow > 0) {
                CMD_LOG_WARN("No bottle present. Water spilled.");
                SetValue("waterSpillRemaining", config::water_leak_duration_ms);
            }
        }

        if (updatedBottleValue != currentBottleValue) {
            CMD_LOG_INFO("Bottle fill level updated, new value: " << updatedBottleValue);
        }

        if (updatedValue != currentValue) {
            CMD_LOG_INFO("Tank water level updated, new value: " << updatedValue);
        }
        return updatedValue;
    });

    // ----------------------------------------------------------
    // Conveyor belt simulation
    // ----------------------------------------------------------
    SetActuatorInput("conveyorBeltEngineStart", 0); // 0..1
    SetActuatorInput("conveyorBeltEngineSpeed", 100); // 0..200
    SetValue("conveyorBeltEngineActualSpeed", 0);
    SetActuatorInput("conveyorBeltEngineDirection", 0); // 1 = clockwise, 0 = counter clockwise
    RegisterValue("bottlePosition", config::conveyor_bottle_distance_in_between_mm, [this](double secondsElapsed) {
        double bottlePosition = GetValue("bottlePosition");
        bool startValue = GetActuatorInput("conveyorBeltEngineStart") == 1;
        double engineSpeedTarget = GetActuatorInput("conveyorBeltEngineSpeed");
        double engineSpeed = GetValue("conveyorBeltEngineActualSpeed");
        bool direction = GetActuatorInput("conveyorBeltEngineDirection") == 1;

        // Motor speed-up and slow-down
        double actualTarget = engineSpeedTarget;
        if (!startValue) {
            actualTarget = 0;
        } else if (direction) {
            actualTarget = -engineSpeedTarget;
        }

        double speedUpTime_s = 0.10;
        engineSpeed = engineSpeed + (actualTarget-engineSpeed) * (secondsElapsed/speedUpTime_s);

        // Make sure the target is reached
        if ((actualTarget > engineSpeed && engineSpeed+0.1 > actualTarget) || (actualTarget < engineSpeed && engineSpeed-0.1 < actualTarget)) {
            engineSpeed = actualTarget;
        }

        if (engineSpeed > 200) {
            engineSpeed = 200;
        } else if (engineSpeed < -200) {
            engineSpeed = -200;
        }
        SetValue("conveyorBeltEngineActualSpeed", engineSpeed);

        // Convert engine speed to mm/s -> mm
        double movement = engineSpeed / 4 * secondsElapsed;

        if (direction) {
            CMD_LOG_WARN("Conveyor belt moving clockwise. Bottle moving further away.");
        }

        bottlePosition -= movement;

        // Assuming bottle is taken off of conveyor belt 50mm after filler
        while (bottlePosition < -(config::conveyor_max_bottle_distance_after_filler_mm)) {
            bottlePosition += config::conveyor_bottle_distance_in_between_mm;
            m_previousBottleFillLevel = -1;
        }

        if (movement != 0) {
            CMD_LOG_INFO("Bottle position updated, new bottle position: " << bottlePosition);
        }

        return bottlePosition;
    });

    SetValue("bottleFillLevel", 0); // Register bottle fill level to be updated by water tank simulation

    // Water leak sensor
    SetValue("waterSpillDetected", 0); // Boolean value set from following (ms) value
    RegisterValue("waterSpillRemaining", 0, [this](double secondsElapsed) {
        double waterSpillRemaining = GetValue("waterSpillRemaining");
        if (waterSpillRemaining - secondsElapsed*1000 > 0) {
            waterSpillRemaining -= secondsElapsed*1000;
            SetValue("waterSpillDetected", 1);
        } else {
            waterSpillRemaining = 0;
            SetValue("waterSpillDetected", 0);
        }

        return waterSpillRemaining;
    });
}

BottleFillingSystem::~BottleFillingSystem() {
    CMD_LOG_FUNCTION(this);
}