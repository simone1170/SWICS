#include "testbed-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("TestbedHelper");

TestbedHelper::TestbedHelper() {
    LogComponentEnable("TestbedHelper", LOG_LEVEL_INFO);
}

TestbedHelper::~TestbedHelper() {}

void TestbedHelper::RegisterCommandLineArguments() {
    // Command-line argument for network selection
    m_cmd.AddValue("use5g", "Use 5G mmWave network (true/false)", m_use5g);
    // Command-line argument for setting the simulation duration
    m_cmd.AddValue("duration", "Duration of the simulation in seconds", m_simulationDuration);
    // Command-line argument for disabling the creation of logfiles
    m_cmd.AddValue("createlogfiles", "Whether to create log files of simulation run (true/false)", m_createlogfiles);
    // Command-line argument for setting the output path
    m_cmd.AddValue("logPcap", "Which pcaps to write (all/gateway)", m_logPcap);
    // Command-line argument for setting the output path
    m_cmd.AddValue("outputPath", "Where to put log files (including pcap)", m_logPathOverride);
    // Command-line argument for attack scheduling (Format: "AttackName|StartSeconds|EndSeconds:AttackName|...")
    m_cmd.AddValue("runAttacks", "Attacks to run in simulation", m_attacksString);
    // Command-line argument for jammer positioning
    m_cmd.AddValue("jammerInside", "Jammer positioned inside building?", m_jammerInside);
    // Command-line argument for jammer directionality
    m_cmd.AddValue("jammerDirected", "Jammer uses beamforming against eNB", m_jammerDirected);

    m_cmd.AddValue("attackNumber", "perform each attack multiple times", m_attackNumber);

    m_cmd.AddValue("attackInterval", "interval between individual attacks of multiple attacks in seconds", m_attackInterval);

    m_cmd.AddValue("jammerPower", "Power used by jammer in dB", m_jammPower);

    m_cmd.AddValue("bjp", "Power used by constant background jammer in dB", m_backgroundJammPower);

    m_cmd.AddValue("run", "Run ID used to initialize the PRNGs", m_run);

    m_cmd.AddValue("dutyCycle", "Duty cycle used by jammer, i.e. percentage of actice time", m_dutyCycle);

    // Logging command-line config
    // Command-line argument for setting the logging level
    m_cmd.AddValue("loggingLevel", "Logging level (" + StringHelper::SerializeVector(m_loggingLevels, ',') + ")", m_loggingLevel);
    // Command-line argument for setting logging component groups
    m_cmd.AddValue("loggingGroup", "Group of components to log (" + StringHelper::SerializeVector(m_loggingGroups, ',') + ")", m_loggingGroup);
    // Command-line argument for enabling additional logging components
    m_cmd.AddValue("includeLoggingComponents", "Components to be included in logging (comma-separated)", m_includeLoggingComponents);
    // Command-line argument for disabling logging components
    m_cmd.AddValue("excludeLoggingComponents", "Components to be excluded in logging (comma-separated)", m_excludeLoggingComponents);
}

bool TestbedHelper::ParseCommandLineArguments(int argc, char *argv[], std::vector<std::string> customComponents) {
    m_cmd.Parse(argc, argv);

    // Components with enabled logging
    std::vector<std::string> loggingComponents;

    // Set logging components
    if (std::find(m_loggingGroups.begin(), m_loggingGroups.end(), m_loggingGroup) != m_loggingGroups.end()) {
        // ToDo: make sure all components are listed (correctly)
        std::vector<std::string> predefinedComponents = {"PLC", "Sensor", "Actuator", "IndustrialDevice", "PhysicalSystem"};
        
        
        if (m_loggingGroup == "ALL" || m_loggingGroup == "PREDEFINED") {
            loggingComponents.insert(loggingComponents.end(), predefinedComponents.begin(), predefinedComponents.end());
        }
        if (m_loggingGroup == "ALL" || m_loggingGroup == "CUSTOM") {
            loggingComponents.insert(loggingComponents.end(), customComponents.begin(), customComponents.end());
        }
    } else {
        CMD_LOG_ERROR(Cmd::Mod(Cmd::CLR_RED) << "Unknown logging group '" << m_loggingGroup << "' specified. Known logging groups: " << StringHelper::SerializeVector(m_loggingGroups, ','));
        CMD_LOG_ERROR("aborting startup...");
        return false;
    }

    // Check pcap log config
    if (m_logPcap != "all" && m_logPcap != "gateway") {
        CMD_LOG_ERROR(Cmd::Mod(Cmd::CLR_RED) << "Unknown pcap log setting '" << m_logPcap << "' specified. Known settings: all,gateway");
        CMD_LOG_ERROR("aborting startup...");
        return false;
    }

    // Include additional logging components (ns-3 will raise an error if component is unknown)
    if (m_includeLoggingComponents.size() > 0) { 
        std::vector<std::string> includeLoggingComponentsVector = StringHelper::StringToVector(m_includeLoggingComponents, ',');
        loggingComponents.insert(loggingComponents.end(), includeLoggingComponentsVector.begin(), includeLoggingComponentsVector.end());
    }

    // Exclude additional logging components
    std::vector<std::string> excludeLoggingComponentsVector = StringHelper::StringToVector(m_excludeLoggingComponents, ',');
    for (size_t i = 0; i < excludeLoggingComponentsVector.size(); ++i) {
        std::vector<std::string>::iterator position = std::find(loggingComponents.begin(), loggingComponents.end(), excludeLoggingComponentsVector[i]);
        if (position != loggingComponents.end()) {
            loggingComponents.erase(position);
        }
    }

    // Set logging level
    if (m_loggingLevel == "INFO") {
        for (size_t i = 0; i < loggingComponents.size(); ++i) {
            CMD_LOG_INFO(loggingComponents[i]);
            LogComponentEnable(loggingComponents[i], LOG_LEVEL_INFO);
        }
    } else if (m_loggingLevel == "WARN") {
        for (size_t i = 0; i < loggingComponents.size(); ++i) {
            LogComponentEnable(loggingComponents[i], LOG_LEVEL_WARN);
        }
    } else if (m_loggingLevel == "ERROR") {
        for (size_t i = 0; i < loggingComponents.size(); ++i) {
            LogComponentEnable(loggingComponents[i], LOG_LEVEL_ERROR);
        }
    } else {
        CMD_LOG_ERROR(Cmd::Mod(Cmd::CLR_RED) << "Unknown logging level '" << m_loggingLevel << "' specified. Known logging levels: " << StringHelper::SerializeVector(m_loggingLevels, ','));
        CMD_LOG_ERROR("aborting startup...");
        return false;
    }

    // Set Log path
    time_t timestamp = time(NULL);
    std::tm * ptm = std::localtime(&timestamp);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", ptm);

    if (m_logPathOverride == "") {
        m_logPath = "./datasets/" + std::string(buffer) + " (" + std::to_string(GetSimulationDuration()) + "s) " + (Using5G() ? "5G" : "wired") + "/";
    } else {
        m_logPath = m_logPathOverride;
        if (m_logPath.back() != '/') {
            m_logPath += "/";
        }
    }
    if (m_createlogfiles) {
        std::filesystem::create_directories(m_logPath);
    }

    // Parse attacks
    try {
        std::string attacksString = m_attacksString;
        while(attacksString.length() > 0) {
            std::string attackString = attacksString.substr(0, attacksString.find(":"));
            if (attacksString.length() == attackString.length()) {
                attacksString = "";
            } else {
                attacksString.erase(0, attacksString.find(":") + 1);
            }

            std::string attackName = attackString.substr(0, attackString.find("|"));
            attackString.erase(0, attackString.find("|") + 1);
            double startTime = std::stod(attackString.substr(0, attackString.find("|")));
            attackString.erase(0, attackString.find("|") + 1);
            double endTime = std::stod(attackString.substr(0, attackString.find("|")));

            m_attacks.insert(m_attacks.end(), AttackScheduleEntry(attackName, Time(Seconds(startTime)), Time(Seconds(endTime))));
        }
    } catch (...) {
        CMD_LOG_ERROR("Could not parse attacks. Aborting.");
        return false;
    }

    return true;
}