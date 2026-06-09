#include <cmath>

#include "reactive-jammer.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/mc-ue-net-device.h"
#include "ns3/mmwave-ue-net-device.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/config.h"

#include "../helper/command-line-helper.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("ReactiveJammer");

ReactiveJammer::ReactiveJammer(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device,
                               IndustrialDevice &target, NodeContainer *jammerContainer,
                               Ptr<ListPositionAllocator> position, double centerFrequency,
                               double bandwidth, double jammerPower,
                               uint8_t mcsThreshold, Time pulseDuration, Time minGap)
    : AttackBase(testbed, device), m_target(target), m_jammerContainer(jammerContainer),
      m_jammerPosition(position), m_tb(testbed), m_mcsThreshold(mcsThreshold),
      m_pulseDuration(pulseDuration), m_minGap(minGap), m_tickInterval(pulseDuration) {
    m_jammerContainer->Create(1);

    m_jammerMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    m_jammerMobility.SetPositionAllocator(position);
    m_jammerMobility.Install(*m_jammerContainer);

    // Attach the jamming generator to the same mmWave channel as the target UE.
    Ptr<Node> node = target.GetNode();
    Ptr<NetDevice> netDevice = node->GetDevice(0);
    Ptr<mmwave::MmWaveUeNetDevice> ueDevice = DynamicCast<mmwave::MmWaveUeNetDevice>(netDevice);

    m_jammingGeneratorHelper.SetMmWaveChannel(ueDevice);
    m_jammingGeneratorHelper.SetMmWaveTxPowerSpectralDensity(centerFrequency, bandwidth, jammerPower);
    // Period/DutyCycle are unused for reactive pulsing (we drive GenerateSingleWaveform
    // directly), but the generator requires them to be set.
    m_jammingGeneratorHelper.SetPhyAttribute("Period", TimeValue(Seconds(1)));
    m_jammingGeneratorHelper.SetPhyAttribute("DutyCycle", DoubleValue(1.0));

    NetDeviceContainer jammerDevices = m_jammingGeneratorHelper.Install(*m_jammerContainer);
    m_jammingGenerator = jammerDevices.Get(0)
                             ->GetObject<NonCommunicatingNetDevice>()
                             ->GetPhy()
                             ->GetObject<MmWaveJammingGenerator>();
}

uint16_t
ReactiveJammer::ResolveTargetRnti() {
    Ptr<Node> node = m_target.GetNode();
    Ptr<mmwave::MmWaveUeNetDevice> ue = DynamicCast<mmwave::MmWaveUeNetDevice>(node->GetDevice(0));
    if (ue && ue->GetRrc()) {
        return ue->GetRrc()->GetRnti();
    }
    return 0;
}

void
ReactiveJammer::DlRxTrampoline(Ptr<ReactiveJammer> self, mmwave::RxPacketTraceParams p) {
    self->OnDlRx(p);
}

void
ReactiveJammer::StartAttack() {
    AttackBase::StartAttack();
    m_reactiveActive = true;
    m_targetRnti = ResolveTargetRnti();
    m_haveRnti = (m_targetRnti != 0);

    if (!m_log.is_open()) {
        m_log.open(m_tb->GetLogPath() + "reactive-jammer-log.csv");
        m_log << "time,targetRnti,observedMcs,corrupt,sinrDb,pulse" << std::endl;
    }

    // The DL Rx trace fires once per received transport block; our callback is the
    // reactive controller. Connecting an additional callback alongside the existing
    // PHY trace is fine (multiple sinks per trace source are supported).
    Config::ConnectWithoutContextFailSafe(
        "/NodeList/*/DeviceList/*/ComponentCarrierMap/*/MmWaveUePhy/DlSpectrumPhy/RxPacketTraceUe",
        MakeBoundCallback(&ReactiveJammer::DlRxTrampoline, Ptr<ReactiveJammer>(this)));

    // Kick off the periodic reactive controller.
    Simulator::Schedule(m_tickInterval, &ReactiveJammer::Tick, this);

    CMD_LOG_WARN("Started Reactive Jamming Attack (target rnti=" << m_targetRnti
                 << ", MCS threshold=" << +m_mcsThreshold << ")");
}

void
ReactiveJammer::OnDlRx(mmwave::RxPacketTraceParams p) {
    if (!m_reactiveActive) {
        return;
    }
    // Focus on the target UE (if its RNTI is known). If we could not resolve the RNTI
    // we fall back to observing any UE's reception.
    if (m_haveRnti && p.m_rnti != m_targetRnti) {
        return;
    }
    m_observations++;
    m_lastMcs = p.m_mcs;
    m_haveMcs = true;

    // Pure observation row (pulse decisions are made in Tick()).
    m_log << Simulator::Now().GetSeconds() << "," << p.m_rnti << "," << +p.m_mcs << ","
          << p.m_corrupt << "," << 10 * std::log10(p.m_sinr) << ",0" << std::endl;
}

void
ReactiveJammer::Tick() {
    if (!m_reactiveActive) {
        return;
    }
    // Reactive rule: keep jamming the channel (contiguously, tick by tick) as long as
    // the target's last observed MCS is at or above the threshold. Once the link
    // adaptation has been driven below the threshold, fall silent — this is what makes
    // the attack energy-efficient and low-duty-cycle compared with a constant jammer.
    if (m_haveMcs && m_lastMcs >= m_mcsThreshold) {
        m_jammingGenerator->GenerateSingleWaveform(m_tickInterval);
        m_lastPulse = Simulator::Now();
        m_pulses++;
        m_totalJamTime += m_tickInterval;
        m_log << Simulator::Now().GetSeconds() << ",-1," << +m_lastMcs << ",,,1" << std::endl;
    }

    Simulator::Schedule(m_tickInterval, &ReactiveJammer::Tick, this);
}

void
ReactiveJammer::StopAttack() {
    AttackBase::StopAttack();
    m_reactiveActive = false;
    CMD_LOG_WARN("Stopped Reactive Jamming Attack. pulses=" << m_pulses
                 << " jamTime=" << m_totalJamTime.GetSeconds() << "s observations=" << m_observations);
    if (m_log.is_open()) {
        m_log << "# summary pulses=" << m_pulses
              << " totalJamTimeS=" << m_totalJamTime.GetSeconds()
              << " observations=" << m_observations << std::endl;
        m_log.flush();
    }
}
