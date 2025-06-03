#ifndef PHYSICAL_SYSTEM_H
#define PHYSICAL_SYSTEM_H

#include "ns3/simulator.h"
#include "ns3/object.h"
#include "ns3/nstime.h"

#include "../helper/config-helper.h"
#include "../helper/testbed-helper.h"

using namespace ns3;

namespace testbed {
    /// \brief Abstract class simulating physical properties of the testbed
    class PhysicalSystem : public Object {
    public:
        /// \brief Instantiates a PhysicalSystem
        /// \param testbed the TestbedHelper of this testbed to access testbed configuration
        PhysicalSystem(Ptr<TestbedHelper> testbed);
        virtual ~PhysicalSystem();

        /// \brief Update values in physical state
        /// \param dt timedelta since last simulation took place
        void Update(Time dt);

        /// \brief Get current values of physical properties
        /// \param key identifier of the physical property
        double GetValue(std::string key) const;

        /// \brief Method to set influence on physical property over time
        /// \param key identifier of the physical property
        /// \param input value of the physical property
        void SetActuatorInput(std::string key, double input);

        /// \brief Retrieve current input value of an actuator
        /// \param key identifier of the physical property
        /// \return value of the property
        double GetActuatorInput(std::string key);

    protected:
        /// \brief Method registering a new physical property
        /// \param key identifier of this property
        /// \param initialValue initial value of the property
        /// \param updateValue function to be called when the values needs updating (input: timedelta in seconds, output: new value of physical property)
        void RegisterValue(std::string key, double initialValue, std::function<double(double)> updateValue);

        /// \brief Set the value of physical property
        /// \param key identifier of the property
        /// \param value value of the property
        void SetValue(std::string key, double value);

    private:
        std::unordered_map<std::string, double> m_values;
        std::unordered_map<std::string, std::function<double(double)>> m_valuesUpdateFunction;
        std::unordered_map<std::string, double> m_inputs;

        Ptr<TestbedHelper> m_testbed;

        uint16_t m_cycleTime = config::physical_system_cycle_time_ms;
        EventId m_cycleEvent;

        void Cycle();

        void LogState();
        std::ofstream m_logFile;
        bool m_fileHeaderCreated = false;
    };
}

#endif // PHYSICAL_SYSTEM_H