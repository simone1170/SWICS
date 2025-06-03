#include "physical-system.h"
#include "../helper/command-line-helper.h"

using namespace ns3;
using namespace testbed;

NS_LOG_COMPONENT_DEFINE("PhysicalSystem");

PhysicalSystem::PhysicalSystem(Ptr<TestbedHelper> testbed) : m_testbed(testbed) {
    CMD_LOG_FUNCTION(this);

    Cycle();

    if (m_testbed->CreateLogFilesEnabled()) {
        m_logFile.open(m_testbed->GetLogPath() + "physical-state.csv");
    }
}

PhysicalSystem::~PhysicalSystem() {
    CMD_LOG_FUNCTION(this);

    if (m_testbed->CreateLogFilesEnabled()) {
        m_logFile.close();
    }
}

void PhysicalSystem::Update(Time dt) {
    CMD_LOG_FUNCTION(this << dt);

    double timeInSeconds = dt.GetSeconds();

    // Update all registered values
    for (auto& entry : m_values) {
        const std::string& key = entry.first;

        // Call the update function for each value
        if (m_valuesUpdateFunction.find(key) != m_valuesUpdateFunction.end()) {
            m_values[key] = m_valuesUpdateFunction[key](timeInSeconds);
            CMD_LOG_INFO(key << ":" << m_values[key]);
        } else {
            CMD_LOG_INFO("No update function for value " << key << " registered. This value can only be updated manually (or as a consequence of other update functions).");
        }
    }
}

void::PhysicalSystem::Cycle() {
    CMD_LOG_FUNCTION(this);

    Update(Time(std::to_string(m_cycleTime) + "ms"));

    if (m_testbed->CreateLogFilesEnabled()) {
        LogState();
    }

    m_cycleEvent = Simulator::Schedule(MilliSeconds(m_cycleTime), &PhysicalSystem::Cycle, this);
}

void PhysicalSystem::RegisterValue(std::string key, double initialValue, std::function<double(double)> updateFunction) {
    CMD_LOG_FUNCTION(this << key << initialValue << "updateFunction"); // cannot log function as it is not named

    m_values[key] = initialValue;
    m_valuesUpdateFunction[key] = updateFunction;
}

double PhysicalSystem::GetValue(std::string key) const {
    CMD_LOG_FUNCTION(this << key);

    return m_values.at(key);
}

void PhysicalSystem::SetValue(std::string key, double value) {
    CMD_LOG_FUNCTION(this << key << value);
    m_values[key] = value;
}

void PhysicalSystem::SetActuatorInput(std::string key, double input) {
    CMD_LOG_FUNCTION(this << key << input);

    m_inputs[key] = input;
}

double PhysicalSystem::GetActuatorInput(std::string key) {
    CMD_LOG_FUNCTION(this << key);

    return m_inputs[key];
}

void PhysicalSystem::LogState() {
    if (!m_fileHeaderCreated && m_values.size() > 0) {
        // Create header (column names)
        m_logFile << "timestamp,";
        for (auto [key, value] : m_values ) {
            m_logFile << key << ",";
        }
        for (auto [key, value] : m_inputs ) {
            m_logFile << key << ",";
        }
        m_logFile << std::endl;
        m_fileHeaderCreated = true;
    }
    m_logFile << std::to_string(Simulator::Now().GetSeconds()) + ",";
    for (auto [key, value] : m_values ) {
        m_logFile << std::to_string(value) << ",";
    }
    for (auto [key, value] : m_inputs ) {
        m_logFile << std::to_string(value) << ",";
    }
    m_logFile << std::endl;
}