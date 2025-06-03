#ifndef CONFIG_HELPER_H
#define CONFIG_HELPER_H

namespace config {
    /// \brief Configuration of cycle times of PLCs
    ///
    /// This value configures the simulation cycle time of all PLCs in milliseconds
    const uint16_t plc_cycle_time_ms = 50;

    /// \brief Configuration of cycle times of HMIs
    ///
    /// This value configures the simulation cycle time of all HMIs in milliseconds
    const uint16_t hmi_cycle_time_ms = 100;

    /// \brief Configuration of the physical system cycle time
    ///
    /// This value configures the simulation cycle time of the underlying physical system in milliseconds
    const uint16_t physical_system_cycle_time_ms = 25;

    /// \brief Configuration of the Modbus request timeout time
    const uint16_t timeout_time_ms = 150;
}

#endif // CONFIG_HELPER_H