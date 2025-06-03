#ifndef PGW_FLOODING_H
#define PGW_FLOODING_H

#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a message suppression attack. Messages from the device to the target will be dropped.
    class PGWFloodingAttack : public AttackBase {
    public:
        /// \brief Instantiates a new suppression attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the target of the attack
        PGWFloodingAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, Ipv4Address pgwAddress, uint32_t packetSize);
        ~PGWFloodingAttack() {};

        /// \brief Starts the P-GW-C flooding attack
        void StartAttack();
    private:
        Ipv4Address m_pgwAddress;
        Ptr<Socket> m_socket;
        uint32_t m_packetSize;

        void RunAttack();

        virtual std::string GetAttackPoint() override { return "P-GW-C"; };
        virtual std::string GetDescription() override { return "P-GW-C flooding attack"; };
    };
}

#endif // PGW_FLOODING_H