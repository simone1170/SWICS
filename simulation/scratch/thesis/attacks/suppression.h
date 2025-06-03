#ifndef SUPPRESSION_H
#define SUPPRESSION_H

#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a message suppression attack. Messages from the device to the target will be dropped.
    class SuppressionAttack : public AttackBase {
    public:
        /// \brief Instantiates a new suppression attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the target of the attack
        SuppressionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target);
        ~SuppressionAttack() {};

        double SendModbusRequestHook(IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) override;
    private:
        IndustrialDevice &m_target;

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override { return "Suppression attack"; };
    };
}

#endif // SUPPRESSION_H