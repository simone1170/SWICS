#ifndef SIMULATION_CONFIG_H
#define SIMULATION_CONFIG_H

#include <math.h>
#include <cmath>

namespace config {
    // ----------------------------------------------------------
    // Physical properties
    // ----------------------------------------------------------
    /// \brief Diameter of the water tank
    const uint16_t water_tank_diameter_mm = 500;
    /// \brief Height of the water tank
    const uint16_t water_tank_height_mm = 500;
    /// \brief Threshold in pct when PLC_A stops filling tank
    const double water_tank_threshold_max_pct = 0.9;
    /// \brief Threshold in pct when PLC_A starts filling tank
    const double water_tank_threshold_min_pct = 0.1;

    /// \brief Capacity of water bottles to be filled
    const uint16_t water_bottle_capacity_ml = 1000;
    /// \brief Maximal value of sensor reading bottle fill status (used to convert double to two bytes)
    const double water_bottle_sensor_max = 2 * water_bottle_capacity_ml;

    /// \brief Diameter of the in-flow pipe
    #define TANK_WATER_LINE_DIAMETER_MM 50
    /// \brief Diameter of the filler
    #define BOTTLE_FILLER_DIAMETER_MM 10

    /// \brief Distance between bottles on conveyor belt
    const uint16_t conveyor_bottle_distance_in_between_mm = 85;
    /// \brief Maximum of distance after filler (negative) before bottle value is removed
    const uint16_t conveyor_max_bottle_distance_after_filler_mm = conveyor_bottle_distance_in_between_mm/2; // must be smaller than conveyor_bottle_distance_in_between_mm (defines minimum value for distance sensor (*-1))
    /// \brief Maximum value of bottle position sensor
    const double conveyor_bottle_position_sensor_max = conveyor_bottle_distance_in_between_mm * 10;
    /// \brief Minimal value of bottle position sensor
    const double conveyor_bottle_position_sensor_min = (-1) * conveyor_max_bottle_distance_after_filler_mm * 10;
    /// \brief Variation in bottle position that is acceptable (water still makes it into bottle)
    const double conveyor_bottle_acceptable_distance_variation_mm = 5.0;
    /// \brief Speed of conveyor belt in mm/us
    #define CONVEYOR_SPEED_MM_MS 0.05

    /// \brief Duration how long the water leak sensor will detect water after water is spilled
    const double water_leak_duration_ms = 5000;

    // Calculated properties
    /// \brief Capacity of water tank in ml
    /// Calculated from diameter and height
    const double water_tank_capacity_ml = M_PI * pow(water_tank_diameter_mm/2,2) * water_tank_height_mm / 1000;
    /// \brief Threshold when PLC_A stops filling the water tank
    /// Calculated from pct
    const double water_tank_threshold_max_ml = water_tank_capacity_ml * water_tank_threshold_max_pct;
    /// \brief Threshold when PLC_A starts filling the water tank
    /// Calculated from pct
    const double water_tank_threshold_min_ml = water_tank_capacity_ml * water_tank_threshold_min_pct;
    /// \brief Area of the inflow in mm^2
    /// Calculated from diameter
    const double water_tank_outlet_surface_area_mm_2 = M_PI * pow(TANK_WATER_LINE_DIAMETER_MM/2,2);
    /// \brief Area of the filler in mm^2
    /// Calculated from diameter
    const double bottle_filler_surface_area_mm_2 = M_PI * pow(BOTTLE_FILLER_DIAMETER_MM/2,2);
}

#endif // SIMULATION_CONFIG_H