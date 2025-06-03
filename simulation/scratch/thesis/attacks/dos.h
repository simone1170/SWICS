#ifndef DOS_H
#define DOS_H

#include "../helper/command-line-helper.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a Denial of Service attack
    class DoS : public AttackBase {
    public:
        /// \brief Instantiates a new Denial of Service attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the attack target
        /// \param functionCode the function code to use for DoS messages
        /// \param message the message body to use for DoS messages
        DoS(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> message);
        ~DoS() {};

        /// \brief Starts the Denial of Service attack
        void StartAttack();
    private:
        IndustrialDevice &m_target;
        uint8_t m_functionCode;
        std::vector<uint8_t> m_message;
        
        void RunAttack();

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override { return "DoS attack"; };
    };
}

#endif // DOS_H