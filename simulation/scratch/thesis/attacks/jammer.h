#ifndef JAMMER_H
#define JAMMER_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/mmwave-jamming-model.h"
#include "ns3/jamming-model-helper.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a 5G Jammer attack
    class Jammer : public AttackBase {
    public:
        /// \brief Instantiates a new Jammer attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param target the attack target
        /// \param jammerContainer jammer container
        /// \param position position of the jammer
        /// \param centerFrequency center frequency of the 5G setup
        /// \param bandwidth bandwidth of the 5G setup
        /// \param period period of jamming
        /// \param dutyCycle portion of active jamming in period
        /// \param jammerPower power of Jammer in dBm
        Jammer(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, NodeContainer *jammerContainer, Ptr<ListPositionAllocator> position, double centerFrequency, double bandwidth, double period, double dutyCycle, double jammerPower);
        ~Jammer() {};

        /// \brief Starts the attack
        void StartAttack();

        /// \brief Stops the attack
        void StopAttack();

        void SetTarget(Ptr<NetDevice> target);
    private:
        IndustrialDevice &m_target;
        NodeContainer *m_jammerContainer;
        // NetDeviceContainer *m_jammerDevices;
        MobilityHelper m_jammerMobility;
        Ptr<ListPositionAllocator> m_jammerPosition;

        JammingGeneratorHelper m_jammingGeneratorHelper = JammingGeneratorHelper(JammerType::MmWave);
        Ptr<MmWaveJammingGenerator> m_jammingGenerator;

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override { 
            Vector mobility = m_jammerContainer->Get(0)->GetObject<MobilityModel>()->GetPosition();
            return "Jamming attack at position (" + std::to_string(mobility.x) + ", " + std::to_string(mobility.y) + ", " +  
            std::to_string(mobility.z) + ")"; 
        };
    };
}

#endif // JAMMER_H