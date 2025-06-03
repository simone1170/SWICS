#ifndef JAMMING_GENERATOR_HELPER_H
#define JAMMING_GENERATOR_HELPER_H

// This is a helper class to initialize and configure the jamming classes.

#include "ns3/attribute.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/antenna-model.h"
// #include "ns3/nr-jamming-model.h"
#include "ns3/mmwave-spectrum-value-helper.h"
#include "ns3/waveform-generator.h"
#include "ns3/mmwave-module.h"
#include "ns3/net-device.h"

#include <string>

namespace ns3
{

// enum class to distinguish what network is being jammed. With ns-O-RAN we need to use the 
// mmWave 5G module. WG stands for WaveformGenerator. In this case, the helper configures
// a simple WaveformGenerator.
enum class JammerType {WG, MmWave, DirectedMmWave};

class SpectrumValue;
class SpectrumChannel;
class MmWaveJammingGenerator;


// Create a Jamming Generator Helper, which can be used to configure a jammer
// which injects specific noise in the channel.
 
class JammingGeneratorHelper
{
  public:
    JammingGeneratorHelper();
    JammingGeneratorHelper(JammerType type);
    ~JammingGeneratorHelper();


// set the SpectrumChannel passed as a parameter that will be used 
// by SpectrumPhy instances created by this helper

    void SetChannel(Ptr<SpectrumChannel> channel);

// set the SpectrumChannel passed as a parameter that will be used 
// by SpectrumPhy instances created by this helper

    void SetChannel(std::string channelName);


// set the Power Spectral Density to be used for transmission by all created PHY
// instances

    void SetTxPowerSpectralDensity(Ptr<SpectrumValue> txPsd);

// set the Power Spectral Density to be used for transmission. Helper method specific to 
// MmWave to facilitate the configuration of the jammer.

    void SetMmWaveTxPowerSpectralDensity(double frequency, double bandwitdth, double jamming_power);

// set the SpectrumChannel passed as a parameter that will be used 
// by MmWaveSpectrumPhy instances created by this helper. 

// Jammer will have to transmit on the same channel as the legitimate devices.
// The methods extract the channel either from the Gnb or the UE, and pass it to the 
// Jammer. We always extract the DL channel, although this does not make a difference

    void SetMmWaveChannel(Ptr<mmwave::MmWaveUeNetDevice> ueNet);

    void SetMmWaveChannel(Ptr<mmwave::MmWaveEnbNetDevice> gnbNet);


// Set the name and the value of the attribute on each HdOfdmSpectrumPhy instance to be created

    void SetPhyAttribute(std::string name, const AttributeValue& v);

// Set the name and the value of the attribute on each AlohaNoackNetDevice created

    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    void SetTarget(Ptr<NetDevice> target);

// Configure the AntennaModel instance for each new device to be created

    template <typename... Ts>
    void SetAntenna(std::string type, Ts&&... args);


// Takes as a parameter the set of nodes on which a device must be created and returns
// a device container which contains all the devices created by this method.

    NetDeviceContainer Install(NodeContainer c) const;

// Takes as a parameter a node on which a device must be created and returns
// a device container which contains all the devices created by this method.

    NetDeviceContainer Install(Ptr<Node> node) const;

// Takes as a parameter the name of the node on which a device must be created and returns
// a device container which contains all the devices created by this method.

    NetDeviceContainer Install(std::string nodeName) const;


// Takes as a parameter the set of nodes on which a device must be created and also the type
// of the simulation in which the jammer will be deployed. 
// Returns a device container which contains all the devices created by this method.

    NetDeviceContainer Install(NodeContainer c, JammerType type) const;

    
  protected:
    ObjectFactory m_phy;            //!< Object factory for the phy objects
    ObjectFactory m_device;         //!< Object factory for the NetDevice objects
    ObjectFactory m_antenna;        //!< Object factory for the Antenna objects
    Ptr<SpectrumChannel> m_channel; //!< Channel
    Ptr<SpectrumValue> m_txPsd;     //!< Tx power spectral density    
    Ptr<NetDevice> m_target;        //The NetDevice to target with beamforming

  private:
    Ptr<SpectrumValue> m_mmWavePsd;     //!< Tx power spectral density
    JammerType m_simType;           //!< The simulation type
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>

void
JammingGeneratorHelper::SetAntenna(std::string type, Ts&&... args)
{
    m_antenna = ObjectFactory(type, std::forward<Ts>(args)...);
}


} // namespace ns3

#endif /* WAVEFORM_GENERATOR_HELPER_H */
