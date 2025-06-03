#ifndef SENSOR_PROPERTY_H
#define SENSOR_PROPERTY_H

#include <string>

namespace testbed {
    /// \brief A property (register) the sensor senses from the physical system
    class SensorProperty {
    public:
        /// \brief Instantiates a sensor property
        /// \param key Key to identify the property of the physical system that the sensor senses
        /// \param minValue Min value of the sensor
        /// \param maxValue Max value of the sensor
        /// \param fault Fault of the sensor in percent
        SensorProperty(std::string key, double minValue, double maxValue, double fault) : m_key(key), m_minValue(minValue), m_maxValue(maxValue), m_fault(fault) {};
        virtual ~SensorProperty() {};

        friend std::ostream& operator<<(std::ostream& os, const SensorProperty& sp) {
            os << "{" << sp.m_key << ": (min: " << sp.m_minValue << ", max: " << sp.m_maxValue << ", fault: " << sp.m_fault << ")}";
            return os;
        }

        /// \brief Method to retrieve the property's key
        /// \return the property's key
        std::string GetKey() { return m_key; };

        /// \brief Method to retrieve the min value of the property
        /// \return the min value of the property
        double GetMinValue() { return m_minValue; };

        /// \brief Method to retrieve the max value of the property
        /// \return the max value of the property
        double GetMaxValue() { return m_maxValue; };

        /// \brief Method to retrieve the fault percentage of the property (sensor)
        /// \return the fault percentage of the sensor for this property
        double GetFault() { return m_fault; };
    private:
        std::string m_key;
        double m_minValue;
        double m_maxValue;
        double m_fault;
    };
}

#endif // SENSOR_PROPERTY_H