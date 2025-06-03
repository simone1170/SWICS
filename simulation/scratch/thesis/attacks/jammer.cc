#include "jammer.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/mc-ue-net-device.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("Jammer");

Jammer::Jammer(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, NodeContainer *jammerContainer, Ptr<ListPositionAllocator> position, double centerFrequency, double bandwidth, double period, double dutyCycle, double jammerPower) : AttackBase(testbed, device), m_target(target), m_jammerContainer(jammerContainer), m_jammerPosition(position) {
    m_jammerContainer->Create(1);

    m_jammerMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    m_jammerMobility.SetPositionAllocator(position);
    m_jammerMobility.Install(*m_jammerContainer);

    // Automatic retrieval of MmWaveUeNetDevice failed
    Ptr<Node> node = target.GetNode();
    Ptr<NetDevice> netDevice = node->GetDevice(0);
    Ptr<mmwave::MmWaveUeNetDevice> ueDevice = DynamicCast<mmwave::MmWaveUeNetDevice>(netDevice);

    m_jammingGeneratorHelper.SetMmWaveChannel(ueDevice);
    m_jammingGeneratorHelper.SetMmWaveTxPowerSpectralDensity(centerFrequency, bandwidth, jammerPower);
    m_jammingGeneratorHelper.SetPhyAttribute("Period", TimeValue(Seconds(period)));
    m_jammingGeneratorHelper.SetPhyAttribute("DutyCycle", DoubleValue(dutyCycle));

    NetDeviceContainer jammerDevices = m_jammingGeneratorHelper.Install(*m_jammerContainer);
    
    m_jammingGenerator = jammerDevices.Get(0)->GetObject<NonCommunicatingNetDevice>()->GetPhy()->GetObject<MmWaveJammingGenerator>();
}

void Jammer::SetTarget(Ptr<NetDevice> target) {
    m_jammingGenerator->SetTarget(target);
}

void Jammer::StartAttack() {
    AttackBase::StartAttack();
    CMD_LOG_WARN("Started Jamming Attack");
    m_jammingGenerator->Start();
}

void Jammer::StopAttack() {
    AttackBase::StopAttack();
    CMD_LOG_WARN("Stopped Jamming Attack");
    m_jammingGenerator->Stop();
}