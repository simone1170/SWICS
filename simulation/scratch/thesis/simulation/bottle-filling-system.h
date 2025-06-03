#ifndef BOTTLE_FILLING_SYSTEM_H
#define BOTTLE_FILLING_SYSTEM_H

#include "../model/physical-system.h"

using namespace testbed;

/// \brief PhysicalSystem implementation for a bottle filling plant
class BottleFillingSystem : public PhysicalSystem {
public:
    /// \brief Instantiates a BottleFillingSystem
    /// \param testbed the TestbedHelper of this testbed to access testbed configuration
    BottleFillingSystem(Ptr<TestbedHelper> testbed);
    virtual ~BottleFillingSystem();
private:
    /// \brief Value used to store bottle fill level when bottle is not under filler (sensor senses 0)
    double m_previousBottleFillLevel = -1;
};

#endif // BOTTLE_FILLING_SYSTEM_H