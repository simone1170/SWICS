#ifndef INJECTION_H
#define INJECTION_H

#include "../helper/command-line-helper.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a message injection attack
    class InjectionAttack : public AttackBase {
    public:
        /// \brief Instantiates a new message injection attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the target of the message injection attack
        /// \param functionCode the function code of the injected message
        /// \param message the message body of the injected message
        InjectionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> message);
        ~InjectionAttack() {};

        /// \brief Starts the message injection attack
        void StartAttack();
    private:
        IndustrialDevice &m_target;
        uint8_t m_functionCode;
        std::vector<uint8_t> m_message;

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override { return "Packet injection attack"; };
    };
}

#endif // INJECTION_H