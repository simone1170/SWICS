#ifndef MODBUS_HEADER_H
#define MODBUS_HEADER_H

#include "ns3/header.h"

using namespace ns3;

namespace testbed {
    class ModbusHeader : public Header {
    public:
        ModbusHeader();
        ModbusHeader(const uint16_t transactionId, const uint16_t dataLength);

        ~ModbusHeader() {};

        static TypeId GetTypeId(void);

        TypeId GetInstanceTypeId(void) const override;

        void Print(std::ostream &os) const override;

        uint32_t GetSerializedSize() const override;

        void Serialize(Buffer::Iterator start) const override;

        uint32_t Deserialize(Buffer::Iterator start) override;

        uint16_t GetTransactionId() { return m_transactionId; };

        uint16_t GetProtocolId() { return m_protocolId; };

        uint8_t GetUnitId() { return m_unitId; };

        uint16_t GetPDULength() { return m_length - sizeof(m_unitId); };

        static uint16_t GetHeaderLength() { return sizeof(m_transactionId) + sizeof(m_protocolId) + sizeof(m_length) + sizeof(m_unitId); };

    private:
        uint16_t m_transactionId;
        uint16_t m_protocolId = 0x0000;
        uint16_t m_length;
        uint8_t m_unitId = 0xFF;
    };
}

#endif // MODBUS_HEADER_H