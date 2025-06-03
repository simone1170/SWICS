#ifndef MMWAVE_JAMMING_GENERATOR_H
#define MMWAVE_JAMMING_GENERATOR_H

#include <ns3/event-id.h>
#include <ns3/mobility-model.h>
#include <ns3/net-device.h>
#include <ns3/nstime.h>
#include <ns3/packet.h>
#include <ns3/spectrum-channel.h>
#include <ns3/spectrum-phy.h>
#include "ns3/mmwave-spectrum-phy.h"
#include <ns3/spectrum-value.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/uniform-planar-array.h>

namespace ns3
{

// Adaption of the NR & LTE jammer for the mmWave 5G module.
// Including the mmWave capability into the original jammer implementation
// is not possible as the NR and mmWave require different versions 
// of ns3. We additionally included our AttackLogger to facilitate 
// evaluation.
// The generated waveforms have a given Spectrum Power Density and
// duration (set with the SetResolution()) . The generator activates
// and deactivates periodically with a given period and with a duty
// cycle.

// For the PHY model we are using a ThreeGppAntennaArrayModel.

class MmWaveJammingGenerator : public mmwave::MmWaveSpectrumPhy
{
  public:
    MmWaveJammingGenerator();
    ~MmWaveJammingGenerator() override;

    // Get the type ID.

    static TypeId GetTypeId();

    // inherited from SpectrumPhy
    void SetChannel(Ptr<SpectrumChannel> c) override;
    void SetMobility(Ptr<MobilityModel> m) override;
    void SetDevice(Ptr<NetDevice> d) override;
    virtual Ptr<MobilityModel> GetMobility() const override;
    Ptr<NetDevice> GetDevice() const override;
    Ptr<const SpectrumModel> GetRxSpectrumModel() const override;
    Ptr<Object> GetAntenna() const override; //removed override, as mmwave only conatains GetRxAntenna()
    void StartRx(Ptr<SpectrumSignalParameters> params) override;

    // Set the Power Spectral Density used for outgoing waveforms

    void SetTxPowerSpectralDensity(Ptr<SpectrumValue> txs);

    // Set the period according to which the JammingGenerator switches
    // on and off.

    void SetPeriod(Time period);

    // Get the value of the period according to which the JammingGenerator switches
    // on and off.

    Time GetPeriod() const;

    //Set the value the value of the duty cycle

    void SetDutyCycle(double value);

    // Get the value of the duty cycle

    double GetDutyCycle() const;

    // Configure the ThreeGppAntennaArrayModel, the beamforming and set the antenna.

    void SetAntenna();

    void SetTarget(Ptr<NetDevice> target);

    // Start the waveform generator, register as start of attack.

    virtual void Start();


    // Stop the waveform generator, register end of waveform as stop of attack.

    virtual void Stop();

    // Only send jamming signal once

    virtual void GenerateSingleWaveform(Time duration);

  private:
    void DoDispose() override;

    Ptr<MobilityModel> m_mobility;  //!< Mobility model
    Ptr<UniformPlanarArray> m_antenna;         //!< Changed because mmWave always uses ThreeGppAntennaArrayModel
    Ptr<NetDevice> m_netDevice;     //!< Owning NetDevice
    Ptr<SpectrumChannel> m_channel; //!< Channel
    Ptr<NetDevice> m_target; //! Target for jamming attack, if directional jammer is used antenna array model is oriented at target

    // Generates a waveform

    virtual void GenerateWaveform();

    Ptr<SpectrumValue> m_txPowerSpectralDensity; //!< Tx PSD
    Time m_period;                               //!< Period
    double m_dutyCycle;                          //!< Duty Cycle (should be in [0,1])
    Time m_startTime;                            //!< Start time
    Time m_startNextCycle;                       //!< Start time of next cycle, used to add stopAttack event at right time
    EventId m_nextWave;                          //!< Next waveform generation event

    ObjectFactory m_bfModelFactory; //! ObjectFactory used generate beamforming model for directional jamming

    TracedCallback<Ptr<const Packet>> m_phyTxStartTrace; //!< TracedCallback: Tx start
    TracedCallback<Ptr<const Packet>> m_phyTxEndTrace;   //!< TracedCallback: Tx end
};

} // namespace ns3

#endif /* MMWAVE_JAMMING_GENERATOR_H */
