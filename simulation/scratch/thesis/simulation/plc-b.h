#ifndef PLC_B_H
#define PLC_B_H

#include "../model/plc.h"
#include "../model/sensor.h"
#include "../model/actuator.h"

using namespace testbed;

/// \brief PLC to control the conveyor belt moving water bottles to the filler.
///
/// Requires 'bottleFillLevel', 'bottlePosition' sensors and 'motorController' actuator.
class PLC_B : public PLC {
public:
    PLC_B(std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators);
    virtual ~PLC_B();
private:
    virtual void RunControlLogic() override;
    virtual void ControlActuators() override;
    virtual void RequestSensorData() override;

    double m_previousBottleLevel = 0;

    // Helper variables for logging / bottle count
    bool m_moving;
    uint16_t m_bottleCount = 0;
    bool m_prevHmiHalt = false;
};

#endif // PLC_B_H