#include "mmwave-jamming-model.h"

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/object-factory.h>
#include <ns3/packet-burst.h>
#include <ns3/simulator.h>
#include "ns3/pointer.h"
#include "ns3/mmwave-spectrum-signal-parameters.h"
#include "ns3/nyu-propagation-loss-model.h"
#include "ns3/nyu-spectrum-propagation-loss-model.h"
#include "ns3/isotropic-antenna-model.h"
#include "ns3/uniform-planar-array.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MmWaveJammingGenerator");

NS_OBJECT_ENSURE_REGISTERED(MmWaveJammingGenerator);

MmWaveJammingGenerator::MmWaveJammingGenerator()
    : m_mobility(nullptr),
      m_netDevice(nullptr),
      m_channel(nullptr),
      m_txPowerSpectralDensity(nullptr),
      m_startTime(Seconds(0))
{
}

MmWaveJammingGenerator::~MmWaveJammingGenerator()
{
}

void
MmWaveJammingGenerator::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_channel = nullptr;
    m_netDevice = nullptr;
    m_mobility = nullptr;
    if (m_nextWave.IsRunning())
    {
        m_nextWave.Cancel();
    }
}

TypeId
MmWaveJammingGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MmWaveJammingGenerator")
            .SetParent<SpectrumPhy>()
            .SetGroupName("Spectrum")
            .AddConstructor<MmWaveJammingGenerator>()
            .AddAttribute(
                "Period",
                "the period (=1/frequency)",
                TimeValue(Seconds(1.0)),
                MakeTimeAccessor(&MmWaveJammingGenerator::SetPeriod, &MmWaveJammingGenerator::GetPeriod),
                MakeTimeChecker())
            .AddAttribute("DutyCycle",
                          "the duty cycle of the generator, i.e., the fraction of the period that "
                          "is occupied by a signal",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&MmWaveJammingGenerator::SetDutyCycle,
                                             &MmWaveJammingGenerator::GetDutyCycle),
                          MakeDoubleChecker<double>())
            .AddTraceSource("TxStart",
                            "Trace fired when a new transmission is started",
                            MakeTraceSourceAccessor(&MmWaveJammingGenerator::m_phyTxStartTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxEnd",
                            "Trace fired when a previously started transmission is finished",
                            MakeTraceSourceAccessor(&MmWaveJammingGenerator::m_phyTxEndTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

Ptr<NetDevice>
MmWaveJammingGenerator::GetDevice() const
{
    return m_netDevice;
}

Ptr<MobilityModel>
MmWaveJammingGenerator::GetMobility() const
{
    return m_mobility;
}

Ptr<const SpectrumModel>
MmWaveJammingGenerator::GetRxSpectrumModel() const
{
    // this device is not interested in RX
    return nullptr;
}

void
MmWaveJammingGenerator::SetDevice(Ptr<NetDevice> d)
{
    m_netDevice = d;
}

void
MmWaveJammingGenerator::SetMobility(Ptr<MobilityModel> m)
{
    m_mobility = m;
}

Ptr<Object>
MmWaveJammingGenerator::GetAntenna() const
{
    return m_antenna;
}


void
MmWaveJammingGenerator::SetChannel(Ptr<SpectrumChannel> c)
{
    NS_LOG_FUNCTION_NOARGS();
    m_channel = c;
}

void
MmWaveJammingGenerator::StartRx(Ptr<SpectrumSignalParameters> params)
{
    NS_LOG_FUNCTION(this << params);
}

void
MmWaveJammingGenerator::SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd)
{
    NS_LOG_FUNCTION(this << *txPsd);
    m_txPowerSpectralDensity = txPsd;
}

void
MmWaveJammingGenerator::SetPeriod(Time period)
{
    m_period = period;
}

Time
MmWaveJammingGenerator::GetPeriod() const
{
    return m_period;
}

void
MmWaveJammingGenerator::SetDutyCycle(double dutyCycle)
{
    m_dutyCycle = dutyCycle;
}

double
MmWaveJammingGenerator::GetDutyCycle() const
{
    return m_dutyCycle;
}

void 
MmWaveJammingGenerator::SetAntenna()
{
    m_antenna = Create<UniformPlanarArray>();
    m_antenna->SetAntennaElement(PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Create a complex number 1 + 0i
    std::complex<double> complexValue(1.0, 0.0);
    std::vector<std::complex<double>> complexVector = {complexValue};
    // Create a ComplexVector and initialize it with the complexValue
    UniformPlanarArray::ComplexVector myVector(complexVector);
    m_antenna->SetBeamformingVector(myVector);

    Ptr<NYUSpectrumPropagationLossModel> lossModel = DynamicCast<NYUSpectrumPropagationLossModel>(m_channel->GetSpectrumPropagationLossModel());
    // lossModel->AddDevice(m_netDevice, DynamicCast<ThreeGppAntennaArrayModel> (m_antenna));
    }

void 
MmWaveJammingGenerator::SetTarget(Ptr<NetDevice> target) {
    // Directional i.e. targeted jamming requires beamforming with larger arrays

    m_antenna->SetNumColumns(16);
    m_antenna->SetNumRows(16);
    
    m_target = target;
    m_bfModelFactory.SetTypeId(mmwave::MmWaveSvdBeamforming::GetTypeId ());
    m_bfModelFactory = ObjectFactory ("ns3::MmWaveSvdBeamforming");
    Ptr<mmwave::MmWaveBeamformingModel> bfModel = m_bfModelFactory.Create<mmwave::MmWaveBeamformingModel> ();
    bfModel->SetDevice(m_netDevice);
    bfModel->SetAntenna(m_antenna);

    Ptr<NYUSpectrumPropagationLossModel> lossModel = DynamicCast<NYUSpectrumPropagationLossModel>(m_channel->GetPhasedArraySpectrumPropagationLossModel());
    Ptr<MatrixBasedChannelModel> channelModel = lossModel->GetChannelModel();
    bfModel->SetAttribute("ChannelModel", PointerValue(channelModel));

    SetBeamformingModel(bfModel);
    ConfigureBeamforming(m_target);  
}

void
MmWaveJammingGenerator::GenerateWaveform()
{
    NS_LOG_FUNCTION(this);

    //Ptr<mmwave::MmwaveSpectrumSignalParametersDataFrame> txParams = Create<mmwave::MmwaveSpectrumSignalParametersDataFrame>();
    Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();

    txParams->duration = Time(m_period.GetTimeStep() * m_dutyCycle);
    txParams->psd = m_txPowerSpectralDensity;
    txParams->txPhy = GetObject<SpectrumPhy>();
    

    NS_LOG_LOGIC("generating waveform : " << *m_txPowerSpectralDensity);
    m_phyTxStartTrace(nullptr);
    m_channel->StartTx(txParams);

    NS_LOG_LOGIC("scheduling next waveform");
    m_startNextCycle += m_period;
    m_nextWave = Simulator::Schedule(m_period, &MmWaveJammingGenerator::GenerateWaveform, this);
}

void
MmWaveJammingGenerator::GenerateSingleWaveform(Time duration)
{
    NS_LOG_FUNCTION(this);

    //Ptr<mmwave::MmwaveSpectrumSignalParametersDataFrame> txParams = Create<mmwave::MmwaveSpectrumSignalParametersDataFrame>();
    Ptr<SpectrumSignalParameters> txParams = Create<SpectrumSignalParameters>();

    txParams->duration = duration;
    txParams->psd = m_txPowerSpectralDensity;
    txParams->txPhy = GetObject<SpectrumPhy>();
    

    NS_LOG_LOGIC("generating waveform : " << *m_txPowerSpectralDensity);
    m_phyTxStartTrace(nullptr);
    m_channel->StartTx(txParams);
}

void
MmWaveJammingGenerator::Start()
{
    NS_LOG_FUNCTION(this);
    if (!m_nextWave.IsRunning())
    {
        NS_LOG_LOGIC("generator was not active, now starting");
        m_startTime = Now();
        m_startNextCycle = m_startTime;
        m_nextWave = Simulator::ScheduleNow(&MmWaveJammingGenerator::GenerateWaveform, this);
    }
}

void
MmWaveJammingGenerator::Stop()
{
    NS_LOG_FUNCTION(this);
    if (m_nextWave.IsRunning())
    {
        m_nextWave.Cancel();
    }
}
} // namespace ns3
