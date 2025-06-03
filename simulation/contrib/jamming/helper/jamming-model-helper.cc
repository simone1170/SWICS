#include "jamming-model-helper.h"

#include "ns3/antenna-model.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-channel.h"
#include "ns3/mmwave-spectrum-phy.h"
#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/uniform-planar-array.h"
#include "ns3/mmwave-jamming-model.h"
//#include "ns3/beam-manager.h"
#include "ns3/mmwave-spectrum-value-helper.h"


namespace ns3
{

NS_LOG_COMPONENT_DEFINE("JammingGeneratorHelper");

JammingGeneratorHelper::JammingGeneratorHelper()
{
    m_phy.SetTypeId("ns3::WaveformGenerator");
    m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
    m_antenna.SetTypeId("ns3::IsotropicAntennaModel");

}

JammingGeneratorHelper::JammingGeneratorHelper(JammerType type){

    if (type == JammerType::MmWave){
        m_simType = JammerType::MmWave;
        m_phy.SetTypeId("ns3::MmWaveJammingGenerator");
        m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
        m_antenna.SetTypeId("ns3::UniformPlanarArray");
    } else if (type == JammerType::DirectedMmWave){
        m_simType = JammerType::DirectedMmWave;
        m_phy.SetTypeId("ns3::MmWaveJammingGenerator");
        m_device.SetTypeId("ns3::NonCommunicatingNetDevice");
        m_antenna.SetTypeId("ns3::UniformPlanarArray");
    }

    else
    {
        NS_ABORT_MSG_IF(true, "Unsupported Simulation type. Choose MmWave, or fallback to default Waveform Generator.");
    }
}

JammingGeneratorHelper::~JammingGeneratorHelper()
{
}

void
JammingGeneratorHelper::SetChannel(Ptr<SpectrumChannel> channel)
{
    m_channel = channel;
}

void
JammingGeneratorHelper::SetChannel(std::string channelName)
{
    Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel>(channelName);
    m_channel = channel;
}

void
JammingGeneratorHelper::SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd)
{
    NS_LOG_FUNCTION(this << txPsd);
    m_txPsd = txPsd;
}

void
JammingGeneratorHelper::SetPhyAttribute(std::string name, const AttributeValue& v)
{
    m_phy.Set(name, v);
}

void
JammingGeneratorHelper::SetDeviceAttribute(std::string name, const AttributeValue& v)
{
    m_device.Set(name, v);
}

void 
JammingGeneratorHelper::SetTarget(Ptr<NetDevice> target) {
    m_target = target;
}

NetDeviceContainer
JammingGeneratorHelper::Install(NodeContainer c) const
{
    NetDeviceContainer devices;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        if (m_simType == JammerType::MmWave)
        {
        Ptr<Node> node = *i;

        Ptr<NonCommunicatingNetDevice> dev =
            m_device.Create()->GetObject<NonCommunicatingNetDevice>();

        Ptr<MmWaveJammingGenerator> phy = m_phy.Create()->GetObject<MmWaveJammingGenerator>();

    
        NS_ASSERT(phy);

        dev->SetPhy(phy);

        NS_ASSERT(node);
        phy->SetMobility(node->GetObject<MobilityModel>());

        NS_ASSERT(dev);
        phy->SetDevice(dev);

        NS_ASSERT_MSG(m_txPsd,
                    "you forgot to call JammingGeneratorHelper::SetTxPowerSpectralDensity ()");
        phy->SetTxPowerSpectralDensity(m_txPsd);

        NS_ASSERT_MSG(m_channel, "you forgot to call JammingGeneratorHelper::SetChannel ()");
        phy->SetChannel(m_channel);
        dev->SetChannel(m_channel);

        node->AddDevice(dev);
        devices.Add(dev);

        phy->SetAntenna();
        }

    else if (m_simType == JammerType::DirectedMmWave){
        Ptr<Node> node = *i;

        Ptr<NonCommunicatingNetDevice> dev =
            m_device.Create()->GetObject<NonCommunicatingNetDevice>();

        Ptr<MmWaveJammingGenerator> phy = m_phy.Create()->GetObject<MmWaveJammingGenerator>();

    
        NS_ASSERT(phy);

        dev->SetPhy(phy);

        NS_ASSERT(node);
        phy->SetMobility(node->GetObject<MobilityModel>());

        NS_ASSERT(dev);
        phy->SetDevice(dev);

        NS_ASSERT_MSG(m_txPsd,
                    "you forgot to call JammingGeneratorHelper::SetTxPowerSpectralDensity ()");
        phy->SetTxPowerSpectralDensity(m_txPsd);

        NS_ASSERT_MSG(m_channel, "you forgot to call JammingGeneratorHelper::SetChannel ()");
        phy->SetChannel(m_channel);
        dev->SetChannel(m_channel);

        node->AddDevice(dev);
        devices.Add(dev);
        phy->SetAntenna();
        phy->SetTarget(m_target);
    }

    else
        {
        Ptr<Node> node = *i;

        Ptr<NonCommunicatingNetDevice> dev =
            m_device.Create()->GetObject<NonCommunicatingNetDevice>();

        Ptr<WaveformGenerator> phy = m_phy.Create()->GetObject<WaveformGenerator>();

        Ptr<AntennaModel> antenna = (m_antenna.Create())->GetObject<AntennaModel>();

        NS_ASSERT_MSG(antenna, "error in creating the AntennaModel object");

        phy->SetAntenna(antenna);
    
        NS_ASSERT(phy);

        dev->SetPhy(phy);

        NS_ASSERT(node);
        phy->SetMobility(node->GetObject<MobilityModel>());

        NS_ASSERT(dev);
        phy->SetDevice(dev);

        NS_ASSERT_MSG(m_txPsd,
                    "you forgot to call JammingGeneratorHelper::SetTxPowerSpectralDensity ()");
        phy->SetTxPowerSpectralDensity(m_txPsd);

        NS_ASSERT_MSG(m_channel, "you forgot to call JammingGeneratorHelper::SetChannel ()");
        phy->SetChannel(m_channel);
        dev->SetChannel(m_channel);

        node->AddDevice(dev);
        devices.Add(dev);
        }
  
    }
    return devices;
}

NetDeviceContainer
JammingGeneratorHelper::Install(Ptr<Node> node) const
{
    return Install(NodeContainer(node));
}

NetDeviceContainer
JammingGeneratorHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node);
}

void  
JammingGeneratorHelper::SetMmWaveTxPowerSpectralDensity(double frequency, double bandwitdth, double jamming_power)
{

    //mmwave GetSpectrumModel does only take MmWavePhyMacCommon as input:
    Ptr<mmwave::MmWavePhyMacCommon> config = Create<mmwave::MmWavePhyMacCommon>();
    config->SetCentreFrequency(frequency);
    config->SetNumerology(mmwave::MmWavePhyMacCommon::NrNumerology2); 
    config->SetBandwidth(bandwitdth); 

    Ptr<SpectrumModel> spectrumModel = mmwave::MmWaveSpectrumValueHelper::GetSpectrumModel(config);
    std::vector<int> activeRbs;

    for (size_t rbId = 0; rbId < spectrumModel->GetNumBands(); rbId++)
    {
        activeRbs.push_back(rbId);
    }

    m_mmWavePsd = mmwave::MmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity(config, jamming_power, activeRbs);
    
    this->SetTxPowerSpectralDensity(m_mmWavePsd);
}

void
JammingGeneratorHelper::SetMmWaveChannel(Ptr<mmwave::MmWaveUeNetDevice> ueNet)
{
    Ptr<mmwave::MmWaveUePhy> uePhy = ueNet->GetPhy(0);
    //MmWaveUePhy separates SpectrumPhy into DL and UL, which one to choose?
    Ptr<mmwave::MmWaveSpectrumPhy> UeSpecPhy = uePhy->GetDlSpectrumPhy();

    Ptr<SpectrumChannel> ueChann = UeSpecPhy->GetSpectrumChannel();

    this->SetChannel(ueChann);
}

void
JammingGeneratorHelper::SetMmWaveChannel(Ptr<mmwave::MmWaveEnbNetDevice> gnbNet)
{
    Ptr<mmwave::MmWaveEnbPhy> gnbPhy = gnbNet->GetPhy(0);
    //MmWaveEnbPhy separates SpectrumPhy into DL and UL, which one to choose?
    Ptr<mmwave::MmWaveSpectrumPhy> GnbSpecPhy = gnbPhy->GetDlSpectrumPhy();

    Ptr<SpectrumChannel> gnbChann = GnbSpecPhy->GetSpectrumChannel();

    this->SetChannel(gnbChann);
}


} // namespace ns3
