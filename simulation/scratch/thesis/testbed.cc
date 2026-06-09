#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/mmwave-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/mmwave-helper.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/aodv-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/building.h"
#include "ns3/buildings-helper.h"
#include "ns3/bridge-helper.h"
#include "ns3/waypoint-mobility-model.h"
#include "ns3/jamming-model-helper.h"

#include "helper/command-line-helper.h"
#include "helper/testbed-helper.h"
#include "helper/string-helper.h"
#include "helper/attack-logger.h"

#include "simulation/bottle-filling-system.h"
#include "simulation/config.h"
#include "simulation/plc-a.h"
#include "simulation/plc-b.h"
#include "model/sensor-property.h"
#include "model/sensor.h"
#include "model/actuator.h"
#include "model/hmi.h"

#include "attacks/attack-base.h"
#include "attacks/dos.h"
#include "attacks/injection.h"
#include "attacks/mitm.h"
#include "attacks/suppression.h"
#include "attacks/jammer.h"
#include "attacks/reactive-jammer.h"
#include "attacks/pgw-flooding.h"
#include "attacks/forced-handover.h"
#include "attacks/multi-injection.h"
#include "attacks/spectrum-analysis.h"
#include "ns3/spectrum-analyzer-helper.h"
#include "ns3/spectrum-analyzer.h"
#include "ns3/non-communicating-net-device.h"


using namespace ns3;
using namespace mmwave;
using namespace testbed;

NS_LOG_COMPONENT_DEFINE("Testbed");

int main(int argc, char *argv[]) {
    // Initialize command-line
    Ptr<TestbedHelper> testbed = CreateObject<TestbedHelper>();
    testbed->RegisterCommandLineArguments();

    // Parse arguments
    std::vector<std::string> customComponents = {"PLC_A", "Testbed", "BottleFillingSystem"};
    if(!testbed->ParseCommandLineArguments(argc, argv, customComponents)) {
        return 1;
    }

    // Disable nagles algorithm
    Config::SetDefault ("ns3::TcpSocket::TcpNoDelay", BooleanValue(true));

    // Number of nodes + 1 as node0 is unable to receive TCP packets 
    NodeContainer actuatorNodes;
    actuatorNodes.Create(4);

    NodeContainer plcNodes;
    plcNodes.Create(2);

    NodeContainer sensorNodes;
    sensorNodes.Create(4);

    NodeContainer hmiNodes;
    hmiNodes.Create(1);

    NodeContainer nodes;
    nodes.Add(actuatorNodes);
    nodes.Add(plcNodes);
    nodes.Add(sensorNodes);
    nodes.Add(hmiNodes);

    InternetStackHelper internet;

    NetDeviceContainer devices;

    // Define variables used in 5G environment outside of if
    // Otherwise "Cannot run ScanDevices without an aggregated node"
    Ptr<MmWaveHelper> mmwaveHelper;
    Ptr<MmWavePointToPointEpcHelper> epcHelper;
    Ptr<Node> pgw;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    NodeContainer ueNodes, enbNodes;
    MobilityHelper enbMobility, ueMobility;
    Ptr<ListPositionAllocator> enbPositionAlloc, uePositionAlloc;
    NetDeviceContainer enbDevices, ueDevices;
    Ipv4InterfaceContainer ueIpIfaces;
    Ptr<Building> building;

    double centerFrequency = 28e9; // 26 GHz
    double bandwidth = 100e6; // 80 MHz

    // Define variables used in wired environment
    CsmaHelper csma;
    NetDeviceContainer switchDevices;
    BridgeHelper bridge;
    NodeContainer switchNodes;


    // Required for attack setup
    bool handoverAttack = false;
    NodeContainer movingNodes;
    for (AttackScheduleEntry attack : testbed->GetAttacks()) {
        if (attack.GetAttackIdentifier() == "ForceHandover") {
            handoverAttack = true;
        }
    }

    // Initialize Network
    if (testbed->Using5G()) {
        CMD_LOG_INFO("Using 5G communication...");

        // Create and configure the mmWave helper
        // Phase 1 (MCS instrumentation): route the per-TB PHY Rx trace (which carries
        // mcs, rnti, SINR, corrupt/NACK and TBler) into this run's output directory so
        // we obtain per-UE MCS over time. This must be set BEFORE the MmWaveHelper is
        // created, because the helper constructs its MmWavePhyTrace (and latches this
        // filename default into a static member) at construction time.
        Config::SetDefault("ns3::MmWavePhyTrace::OutputFilename",
                           StringValue(testbed->GetLogPath() + "rx-packet-trace.tsv"));
        // gNB-side DL transmission trace: logs the COMMANDED (AMC/CQI-derived) MCS per
        // DL allocation, independent of whether the UE decodes it. This is the true
        // link-adaptation signal for the MCS-downgrade attack (the UE Rx trace only
        // sees surviving TBs and is biased high under jamming).
        Config::SetDefault("ns3::MmWavePhyTrace::DlPhyTransmissionFilename",
                           StringValue(testbed->GetLogPath() + "dl-phy-trace.tsv"));
        mmwaveHelper = CreateObject<MmWaveHelper>();
        mmwaveHelper->SetPathlossModelType ("ns3::ThreeGppIndoorOfficePropagationLossModel");
        mmwaveHelper->SetChannelConditionModelType ("ns3::ThreeGppIndoorOpenOfficeChannelConditionModel");

        // Modification: Try out different antenna array configurations
        /*
        mmwaveHelper->SetUePhasedArrayModelAttribute("NumColumns", UintegerValue(1));
        mmwaveHelper->SetUePhasedArrayModelAttribute("NumRows", UintegerValue(1));

        mmwaveHelper->SetEnbPhasedArrayModelAttribute("NumColumns", UintegerValue(1));
        mmwaveHelper->SetEnbPhasedArrayModelAttribute("NumRows", UintegerValue(1));
        */


        epcHelper = CreateObject<MmWavePointToPointEpcHelper>();
        mmwaveHelper->SetEpcHelper(epcHelper);
        Config::SetDefault("ns3::MmWavePhyMacCommon::CenterFreq", DoubleValue(centerFrequency));
        Config::SetDefault("ns3::MmWavePhyMacCommon::Bandwidth", DoubleValue(bandwidth));
        Config::SetDefault("ns3::MmWaveEnbPhy::TxPower", DoubleValue(34.0)); // eNB power in dBm
        Config::SetDefault("ns3::MmWaveUePhy::TxPower", DoubleValue(23.0)); // eNB power in dBm

        // Handover
        mmwaveHelper->SetAttribute("UseIdealRrc", BooleanValue(true));
        mmwaveHelper->SetLteHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");

        Config::SetDefault("ns3::LteEnbRrc::FixedTttValue", UintegerValue(150));
        Config::SetDefault("ns3::LteEnbRrc::SecondaryCellHandoverMode", EnumValue(LteEnbRrc::FIXED_TTT));

        pgw = epcHelper->GetPgwNode();

        // Set up the LTE and mmWave parts of the 5G network
        ueNodes = nodes; // All nodes act as UEs
        if (!handoverAttack) {
            enbNodes.Create(1); // One 5G gNB (base station)
        } else {
            enbNodes.Create(2); // Two 5G gNB (base station)
        }

        // Building setup
        building = CreateObject<Building>();
        building->SetBoundaries(Box(0.0, 50.0, 0.0, 20.0, 0.0, 10.0)); // Building with 50m x, 20m y and 10m z
        building->SetBuildingType(Building::Commercial);
        building->SetExtWallsType(Building::ConcreteWithoutWindows);
        building->SetNFloors(1);
        building->SetNRoomsX(1);
        building->SetNRoomsY(1);

        // Mobility setup for eNB nodes
        enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        enbPositionAlloc = CreateObject<ListPositionAllocator>();
        enbPositionAlloc->Add(Vector(25.0,10.0,9.5)); // Center of building (ceiling)
        if (handoverAttack) {
            enbPositionAlloc->Add(Vector(1.0,19.0,9.5));
        }
        enbMobility.SetPositionAllocator(enbPositionAlloc);
        enbMobility.Install(enbNodes);

        // Mobility setup for UE nodes
        ueMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        uePositionAlloc = CreateObject<ListPositionAllocator>();

        uePositionAlloc->Add(Vector(-100,-100,-100)); // unused node
        uePositionAlloc->Add(Vector(13,2,2)); // actuator: tankWaterFlowIn
        uePositionAlloc->Add(Vector(13,2,1.25)); // actuator: tankWaterFlowOut
        uePositionAlloc->Add(Vector(13,4,.8)); // actuator: motorController
        uePositionAlloc->Add(Vector(12,2.5,1.5)); // PLC: plcA
        uePositionAlloc->Add(Vector(13,3,.8)); // PLC: plcB
        uePositionAlloc->Add(Vector(12.5,2.5,1.5)); // Sensor: tankWaterLevel
        uePositionAlloc->Add(Vector(13,2,1)); // Sensor: bottleFillLevel
        uePositionAlloc->Add(Vector(13,1,1)); // Sensor: bottlePosition
        uePositionAlloc->Add(Vector(13,.5,1)); // Sensor: waterSpill
        uePositionAlloc->Add(Vector(25,1,1)); // HMI

        ueMobility.SetPositionAllocator(uePositionAlloc);
        ueMobility.Install(ueNodes);

        BuildingsHelper::Install(enbNodes);
        BuildingsHelper::Install(ueNodes);

        // Install mmWave devices
        enbDevices = mmwaveHelper->InstallEnbDevice(enbNodes);
        ueDevices = mmwaveHelper->InstallUeDevice(ueNodes);
        devices.Add(ueDevices);

        // Phase 1 (MCS instrumentation): connect the PHY Rx traces (DL received at the
        // UE, UL received at the gNB). This populates rx-packet-trace.tsv with one row
        // per transport block: time, rnti, mcs, SINR(dB), corrupt (=NACK) and TBler.
        mmwaveHelper->EnableTraces();

        // Install internet stack on all nodes
        internet.Install(ueNodes);

        // Assign IP addresses
        ueIpIfaces = epcHelper->AssignUeIpv4Address(ueDevices);
        for(uint32_t i = 0; i < ueNodes.GetN(); i++) {
            Ptr<Node> ueNode = ueNodes.Get(i);
            // Set default gateway
            Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
        }

        // Attach UEs to the eNB
        mmwaveHelper->AttachToClosestEnb(ueDevices, enbDevices);

        // Initialize SpectrumAnalyzer node, NetDevice and Phy
        /*
        JammingGeneratorHelper analyzerHelper(JammerType::MmWave);
        Ptr<NetDevice> analyzerNetDevice = ueNodes.Get(0)->GetDevice(0);
        Ptr<mmwave::MmWaveUeNetDevice> analyzerDevice = DynamicCast<mmwave::MmWaveUeNetDevice>(analyzerNetDevice);

        NodeContainer analyzerNodes;
        analyzerNodes.Create(1);
        MobilityHelper analyzerMobility;
        analyzerMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        Ptr<ListPositionAllocator> analyzerPosition = Create<ListPositionAllocator>();
        analyzerPosition->Add(Vector(25, 10, 9));
        analyzerMobility.SetPositionAllocator(analyzerPosition);
        analyzerMobility.Install(analyzerNodes);
        analyzerHelper.SetMmWaveChannel(analyzerDevice);
        //Not interested in jamming, just device needed
        analyzerHelper.SetMmWaveTxPowerSpectralDensity(centerFrequency, bandwidth, 0);
        analyzerHelper.Install(analyzerNodes);

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
        analyzer.SetAntenna("ns3::UniformPlanarArray");
        NetDeviceContainer analyzerDevices = analyzer.Install(analyzerNodes);
        Ptr<SpectrumAnalyzer> phy = DynamicCast<SpectrumAnalyzer>(DynamicCast<NonCommunicatingNetDevice>(analyzerDevices.Get(0))->GetPhy());
        
        // Create a complex number 1 + 0i
        std::complex<double> complexValue(1.0, 0.0);
        std::vector<std::complex<double>> complexVector = {complexValue};
        // Create a ComplexVector and initialize it with the complexValue
        UniformPlanarArray::ComplexVector myVector(complexVector);
        Ptr<UniformPlanarArray> analyzerAntenna = DynamicCast<UniformPlanarArray> (phy->GetAntenna());
        analyzerAntenna->SetNumColumns(1);
        analyzerAntenna->SetNumRows(1);
        analyzerAntenna->SetBeamformingVector(myVector);
        */
        

    } else {
        CMD_LOG_INFO("Using wired communication...");

        switchNodes.Create(1);

        internet.Install(nodes);

        csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
        csma.SetChannelAttribute("Delay", StringValue ("10us"));

        for (uint32_t i = 0; i < nodes.GetN(); i++) {
            NetDeviceContainer link = csma.Install(NodeContainer(nodes.Get(i), switchNodes));
            devices.Add(link.Get(0));
            switchDevices.Add(link.Get(1));
        }

        bridge.Install(switchNodes.Get(0), switchDevices);

        Ipv4AddressHelper ipv4;
        ipv4.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

        Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    }

    // Create underlying physical system
    Ptr<PhysicalSystem> physicalSystem = CreateObject<BottleFillingSystem>(testbed);

    // PLC_A
    std::unordered_map<std::string, Ptr<Sensor>> sensorApplications_a;
    sensorApplications_a["tankWaterLevel"] = CreateObject<Sensor>(physicalSystem, "tankWaterLevel", std::vector<std::string>{}, std::vector<SensorProperty>{SensorProperty("tankWaterLevel", 0, config::water_tank_capacity_ml, 0.0005)});
    sensorApplications_a["bottleFillLevel"] = CreateObject<Sensor>(physicalSystem, "bottleFillLevel", std::vector<std::string>{}, std::vector<SensorProperty>{SensorProperty("bottleFillLevel", 0, config::water_bottle_sensor_max, 0.001)});
    sensorApplications_a["bottlePosition"] = CreateObject<Sensor>(physicalSystem, "bottlePosition", std::vector<std::string>{}, std::vector<SensorProperty>{SensorProperty("bottlePosition", config::conveyor_bottle_position_sensor_min, config::conveyor_bottle_position_sensor_max, 0.01)});
    sensorApplications_a["waterLeak"] = CreateObject<Sensor>(physicalSystem, "waterLeak", std::vector<std::string>{"waterSpillDetected"}, std::vector<SensorProperty>{});
    
    sensorNodes.Get(0)->AddApplication(sensorApplications_a["tankWaterLevel"]);
    sensorNodes.Get(1)->AddApplication(sensorApplications_a["bottleFillLevel"]);
    sensorNodes.Get(2)->AddApplication(sensorApplications_a["bottlePosition"]);
    sensorNodes.Get(3)->AddApplication(sensorApplications_a["waterLeak"]);

    std::unordered_map<std::string, Ptr<Actuator>> actuatorApplications_a;
    actuatorApplications_a["tankWaterFlowIn"] = CreateObject<Actuator>(physicalSystem, "tankWaterFlowIn", std::vector<std::string>{"tankWaterFlowIn"}, std::vector<std::string>{});
    actuatorApplications_a["tankWaterFlowOut"] = CreateObject<Actuator>(physicalSystem, "tankWaterFlowOut", std::vector<std::string>{"tankWaterFlowOut"}, std::vector<std::string>{});
    
    actuatorNodes.Get(3)->AddApplication(actuatorApplications_a["tankWaterFlowIn"]);
    actuatorNodes.Get(1)->AddApplication(actuatorApplications_a["tankWaterFlowOut"]);

    Ptr<PLC_A> plcA = CreateObject<PLC_A>(sensorApplications_a, actuatorApplications_a);
    plcNodes.Get(0)->AddApplication(plcA);

    // PLC_B
    std::unordered_map<std::string, Ptr<Sensor>> sensorApplications_b;
    sensorApplications_b["bottlePosition"] = sensorApplications_a["bottlePosition"];
    sensorApplications_b["bottleFillLevel"] = sensorApplications_a["bottleFillLevel"];

    std::unordered_map<std::string, Ptr<Actuator>> actuatorApplications_b;
    actuatorApplications_b["motorController"] = CreateObject<Actuator>(physicalSystem, "motorController", std::vector<std::string>{"conveyorBeltEngineStart", "conveyorBeltEngineDirection"}, std::vector<std::string>{"conveyorBeltEngineSpeed"});
    
    actuatorNodes.Get(2)->AddApplication(actuatorApplications_b["motorController"]);

    Ptr<PLC_B> plcB = CreateObject<PLC_B>(sensorApplications_b, actuatorApplications_b);
    plcNodes.Get(1)->AddApplication(plcB);

    Ptr<HMI> hmi = CreateObject<HMI>("HMI", std::vector<Ptr<PLC>>{plcA, plcB});
    hmiNodes.Get(0)->AddApplication(hmi);

    // Create application containers
    ApplicationContainer sensors;
    sensors.Add(sensorApplications_a["tankWaterLevel"]);
    sensors.Add(sensorApplications_a["bottleFillLevel"]);
    sensors.Add(sensorApplications_a["bottlePosition"]);

    ApplicationContainer actuators;
    actuators.Add(actuatorApplications_a["tankWaterFlowIn"]);
    actuators.Add(actuatorApplications_a["tankWaterFlowOut"]);
    actuators.Add(actuatorApplications_b["motorController"]);

    ApplicationContainer plcs;
    plcs.Add(plcA);
    plcs.Add(plcB);

    // Dataset generation
    if (testbed->CreateLogFilesEnabled()) {
        ApplicationContainer allApplications;
        allApplications.Add(plcs);
        allApplications.Add(hmi);
        allApplications.Add(sensors);
        allApplications.Add(actuators);

        // Enable pcap according to command-line setting
        if (testbed->GetPcapMode() == "all") {
            internet.EnablePcapIpv4All(testbed->GetLogPath() + "testbed");
        } else if (testbed->Using5G()) {
            internet.EnablePcapIpv4(testbed->GetLogPath() + "testbed", enbNodes);
        } else {
            csma.EnablePcap(testbed->GetLogPath() + "testbed", switchNodes, false);
        }

        std::ofstream DevicesFile(testbed->GetLogPath() + "devices.txt");
        for (uint32_t i = 0; i < allApplications.GetN(); i++) {
            Ptr<IndustrialDevice> device = DynamicCast<IndustrialDevice>(allApplications.Get(i));
            Ipv4Address address = device->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
            DevicesFile << device->GetIdentifier() << ": " << address << " (Node: " << device->GetNode()->GetId() << ")" << std::endl;
        }
        DevicesFile.close();
    }

    // Attacks
    if (testbed->CreateLogFilesEnabled()) {
        AttackLogger::GetInstance().SetPath(testbed->GetLogPath() + "attacks.json");
    }
    //Ptr<DoS> dos = CreateObject<DoS>(testbed, plcA, *plcB, 0x04, std::vector<uint8_t>{0x00, 0x00, 0x00, 0x02});
    Ptr<DoS> dos = CreateObject<DoS>(testbed, hmi, *plcA, 0x02, std::vector<uint8_t>{0x00, 0x00, 0x00, 0x01});
    Ptr<InjectionAttack> injection = CreateObject<InjectionAttack>(testbed, plcA, *actuatorApplications_a["tankWaterFlowOut"], 0x05, std::vector<uint8_t>{0x00, 0x00, 0xFF, 0x00}); // Open outputValve
    Ptr<MultiInjectionAttack> multiInjection = CreateObject<MultiInjectionAttack>(testbed, plcA, *actuatorApplications_a["tankWaterFlowIn"], 0x05, std::vector<uint8_t>{0x00, 0x00, 0xFF, 0x00}, std::vector<uint8_t>{0x00, 0x00, 0x00, 0x00});
    Ptr<SuppressionAttack> suppression = CreateObject<SuppressionAttack>(testbed, plcA, *plcA);
    Ptr<MitM> mitm16 = CreateObject<MitM>(testbed, plcB, 16);
    Ptr<MitM> mitm32 = CreateObject<MitM>(testbed, plcB, 32);
    Ptr<MitM> mitm48 = CreateObject<MitM>(testbed, plcB, 48);
    Ptr<MitM> mitm64 = CreateObject<MitM>(testbed, plcB, 64);
    auto mitmRequestHook = [](IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) {
        std::vector<uint8_t> mitmMessage = {0x00,0x00,0x00,0x02,0x01,0x00}; // In case plc indicates to stop motor
        if (server->GetIdentifier() == "motorController" && *functionCode == 0x0F && *message == mitmMessage) {
            (*message)[5] = 0x01; // Keep motor running
            CMD_LOG_INFO("MitM kept motor running");
        }
        return;
    };
    Ptr<MitM> mitm16msg = CreateObject<MitM>(testbed, plcB, 16, actuatorApplications_b["motorController"], mitmRequestHook);
    Ptr<MitM> mitm32msg = CreateObject<MitM>(testbed, plcB, 32, actuatorApplications_b["motorController"], mitmRequestHook);
    Ptr<MitM> mitm48msg = CreateObject<MitM>(testbed, plcB, 48, actuatorApplications_b["motorController"], mitmRequestHook);
    Ptr<MitM> mitm64msg = CreateObject<MitM>(testbed, plcB, 64, actuatorApplications_b["motorController"], mitmRequestHook);

    Ipv4Address pgwAddress;
    if (testbed->Using5G()) {
        pgwAddress = epcHelper->GetPgwNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
    }
    Ptr<PGWFloodingAttack> pgwFlooding = CreateObject<PGWFloodingAttack>(testbed, plcB, pgwAddress, 10000);

    // 5G only
    Ptr<ListPositionAllocator> jammerPosition = CreateObject<ListPositionAllocator>();
    if (testbed->GetJammerInside()) {
        jammerPosition->Add(Vector(19, 7, 2));
    } else {
       jammerPosition->Add(Vector(13, 22, 2)); 
    }
    NodeContainer jammerContainer;
    Ptr<Jammer> jammer;
    Ptr<ReactiveJammer> reactiveJammer;

    Ptr<SpectrumAnalysis> spectrumAnalysisAttack;
    NodeContainer analyzerContainer;
    Ptr<ListPositionAllocator> analyzerPosition = CreateObject<ListPositionAllocator>();
    //analyzerPosition->Add(Vector(13, 22, 2)); 
    analyzerPosition->Add(Vector(19, 7, 2)); 


    // 30 Moving UEs (only created if needed)
    Ptr<ForcedHandover> forcedHandover = CreateObject<ForcedHandover>(testbed, plcB);
    MobilityHelper movingMobility;
    NetDeviceContainer handoverUeDevices;
    Ipv4InterfaceContainer handoverUeIpIfaces;
    movingMobility.SetMobilityModel("ns3::WaypointMobilityModel");
    Vector initialPosition(1.0, 19.0, 2.0);
    Vector targetPosition(25.0, 10.0, 2.0);
    bool forcedHandoverAlreadySetUp = false;
    ApplicationContainer movingClientApps;
    UdpServerHelper server(4000);
    ApplicationContainer handoverServer;

    // End 5G Only
    bool initializedJammer = false;

    for (AttackScheduleEntry attack : testbed->GetAttacks()) {
        // Schedule attacks
        Time attackStart = attack.GetStartTime();
        Time attackEnd = attack.GetEndTime();
        if (attack.GetAttackIdentifier() == "DoS") {
            Simulator::Schedule(attackStart, &DoS::StartAttack, dos);
            Simulator::Schedule(attackEnd, &DoS::StopAttack, dos);
        } else if (attack.GetAttackIdentifier() == "Injection") {
            Simulator::Schedule(attackStart, &InjectionAttack::StartAttack, injection);
            Simulator::Schedule(attackEnd, &InjectionAttack::StopAttack, injection);
        } else if (attack.GetAttackIdentifier() == "MultiInjection") { //Alternating injection will schedule 2x AttackNumber
            Simulator::Schedule(attackStart, &MultiInjectionAttack::StartAttack, multiInjection);
        } else if (attack.GetAttackIdentifier() == "Suppression") {
            Simulator::Schedule(attackStart, &SuppressionAttack::StartAttack, suppression);
            Simulator::Schedule(attackEnd, &SuppressionAttack::StopAttack, suppression);
        } else if (attack.GetAttackIdentifier() == "MitM16") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm16);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm16);
        } else if (attack.GetAttackIdentifier() == "MitM32") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm32);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm32);
        } else if (attack.GetAttackIdentifier() == "MitM48") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm48);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm48);
        } else if (attack.GetAttackIdentifier() == "MitM64") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm64);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm64);
        } else if (attack.GetAttackIdentifier() == "MitM16msg") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm16msg);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm16msg);
        } else if (attack.GetAttackIdentifier() == "MitM32msg") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm32msg);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm32msg);
        } else if (attack.GetAttackIdentifier() == "MitM48msg") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm48msg);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm48msg);
        } else if (attack.GetAttackIdentifier() == "MitM64msg") {
            Simulator::Schedule(attackStart, &MitM::StartAttack, mitm64msg);
            Simulator::Schedule(attackEnd, &MitM::StopAttack, mitm64msg);
        } else if (attack.GetAttackIdentifier() == "Jammer") {
            if (!testbed->Using5G()) {
                CMD_LOG_ERROR("Scheduled jammer attack in wired setting. Skipping attack...");
            } else {
                if (!initializedJammer) {
                    jammer = CreateObject<Jammer>(testbed, plcB, *plcB, &jammerContainer, jammerPosition, 
                        centerFrequency, bandwidth, 0.400, testbed->GetDutyCycle(), testbed->GetJammPower());
                    if (testbed->GetJammerDirected()) {
                        jammer->SetTarget(enbDevices.Get(0));
                    }
                    initializedJammer = true;
                }
                Simulator::Schedule(attackStart, &Jammer::StartAttack, jammer);
                Simulator::Schedule(attackEnd, &Jammer::StopAttack, jammer);
            }
        } else if (attack.GetAttackIdentifier() == "ReactiveJammer") {
            if (!testbed->Using5G()) {
                CMD_LOG_ERROR("Scheduled reactive jammer attack in wired setting. Skipping attack...");
            } else {
                if (!initializedJammer) {
                    // MCS-aware reactive jammer. Threshold/pulse parameters are fixed
                    // here for now; they can be promoted to command-line options to
                    // sweep them in experiments.
                    reactiveJammer = CreateObject<ReactiveJammer>(testbed, plcB, *plcB,
                        &jammerContainer, jammerPosition, centerFrequency, bandwidth,
                        testbed->GetJammPower(),
                        /*mcsThreshold*/ 10,
                        /*pulseDuration*/ MicroSeconds(500),
                        /*minGap*/ MicroSeconds(250));
                    initializedJammer = true;
                }
                Simulator::Schedule(attackStart, &ReactiveJammer::StartAttack, reactiveJammer);
                Simulator::Schedule(attackEnd, &ReactiveJammer::StopAttack, reactiveJammer);
            }
        } else if (attack.GetAttackIdentifier() == "SpectrumAnalysis") {
            Ptr<NetDevice> analyzerNetDevice = ueNodes.Get(0)->GetDevice(0);
            Ptr<mmwave::MmWaveUeNetDevice> analyzerDevice = DynamicCast<mmwave::MmWaveUeNetDevice>(analyzerNetDevice);
            spectrumAnalysisAttack = CreateObject<SpectrumAnalysis>(testbed, analyzerDevice, &analyzerContainer, analyzerPosition, centerFrequency, bandwidth);
            //spectrumAnalysisAttack->SetTarget(enbDevices.Get(0));
            Simulator::Schedule(attackStart, &SpectrumAnalysis::StartAttack, spectrumAnalysisAttack);
            Simulator::Schedule(attackEnd, &SpectrumAnalysis::StopAttack, spectrumAnalysisAttack);

        } else if (attack.GetAttackIdentifier() == "PGWFlooding") {
            if (!testbed->Using5G()) {
                CMD_LOG_ERROR("Scheduled PGW Flooding attack in wired setting. Skipping attack...");
            } else {
                Simulator::Schedule(attackStart, &PGWFloodingAttack::StartAttack, pgwFlooding);
                Simulator::Schedule(attackEnd, &PGWFloodingAttack::StopAttack, pgwFlooding);
            }
        } else if (attack.GetAttackIdentifier() == "ForceHandover") {
            if (!testbed->Using5G()) {
                CMD_LOG_ERROR("Scheduled PGW Flooding attack in wired setting. Skipping attack...");
            } else {
                if (!forcedHandoverAlreadySetUp) {
                    forcedHandoverAlreadySetUp = true;


                    movingNodes.Create(30);
                    nodes.Add(movingNodes);
                    
                    movingMobility.Install(movingNodes);

                    // Install mmWave devices
                    handoverUeDevices = mmwaveHelper->InstallUeDevice(movingNodes);
                    devices.Add(handoverUeDevices);

                    // Install internet stack on all nodes
                    internet.Install(movingNodes);

                    // Assign IP addresses
                    ueIpIfaces = epcHelper->AssignUeIpv4Address(handoverUeDevices);
                    for(uint32_t i = 0; i < movingNodes.GetN(); i++) {
                        Ptr<Node> ueNode = movingNodes.Get(i);
                        // Set default gateway
                        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
                        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
                    }

                    // Assign IP addresses
                    for(uint32_t i = 0; i < movingNodes.GetN(); i++) {
                        Ptr<Node> ueNode = movingNodes.Get(i);

                        Ptr<WaypointMobilityModel> mobility = ueNode->GetObject<WaypointMobilityModel>();
                        mobility->AddWaypoint(Waypoint(Seconds(0.0), initialPosition));
                        mobility->AddWaypoint(Waypoint(attackStart, initialPosition));
                        mobility->AddWaypoint(Waypoint(attackEnd, targetPosition));
                    }

                    // Add traffic
                    handoverServer = server.Install(movingNodes.Get(0));

                    OnOffHelper client("ns3::UdpSocketFactory", InetSocketAddress(movingNodes.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 4000));
                    
                    handoverServer.Start(attackStart);
                    handoverServer.Stop(attackEnd);

                    client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
                    client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.0]"));
                    client.SetAttribute("DataRate", StringValue("1Mbps"));
                    client.SetAttribute("PacketSize", UintegerValue(512));

                    for (uint32_t i = 1; i < movingNodes.GetN(); i++) {
                        movingClientApps.Add(client.Install(movingNodes.Get(i)));
                    }

                    // movingClientApps.Start(attackStart);
                    // movingClientApps.Stop(attackEnd);

                    movingClientApps.Start(Seconds(.5));

                    // Attach UEs to the eNB
                    mmwaveHelper->AttachToClosestEnb(handoverUeDevices, enbDevices);

                    // Temporary
                    mmwaveHelper->EnablePdcpTraces();
                    mmwaveHelper->EnableRlcTraces();

                    Simulator::Schedule(attackStart, &ForcedHandover::StartAttack, forcedHandover);
                    Simulator::Schedule(attackEnd, &ForcedHandover::StopAttack, forcedHandover);
                } else {
                    CMD_LOG_ERROR("Multiple force Handover attacks scheduled. Skipping additional ones...");
                }
            }
        } else {
            CMD_LOG_ERROR("Tried scheduling unknown attack " << attack.GetAttackIdentifier() << ". Aborting.");
            return 1;
        }

    }
    if (testbed->GetBackgroundJammPower() > 0 && !initializedJammer) { //Don't background jamm if already jammer in use. 
        jammer = CreateObject<Jammer>(testbed, plcB, *plcB, &jammerContainer, jammerPosition, centerFrequency, bandwidth, 1, 1, testbed->GetBackgroundJammPower());
        //jammer->SetTarget(enbDevices.Get(0));
        initializedJammer = true;
        Simulator::Schedule(Seconds(0.5), &Jammer::StartAttack, jammer);
        Simulator::Schedule(Seconds(testbed->GetSimulationDuration() - 0.5), &Jammer::StopAttack, jammer);
    }

    // Request process halt, resume with slower motor speed, increase later (HMI behavior)
    // TODO: Randomize timing?
    uint16_t haltProcessAddress = plcB->GetCoilAddress("hmiHaltProcess");
    uint16_t engineSpeedAddress = plcB->GetHoldingRegisterAddress("engineSpeed");
    Simulator::Schedule(Seconds(600), &HMI::SendModbusRequest, hmi, *plcB, 502, 0x05, std::vector<uint8_t>{(uint8_t)((haltProcessAddress >> 8) & 0xff), (uint8_t)((haltProcessAddress >> 0) & 0xff), 0xFF, 0x00}, RequestContext("plcB", haltProcessAddress, 1)); // Halt process
    Simulator::Schedule(Seconds(605), &HMI::SendModbusRequest, hmi, *plcB, 502, 0x06, std::vector<uint8_t>{(uint8_t)((engineSpeedAddress >> 8) & 0xff), (uint8_t)((engineSpeedAddress >> 0) & 0xff), 0x00, 0x32}, RequestContext("plcB", engineSpeedAddress, 1)); // Set engine speed low (50)
    Simulator::Schedule(Seconds(660), &HMI::SendModbusRequest, hmi, *plcB, 502, 0x05, std::vector<uint8_t>{(uint8_t)((haltProcessAddress >> 8) & 0xff), (uint8_t)((haltProcessAddress >> 0) & 0xff), 0x00, 0x00}, RequestContext("plcB", haltProcessAddress, 1)); // Resume process
    Simulator::Schedule(Seconds(780), &HMI::SendModbusRequest, hmi, *plcB, 502, 0x06, std::vector<uint8_t>{(uint8_t)((engineSpeedAddress >> 8) & 0xff), (uint8_t)((engineSpeedAddress >> 0) & 0xff), 0x00, 0x64}, RequestContext("plcB", engineSpeedAddress, 1)); // Reset engine speed (100)

    Simulator::Stop(Seconds(testbed->GetSimulationDuration()));

    SeedManager::SetRun(testbed->GetRunNumber());
    uint32_t seed = SeedManager::GetSeed ();
    uint32_t run = SeedManager::GetRun ();
  
    std::cout << "Current Seed: " << seed << std::endl;
    std::cout << "Current Run: " << run << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    Simulator::Run();

    // Write time it took to simulate
    if (testbed->CreateLogFilesEnabled()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::ofstream SimulationFile(testbed->GetLogPath() + "simulation.txt");
        SimulationFile << "Simulation duration: " << elapsedTime.count() << " seconds." << std::endl;
        SimulationFile.close();
    }

    Simulator::Destroy();

    return 0;
}