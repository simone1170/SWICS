#ifndef REACTIVE_JAMMER_H
#define REACTIVE_JAMMER_H

#include <fstream>

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/mmwave-jamming-model.h"
#include "ns3/jamming-model-helper.h"
#include "ns3/mmwave-phy-mac-common.h"
#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief MCS-aware reactive jammer.
    ///
    /// Unlike the constant/periodic Jammer, this attack observes the target UE's
    /// downlink MCS (via the PHY Rx trace) and emits a short jamming pulse only when
    /// that MCS is at or above a threshold. Corrupting those high-rate transport
    /// blocks triggers NACKs, which drive the outer-loop link adaptation to lower the
    /// MCS. Once the target is trapped in the low-MCS region (below the threshold) the
    /// jammer falls silent, making the attack energy-efficient and hard to detect.
    class ReactiveJammer : public AttackBase {
    public:
        /// \param testbed         the testbed the attack runs on
        /// \param device          the compromised device (for attack logging)
        /// \param target          the UE whose MCS is observed and jammed
        /// \param jammerContainer  node container the jammer node is created in
        /// \param position         position of the jammer node
        /// \param centerFrequency  5G center frequency (Hz)
        /// \param bandwidth        5G bandwidth (Hz)
        /// \param jammerPower      jammer power (dBm)
        /// \param mcsThreshold     pulse only when observed MCS >= this value
        /// \param pulseDuration    duration of one jamming pulse
        /// \param minGap           minimum time between consecutive pulses
        ReactiveJammer(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device,
                       IndustrialDevice &target, NodeContainer *jammerContainer,
                       Ptr<ListPositionAllocator> position, double centerFrequency,
                       double bandwidth, double jammerPower,
                       uint8_t mcsThreshold, Time pulseDuration, Time minGap);
        ~ReactiveJammer() {};

        void StartAttack();
        void StopAttack();

        /// \brief Trace sink: tracks the target UE's most recent observed MCS.
        void OnDlRx(mmwave::RxPacketTraceParams params);
        /// \brief Static trampoline so the instance method can be bound to the trace.
        static void DlRxTrampoline(Ptr<ReactiveJammer> self, mmwave::RxPacketTraceParams p);
        /// \brief Periodic reactive controller: jams the next interval iff the target's
        /// last observed MCS is at or above the threshold.
        void Tick();

    private:
        IndustrialDevice &m_target;
        NodeContainer *m_jammerContainer;
        MobilityHelper m_jammerMobility;
        Ptr<ListPositionAllocator> m_jammerPosition;

        JammingGeneratorHelper m_jammingGeneratorHelper = JammingGeneratorHelper(JammerType::MmWave);
        Ptr<MmWaveJammingGenerator> m_jammingGenerator;

        Ptr<TestbedHelper> m_tb;
        uint16_t m_targetRnti = 0;
        bool m_haveRnti = false;
        uint8_t m_mcsThreshold;
        Time m_pulseDuration;
        Time m_minGap;
        Time m_lastPulse = Seconds(-1);
        bool m_reactiveActive = false;

        // reactive controller state
        Time m_tickInterval;          ///< how often the controller re-evaluates / jams
        uint8_t m_lastMcs = 0;        ///< most recent MCS observed for the target UE
        bool m_haveMcs = false;       ///< have we observed the target at least once

        // metrics (for energy / detectability analysis)
        uint64_t m_pulses = 0;
        uint64_t m_observations = 0;
        Time m_totalJamTime = Seconds(0);
        std::ofstream m_log;

        uint16_t ResolveTargetRnti();

        virtual std::string GetAttackPoint() override { return m_target.GetIdentifier(); };
        virtual std::string GetDescription() override {
            return "Reactive MCS-aware jamming (threshold MCS=" + std::to_string(m_mcsThreshold) + ")";
        };
    };
}

#endif // REACTIVE_JAMMER_H
