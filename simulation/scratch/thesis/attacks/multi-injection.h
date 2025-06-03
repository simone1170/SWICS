#ifndef MULTI_INJECTION_H
#define MULTI_INJECTION_H

#include "../helper/command-line-helper.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a message injection attack
    class MultiInjectionAttack : public AttackBase {
    public:
        /// \brief Instantiates a new message injection attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the target of the message injection attack
        /// \param functionCode the function code of the injected message
        /// \param message the message body of the injected message
        MultiInjectionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> messageA, std::vector<uint8_t> m_messageB);
        ~MultiInjectionAttack() {};

        /// \brief Starts the message injection attack
        void StartAttack();
    private:
        Ptr<TestbedHelper> m_testbed;
        IndustrialDevice &m_target;
        uint8_t m_functionCode;
        std::vector<uint8_t> m_messageA;
        std::vector<uint8_t> m_messageB;
        bool m_scheduledEnd = false;
        uint32_t m_packetNum = 0; //Number of packet in current burst

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override { return "Multiple packet injection attack"; };
    };
}

#endif // INJECTION_H