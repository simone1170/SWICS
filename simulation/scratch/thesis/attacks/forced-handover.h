#ifndef FORCED_HANDOVER_H
#define FORCED_HANDOVER_H

#include "../helper/command-line-helper.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Scaffolding of a Forced Handover attack (only used for logging purposes)
    class ForcedHandover : public AttackBase {
    public:
        /// \brief Instantiates a new Forced Handover attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device (irrelevant in this scenario)
        ForcedHandover(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device) : AttackBase(testbed, device) {};
        ~ForcedHandover() {};
    private:

        virtual std::string GetAttackPoint() override { return "Forced handover"; };
        virtual std::string GetDescription() override { return "Forced handover"; };
    };
}

#endif // FORCED_HANDOVER_H