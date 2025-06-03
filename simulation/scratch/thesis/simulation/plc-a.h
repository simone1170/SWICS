#ifndef PLC_A_H
#define PLC_A_H

#include "../model/plc.h"
#include "../model/sensor.h"
#include "../model/actuator.h"

using namespace testbed;

/// \brief PLC to control the water tank in- and out-flow.
///
/// Requires 'tankWaterLevel', 'bottleFillLevel', 'bottlePosition' sensors and 'tankWaterFlowIn', 'tankWaterFlowOut' actuators.
class PLC_A : public PLC {
public:
    PLC_A(std::unordered_map<std::string, Ptr<Sensor>> sensors, std::unordered_map<std::string, Ptr<Actuator>> actuators);
    virtual ~PLC_A();

private:
    virtual void RunControlLogic() override;
    virtual void ControlActuators() override;
    virtual void RequestSensorData() override;

    // Helper variables for logging / bottle count
    bool m_flowIn = false;
    bool m_flowOut = false;
    uint16_t m_fillCount = 0;
};

#endif