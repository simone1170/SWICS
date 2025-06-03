#ifndef SPECTRUMANALYSIS_H
#define SPECTRUMANALYSIS_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/mmwave-jamming-model.h"
#include "ns3/jamming-model-helper.h"
#include "attack-base.h"
#include "ns3/spectrum-analyzer-helper.h"
#include "ns3/spectrum-analyzer.h"
#include "ns3/non-communicating-net-device.h"


using namespace ns3;

namespace testbed {
    /// \brief Implementation of a 5G Jammer attack
    class SpectrumAnalysis : public AttackBase {
    public:
        /// \brief Instantiates a new Jammer attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param analyzerDevice the device from which to extract the channel model
        /// \param target the attack target
        /// \param jammerContainer jammer container
        /// \param position position of the jammer
        /// \param centerFrequency center frequency of the 5G setup
        /// \param bandwidth bandwidth of the 5G setup
        /// \param period period of jamming
        /// \param dutyCycle portion of active jamming in period
        /// \param jammerPower power of Jammer in dBm
        SpectrumAnalysis(Ptr<TestbedHelper> testbed, Ptr<mmwave::MmWaveUeNetDevice> analyzerDevice, NodeContainer *analyzerNodes, Ptr<ListPositionAllocator> position, double centerFrequency, double bandwidth);
        ~SpectrumAnalysis() {};

        /// \brief Starts the attack
        void StartAttack();

        /// \brief Stops the attack
        void StopAttack();

        void SetTarget(Ptr<NetDevice> target);
    private:
        Ptr<SpectrumAnalyzer> m_spectrumAnalyzer;
        NodeContainer *m_analyzerNodes;

        virtual std::string GetAttackPoint() override { return "Wireless Medium";};
        virtual std::string GetDescription() override { 
            Vector mobility = m_analyzerNodes->Get(0)->GetObject<MobilityModel>()->GetPosition();
            return "Jamming attack at position (" + std::to_string(mobility.x) + ", " + std::to_string(mobility.y) + ", " +  
            std::to_string(mobility.z) + ")"; 
        };
    };
}

#endif