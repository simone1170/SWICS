#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/udp-socket-factory.h"
#include "pgw-flooding.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("PGWFlooding");

PGWFloodingAttack::PGWFloodingAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, Ipv4Address pgwAddress, uint32_t packetSize) : AttackBase(testbed, device), m_pgwAddress(pgwAddress), m_packetSize(packetSize) {

};

void PGWFloodingAttack::StartAttack() {
    AttackBase::StartAttack();
    CMD_LOG_WARN("Started PGW Flooding Attack");

    m_socket = Socket::CreateSocket(m_device->GetNode(), UdpSocketFactory::GetTypeId());
    m_socket->Bind();
    m_socket->Connect(InetSocketAddress(m_pgwAddress, 2123));

    RunAttack();
}

void PGWFloodingAttack::RunAttack() {
    if (IsActive()) {
        Ptr<Packet> packet = Create<Packet>(m_packetSize);
        m_socket->Send(packet);
        Simulator::Schedule(MicroSeconds(10), &PGWFloodingAttack::RunAttack, this);
    }
}