#include "spectrum-analysis.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/mc-ue-net-device.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("SpectrumAnalysis");

SpectrumAnalysis::SpectrumAnalysis(Ptr<TestbedHelper> testbed, Ptr<mmwave::MmWaveUeNetDevice> analyzerDevice, NodeContainer *analyzerNodes, Ptr<ListPositionAllocator> position, double centerFrequency, double bandwidth) : AttackBase(testbed, nullptr), m_analyzerNodes(analyzerNodes){
        // Initialize SpectrumAnalyzer node, NetDevice and Phy
        JammingGeneratorHelper analyzerHelper(JammerType::MmWave);

        m_analyzerNodes->Create(1);
        MobilityHelper analyzerMobility;
        analyzerMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        analyzerMobility.SetPositionAllocator(position);
        analyzerMobility.Install(*m_analyzerNodes);
        analyzerHelper.SetMmWaveChannel(analyzerDevice);
        //Not interested in jamming, just device needed
        analyzerHelper.SetMmWaveTxPowerSpectralDensity(centerFrequency, bandwidth, 0);
        analyzerHelper.Install(*m_analyzerNodes);

        SpectrumAnalyzerHelper analyzer = SpectrumAnalyzerHelper();
        Ptr<mmwave::MmWaveUePhy> analyzerPhy = analyzerDevice->GetPhy(0);
        Ptr<mmwave::MmWaveSpectrumPhy> UeSpecPhy = analyzerPhy->GetDlSpectrumPhy();
        Ptr<SpectrumChannel> ueChann = UeSpecPhy->GetSpectrumChannel();
        analyzer.SetChannel(ueChann);
        Ptr<mmwave::MmWavePhyMacCommon> config = Create<mmwave::MmWavePhyMacCommon>();
        config->SetCentreFrequency(centerFrequency);
        config->SetNumerology(mmwave::MmWavePhyMacCommon::NrNumerology2); 
        config->SetBandwidth(bandwidth); 
        Ptr<SpectrumModel> spectrumModel = mmwave::MmWaveSpectrumValueHelper::GetSpectrumModel(config);
        analyzer.SetRxSpectrumModel(spectrumModel);
        analyzer.EnableAsciiAll(testbed->GetLogPath());
        analyzer.SetPhyAttribute("Resolution", TimeValue(MicroSeconds(500)));
        //analyzer.SetAntenna("ns3::UniformPlanarArray", "NumColumns", UintegerValue(1), "NumRows", UintegerValue(1));
        NetDeviceContainer analyzerDevices = analyzer.Install(*m_analyzerNodes);
        m_spectrumAnalyzer = DynamicCast<SpectrumAnalyzer>(DynamicCast<NonCommunicatingNetDevice>(analyzerDevices.Get(0))->GetPhy());

}


void SpectrumAnalysis::StartAttack() {
    // No need to call attacked device's hooks
    // AttackBase::StartAttack(); 
    CMD_LOG_WARN("Started Spectrum Analysis Attack");
    m_spectrumAnalyzer->Start();
}

void SpectrumAnalysis::StopAttack() {
    // AttackBase::StopAttack();
    CMD_LOG_WARN("Stopped Spectrum Analysis Attack");
    m_spectrumAnalyzer->Stop();
}

void SpectrumAnalysis::SetTarget(Ptr<NetDevice> target) {
    m_spectrumAnalyzer->SetTarget(target);
}