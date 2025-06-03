#include "ns3/node.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"

#include "../utils/modbus-header.h"
#include "../helper/command-line-helper.h"
#include "../helper/hex-helper.h"
#include "../helper/config-helper.h"
#include "industrial-device.h"

NS_LOG_COMPONENT_DEFINE ("IndustrialDevice");

using namespace testbed;

IndustrialDevice::IndustrialDevice(std::string identifier, bool isServer) : m_identifier(identifier), m_isServer(isServer) {
    CMD_LOG_FUNCTION(this << isServer);
}

IndustrialDevice::~IndustrialDevice() {
    CMD_LOG_FUNCTION(this);
}

Ptr<Socket> IndustrialDevice::GetSocket(InetSocketAddress remote) {
    CMD_LOG_FUNCTION(this << remote);

    if(m_connectedServers.find(remote) == m_connectedServers.end()) {
        CreateSocket(remote);
    }
    return m_connectedServers[remote];
}

bool IndustrialDevice::IsServer() {
    CMD_LOG_FUNCTION(this);

    return m_isServer;
}

std::string IndustrialDevice::GetIdentifier() {
    return m_identifier;
}

void IndustrialDevice::CreateSocket(InetSocketAddress remote) {
    CMD_LOG_FUNCTION(this << remote);

    TypeId tid;
    tid = TypeId::LookupByName("ns3::TcpSocketFactory");

    // Only create socket if not already exists
    if(m_connectedServers.find(remote) != m_connectedServers.end()) {
        return;
    }

    // if remote is own address: create server socket
    if(remote == InetSocketAddress(m_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 502)) {
        Ptr<Socket> socket = Socket::CreateSocket(m_node, tid);
        socket->SetAttribute("TcpNoDelay", BooleanValue(true));
        socket->Bind(remote);
        socket->Listen();
        socket->SetRecvCallback(MakeCallback(&IndustrialDevice::ReceiveModbusMessage, this));
        socket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
            MakeCallback(&IndustrialDevice::HandleAccept, this)
        );

        m_connectedServers[remote] = socket;
        m_isConnectedServer[remote] = false;

        CMD_LOG_INFO(m_identifier << ": Modbus server listening on " << remote.GetIpv4() << ":" << 502);
    } else {
        Ptr<Socket> socket = Socket::CreateSocket(m_node, tid);
        socket->SetAttribute("TcpNoDelay", BooleanValue(true));
        socket->SetRecvCallback(MakeCallback(&IndustrialDevice::ReceiveModbusMessage, this)); // For client
        socket->SetCloseCallbacks(
            MakeCallback(&IndustrialDevice::HandleSocketClose, this),
            MakeCallback(&IndustrialDevice::HandleSocketError, this)
        );
        
        m_connectedServers[remote] = socket;
        m_isConnectedServer[remote] = false;
    }
}

void IndustrialDevice::HandleAccept(Ptr<Socket> socket, const Address& from) {
    CMD_LOG_FUNCTION(this << socket << from);

    InetSocketAddress clientAddress = InetSocketAddress::ConvertFrom(from);

    m_connectedClients[clientAddress] = socket;
    m_isConnectedClient[clientAddress] = true;

    CMD_LOG_INFO(m_identifier << ": New connection from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
    socket->SetRecvCallback(MakeCallback(&IndustrialDevice::ReceiveModbusMessage, this));
}

void IndustrialDevice::SetRecvCallback(Callback<void, InetSocketAddress, ModbusHeader, uint8_t, std::vector<uint8_t>, RequestContext> cb) {
    CMD_LOG_FUNCTION(this);

    m_recvCallback = cb;
}

void IndustrialDevice::SendModbusRequest(IndustrialDevice& server, uint16_t port, uint8_t functionCode, std::vector<uint8_t> message, RequestContext context) {
    CMD_LOG_FUNCTION(this << server.GetNode() << port << functionCode << message << context);

    InetSocketAddress remote = InetSocketAddress(server.GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), port);
    uint16_t transactionId = m_transactionId;
    m_transactionId++;

    if(m_connectedServers.find(remote) == m_connectedServers.end()) {
        CreateSocket(remote);
    }

    Ptr<Socket> socket = m_connectedServers[remote];

    // Set context to connect response to request
    m_connectionContext[std::make_pair(transactionId, remote)] = context;
    
    if(!m_isConnectedServer[remote]) {
        Ipv4Address serverAddress = server.GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
        InetSocketAddress remote = InetSocketAddress(serverAddress, port);
        double attackDelay = 0;

        if (!m_attackRequestHook.IsNull()) {
            attackDelay = m_attackRequestHook(&server, &port, &functionCode, &message, &context);
        }
        
        context.SetTime(Simulator::Now());
        Callback<void, Ptr<Socket>> cb = Callback<void, Ptr<Socket>>([this, message, remote, transactionId, functionCode, attackDelay](Ptr<Socket> socket) {
            CMD_LOG_FUNCTION(this << message << remote << transactionId << functionCode);

            CMD_LOG_INFO(m_identifier << ": TCP connection established to " << remote.GetIpv4() << ":" << remote.GetPort());
            m_isConnectedServer[remote] = true;

            // Negative attackDelay == do not send message
            if (attackDelay == 0) {
                this->SendData(socket, functionCode, message, transactionId);
            } else if (attackDelay > 0) {
                Simulator::Schedule(MilliSeconds(attackDelay), &IndustrialDevice::SendData, this, socket, functionCode, message, transactionId);
            }
        });

        socket->Connect(remote);
        socket->SetConnectCallback(
            cb,
            MakeCallback(&IndustrialDevice::HandleConnectionFailed, this)
        );
        CMD_LOG_INFO(m_identifier << ": Initiated connection to " << serverAddress << ":" << port);
        m_openRequests[transactionId] = Simulator::Schedule(MilliSeconds(config::timeout_time_ms), &IndustrialDevice::HandleTimeout, this, std::make_pair(transactionId, remote));
    } else {
        double attackDelay = 0;

        if (!m_attackRequestHook.IsNull()) {
            attackDelay = m_attackRequestHook(&server, &port, &functionCode, &message, &context);
        }

        // Negative attackDelay == do not send message
        if (attackDelay == 0) {
            SendData(socket, functionCode, message, transactionId);
        } else if (attackDelay > 0) {
            Simulator::Schedule(MilliSeconds(attackDelay), &IndustrialDevice::SendData, this, socket, functionCode, message, transactionId);
        }
        context.SetTime(Simulator::Now());
        m_openRequests[transactionId] = Simulator::Schedule(MilliSeconds(config::timeout_time_ms), &IndustrialDevice::HandleTimeout, this, std::make_pair(transactionId, remote));
    }
}

void IndustrialDevice::HandleConnectionFailed(Ptr<Socket> socket) {
    CMD_LOG_FUNCTION(this << socket);

    for(auto& pair : m_connectedServers) {
        if(pair.second == socket) {
            CMD_LOG_INFO(m_identifier << ": Connection failed for " << pair.first.GetIpv4() << ":" << pair.first.GetPort());
            m_isConnectedServer[pair.first] = false;
        }
    }
}

void IndustrialDevice::HandleSocketClose(Ptr<Socket> socket) {
    CMD_LOG_FUNCTION(this << socket);

    for(auto& pair : m_connectedServers) {
        if(pair.second == socket) {
            CMD_LOG_INFO(m_identifier << ": Socket closed for " << pair.first.GetIpv4() << ":" << pair.first.GetPort());
            m_isConnectedServer[pair.first] = false;
        }
    }
}

void IndustrialDevice::HandleSocketError(Ptr<Socket> socket) {
    CMD_LOG_FUNCTION(this << socket);

    for(auto it = m_connectedServers.begin(); it != m_connectedServers.end(); it++) {
        if(it->second == socket) {
            InetSocketAddress remote = it->first;
            CMD_LOG_INFO(m_identifier << ": Socket error for " << remote.GetIpv4() << ":" << remote.GetPort() << ". Reconnecting...");
            
            // Close and reconnect
            socket->Close();
            m_connectedServers.erase(it);
            m_isConnectedServer[remote] = false;

            CreateSocket(remote);
            m_connectedServers[remote]->Connect(remote);
            m_isConnectedServer[remote] = true;

            break;
        }
    }
}

void IndustrialDevice::SendData(Ptr<Socket> socket, uint8_t functionCode, std::vector<uint8_t> pduData, uint16_t transactionId) {
    CMD_LOG_FUNCTION(this << socket << functionCode << pduData << transactionId);

    Ptr<Packet> packet = CreateModbusPacket(transactionId, functionCode, pduData);
    socket->Send(packet);

    CMD_LOG_INFO(m_identifier << ": Sent Modbus data: " << hex::toHex(pduData));
}

void IndustrialDevice::ReceiveModbusMessage(Ptr<Socket> socket) {
    CMD_LOG_FUNCTION(this << socket);

    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);

    std::vector<uint8_t> buffer(packet->GetSize());
    packet->CopyData(buffer.data(), packet->GetSize());

    // Logging
    CMD_LOG_INFO(m_identifier << ": Received Modbus message: " << hex::toHex(buffer));

    if (m_recvCallback.IsNull()) {
        CMD_LOG_ERROR(m_identifier << ": No callback registered");
        return;
    }
    if (!InetSocketAddress::IsMatchingType(from)) {
        CMD_LOG_ERROR(m_identifier << ": Could not convert from address to InetSocketAddress. Cannot call receive callback.");
        return;
    }

    // Handle multiple messages combined as one
    uint16_t packetCount = 1;
    while (true) {
        if (buffer.size() < (uint16_t) (ModbusHeader::GetHeaderLength() + 1)) {
            CMD_LOG_ERROR(m_identifier << ": Could not extract packet. Remaining length is not sufficient to include Modbus header and function code (part " << packetCount << ")");
            break;
        }
        // Handle packet
        ModbusHeader modbusHeader;
        packet->RemoveHeader(modbusHeader);
        uint16_t pduLength = modbusHeader.GetPDULength();
        
        // Remove header from buffer
        buffer.erase(buffer.begin(), buffer.begin() + ModbusHeader::GetHeaderLength());

        // Get PDU
        if (pduLength > buffer.size()) {
            CMD_LOG_ERROR(m_identifier << ": Could not extract packet as PDU length is greater than remaining packet size (" << pduLength << " > " << buffer.size() << ") (part " << packetCount << ")");
            break;
        }

        std::vector<uint8_t> pdu(pduLength);
        packet->CopyData(pdu.data(), pduLength);

        // Get functionCode (and remove from PDU)
        uint8_t functionCode = pdu[0];
        pdu.erase(pdu.begin());

        buffer.erase(buffer.begin(), buffer.begin() + pduLength);

        CMD_LOG_INFO(m_identifier << ": Extracted Modbus PDU (packet " << packetCount << ") " << hex::toHex(pdu));

        InetSocketAddress remote = InetSocketAddress::ConvertFrom(from);

        // Retrieve context (might be empty when connection was initiated by remote)
        RequestContext context = m_connectionContext[std::make_pair(modbusHeader.GetTransactionId(),remote)];

        // Only call callback if request has not already timed out
        if (context.GetName() != "timeout") {
            double attackDelay = 0;
            // If attack is installed, let attack change input data
            if(!m_attackMessageHook.IsNull()) {
                attackDelay = m_attackMessageHook(&modbusHeader, &functionCode, &pdu, &context);
            }
            // Negative attackDelay == do not handle message
            if (attackDelay == 0) {
                m_recvCallback(remote, modbusHeader, functionCode, pdu, context);
            } else if (attackDelay > 0) {
                Simulator::Schedule(
                    MilliSeconds(attackDelay),
                    [this, remote, modbusHeader, functionCode, pdu, context]() {
                        m_recvCallback(remote, modbusHeader, functionCode, pdu, context);
                    }
                );
            }
        }

        // Cancel timeout handling
        Simulator::Cancel(m_openRequests[modbusHeader.GetTransactionId()]);

        // Remove context (if it exists)
        m_connectionContext.erase(std::make_pair(modbusHeader.GetTransactionId(),remote));

        // Update buffer and packet for next part
        if (buffer.size() == 0) {
            break;
        }
        packet = Create<Packet>(buffer.data(), buffer.size());
        packetCount++;
    }
}

void IndustrialDevice::SendModbusResponse(InetSocketAddress to, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> message) {
    CMD_LOG_FUNCTION(this << to << modbusHeader << functionCode << message);

    if (m_connectedClients.find(to) == m_connectedClients.end()) {
        CMD_LOG_ERROR(m_identifier << ": No active connection found for client " << to.GetIpv4() << ":" << to.GetPort());
        return;
    }

    Ptr<Socket> socket = m_connectedClients[to];
    uint16_t transactionId = modbusHeader.GetTransactionId();
    double attackDelay = 0;

    if (!m_attackResponseHook.IsNull()) {
        attackDelay = m_attackResponseHook(&to, &modbusHeader, &functionCode, &message);
    }

    // Negative attackDelay == do not send message
    if (attackDelay == 0) {
        SendData(socket, functionCode, message, transactionId);
    } else if (attackDelay > 0) {
        Simulator::Schedule(MilliSeconds(attackDelay), &IndustrialDevice::SendData, this, socket, functionCode, message, transactionId);
    }
}

Ptr<Packet> IndustrialDevice::CreateModbusPacket(const uint16_t transactionId, const uint8_t functionCode, std::vector<uint8_t> pduData) {
    CMD_LOG_FUNCTION(this << transactionId << functionCode << pduData);

    pduData.insert(pduData.begin(), functionCode);
    Ptr<Packet> modbusPacket = Create<Packet>();
    modbusPacket->AddHeader(ModbusHeader(transactionId, pduData.size()));
    modbusPacket->AddAtEnd(Create<Packet>(pduData.data(), pduData.size()));
    return modbusPacket;
}

void IndustrialDevice::StartApplication() {
    CMD_LOG_FUNCTION(this);
    if(IsServer()) {
        CreateSocket(InetSocketAddress(m_node->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 502));
    }
}

void IndustrialDevice::StopApplication() {
    CMD_LOG_FUNCTION(this);
    for(auto& pair : m_connectedServers) {
        pair.second->Close();
    }
    for(auto& pair : m_connectedClients) {
        pair.second->Close();
    }
}

void IndustrialDevice::HandleTimeout(std::pair<uint16_t, InetSocketAddress> contextIndex) {
    CMD_LOG_WARN(m_identifier << ": Request with transactionId " << contextIndex.first << " (" << m_connectionContext[contextIndex].GetName() << ") timed out.");
    m_connectionContext[contextIndex].SetName("timeout");
}