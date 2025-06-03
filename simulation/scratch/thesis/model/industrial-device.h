#ifndef INDUSTRIAL_DEVICE_H
#define INDUSTRIAL_DEVICE_H

#include <map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/packet.h"

#include "../utils/modbus-header.h"
#include "../utils/request-context.h"

using namespace ns3;

/// \brief Implementation of a testbed framework for Modbus TCP/IP communication over 5G and wired communication channels;
/// Generate datasets for, both, 5G and wired communication to evaluate differences in attack detection by IDSs
///
/// Supports the definition of an underlying physical process, instantiation of industrial devices and implementation of control logic by extending the PLC base class
namespace testbed {
    /// \brief Base class providing modbus functions for all industrial devices (PLCs, Actuators, ...)
    class IndustrialDevice : public Application {
    public:
        /// \brief Instantiates an IndustrialDevice
        /// \param identifier Name to identify this device in logs, pcap etc.
        /// \param isServer Whether this device should listen to requests on Modbus port 502
        IndustrialDevice(std::string identifier, bool isServer);
        ~IndustrialDevice();

        /// \brief Retrieve a socket corresponding to an open connection with the remote host specified
        /// \param remote Address and port of remote host
        /// \return The socket of an open connection or newly created socket for this connection
        Ptr<Socket> GetSocket(InetSocketAddress remote);

        /// \brief Whether this device is a server
        /// \return Whether this device is a server
        bool IsServer();

        /// \brief Retrieves the IndustrialDevice's identifier
        /// \return the name of this device
        std::string GetIdentifier();

        /// \brief Send a modbus request to another device (server)
        /// \param server IndustrialDevice object of the server
        /// \param port Port of the server (typically 502)
        /// \param functionCode Modbus function code of the request
        /// \param message Modbus PDU of the request (excluding functionCode)
        /// \param context Context to be stored for this request (retrieved when getting a response)
        void SendModbusRequest(IndustrialDevice& server, uint16_t port, const uint8_t functionCode, const std::vector<uint8_t> message, RequestContext context = RequestContext("request", 0, 0));

        /// \brief Send a modbus response to a previous request
        /// \param to Address of the recipient
        /// \param modbusHeader ModbusHeader from the request
        /// \param functionCode Modbus functionCode of the response (same as request or +0x80 on error)
        /// \param message Modbus PDU to send to recipient
        void SendModbusResponse(InetSocketAddress to, ModbusHeader modbusHeader, uint8_t functionCode, std::vector<uint8_t> message);


        // Attack hooks (set by attack when installed on device)
        Callback<double, IndustrialDevice *, uint16_t *, uint8_t *, std::vector<uint8_t> *, RequestContext *> m_attackRequestHook;
        Callback<double, InetSocketAddress *, ModbusHeader *, uint8_t *, std::vector<uint8_t> *> m_attackResponseHook;
        Callback<double, ModbusHeader *, uint8_t *, std::vector<uint8_t> *, RequestContext *> m_attackMessageHook;

    protected:
        /// \brief Name to identify the IndustrialDevice in logs, pcap etc.
        std::string m_identifier;

        /// \brief Sets the callback method to be called when receiving modbus data
        /// \param cb The callback method to be called when receiving modbus data
        void SetRecvCallback(Callback<void, InetSocketAddress, ModbusHeader, uint8_t, std::vector<uint8_t>, RequestContext>  cb);

        /// \brief Send data to a socket
        /// \param socket Socket to send data to
        /// \param functionCode Modbus function code of message
        /// \param pduData Modbus PDU data (excluding functionCode)
        /// \param transactionId TransactionId of this request (if response same as request)
        void SendData(Ptr<Socket> socket, const uint8_t functionCode, const std::vector<uint8_t> pduData, uint16_t transactionId);

        /// \brief Method to start the IndustrialDevice application and call corresponding method of Application
        void StartApplication() override;

        /// \brief Method to stop the IndustrialDevice application and call corresponding method of Application
        void StopApplication() override;

        /// \brief Method handling Modbus request timeouts. Method is called when timeout time has been reached. This method can be overridden by implementations. Sub-classes should always call parent implementation.
        /// \param contextIndex the index of context for the request in m_connectionContext (transactionId, InetSocketAddress)
        virtual void HandleTimeout(std::pair<uint16_t,InetSocketAddress> contextIndex);

    private:
        /// \brief Store sockets of open connections with Modbus servers
        std::map<InetSocketAddress, Ptr<Socket>> m_connectedServers;
        /// \brief Store sockets of open connections with Modbus clients (as server)
        std::map<InetSocketAddress, Ptr<Socket>> m_connectedClients;
        /// \brief Store whether connections are still open with Modbus server
        std::map<InetSocketAddress, bool> m_isConnectedServer;
        /// \brief Store whether connections are still open with Modbus client
        std::map<InetSocketAddress, bool> m_isConnectedClient;
        /// \brief Store context that can be retrieved when receiving a response (e.g., where to store requested data)
        std::map<std::pair<uint16_t,InetSocketAddress>, RequestContext> m_connectionContext; // Store context for requests made for transactionId and remote address
        /// \brief The callback method to be called when receiving modbus data
        Callback<void, InetSocketAddress, ModbusHeader, uint8_t, std::vector<uint8_t>, RequestContext> m_recvCallback;
        /// \brief Timeout methods for open requests
        std::map<uint16_t, EventId> m_openRequests;
        /// \brief Whether this device is a server
        bool m_isServer;
        /// \brief Counter for the current transactionId
        uint16_t m_transactionId = 0;

        void CreateSocket(InetSocketAddress remote);
        void HandleAccept(Ptr<Socket> socket, const Address& from);
        void HandleConnectionFailed(Ptr<Socket> socket);
        void HandleSocketClose(Ptr<Socket> socket);
        void HandleSocketError(Ptr<Socket> socket);
        void ReceiveModbusMessage(Ptr<Socket> socket);
        Ptr<Packet> CreateModbusPacket(const uint16_t transactionId, const uint8_t functionCode, std::vector<uint8_t> pduData);
    };
}

#endif