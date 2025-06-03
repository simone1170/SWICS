#ifndef TESTBED_HELPER_H
#define TESTBED_HELPER_H

#include <filesystem>
#include "ns3/core-module.h"
#include "string-helper.h"
#include "command-line-helper.h"

#include "../attacks/attack-schedule-entry.h"

using namespace ns3;

namespace testbed {
    /// \brief Provides methods required in any testbed implemented in this framework
    ///
    /// This includes adding a command line option to enable or disable 5G (--use5g), setting simulation duration (--duration) and setting logging levels for components
    class TestbedHelper : public Object {
    public:
        TestbedHelper();
        ~TestbedHelper();

        /// \brief Registers common command line arguments (e.g., use5g)
        void RegisterCommandLineArguments();

        /// \brief Parses command line arguments previously registered
        /// \param argc Arguments provided in console call
        /// \param argv Arguments provided in console call
        /// \param customComponents List of custom components to be enabled if loggingGroup "custom" is enabled
        /// \return Returns true if successful
        bool ParseCommandLineArguments(int argc, char *argv[], std::vector<std::string> customComponents);

        /// \brief Provides the CommandLine object in use
        /// \return Returns the CommandLien object in use
        inline CommandLine GetCommandLine() {
            return m_cmd;
        }

        /// \brief Whether 5G is enabled
        /// \return true if 5G is enabled
        inline bool Using5G() {
            return m_use5g;
        }

        /// \brief Whether to create logfiles for simulation run
        /// \return true if log files should be created
        inline bool CreateLogFilesEnabled() {
            return m_createlogfiles;
        }

        /// \brief Gets the simulation duration
        /// \return The simulation duration
        inline uint32_t GetSimulationDuration() {
            return m_simulationDuration;
        }

        /// \brief Gets the path where to store log files
        /// \return the path to store log files
        inline std::string GetLogPath() {
            return m_logPath;
        }

        /// \brief Gets the scheduled attacks
        /// \return the scheduled attacks
        inline std::vector<AttackScheduleEntry> GetAttacks() {
            return m_attacks;
        }

        /// \brief Gets whether the jammer is positioned inside the building
        /// \return whether the jammer is positioned inside the building
        inline bool GetJammerInside() {
            return m_jammerInside;
        }

        /// \brief Gets whether the jammer is directional
        /// \return whether the jammer is directional
        inline bool GetJammerDirected() {
            return m_jammerDirected;
        }

        /// \brief Gets the setting which pcap files are set to be created
        /// \return the pcap setting
        inline std::string GetPcapMode() {
            return m_logPcap;
        }

        /// \brief Gets the number of attacks in each multi-attack
        /// \return number of repetitions per scheduled attack
        inline uint32_t GetAttackNumber() {
            return m_attackNumber;
        }
            
        /// \brief Gets the interval between attacks of one multi-attack
        /// \return interval in seconds between attack repetitions
        inline double GetAttackInterval() {
            return m_attackInterval;
        }

        inline double GetJammPower() {
            return m_jammPower;
        }

        inline double GetBackgroundJammPower() {
            return m_backgroundJammPower;
        }

        inline uint32_t GetRunNumber(){
            return m_run;
        }

        inline double GetDutyCycle() {
            return m_dutyCycle;
        }

    private:
        CommandLine m_cmd;
        uint32_t m_simulationDuration = 20;
        bool m_use5g = true;
        bool m_createlogfiles = true;
        std::string m_logPath = "./datasets";
        std::string m_logPathOverride = "";
        std::string m_logPcap = "all";
        std::string m_attacksString;
        std::vector<AttackScheduleEntry> m_attacks;
        bool m_jammerInside = true;
        bool m_jammerDirected = false;
        uint32_t m_attackNumber = 1;
        double m_attackInterval = 1; // Interval between individual attacks making up one multi-attack in seconds
        double m_jammPower = 0;
        double m_backgroundJammPower = 0;
        uint32_t m_run = 1;
        double m_dutyCycle = 1; // Duty Cycle used by the jammer

        std::vector<std::string> m_loggingLevels = {"INFO", "WARN", "ERROR"};
        std::string m_loggingLevel = "ERROR";
        std::vector<std::string> m_loggingGroups = {"ALL", "PREDEFINED", "CUSTOM", "NONE"};
        std::string m_loggingGroup = "ALL";
        std::string m_includeLoggingComponents = "";
        std::string m_excludeLoggingComponents = "";
    };
}

#endif // TESTBED_HELPER_H