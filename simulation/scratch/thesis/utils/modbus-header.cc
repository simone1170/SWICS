#include "modbus-header.h"

using namespace testbed;

ModbusHeader::ModbusHeader(const uint16_t transactionId, const uint16_t dataLength) : m_transactionId(transactionId), m_length(dataLength+1) {}
ModbusHeader::ModbusHeader() {}

TypeId ModbusHeader::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::ModbusHeader")
        .SetParent<Header>()
        .SetGroupName("Tutorial")
        .AddConstructor<ModbusHeader>();
    return tid;
}

TypeId ModbusHeader::GetInstanceTypeId(void) const {
    return GetTypeId();
}

void ModbusHeader::Print(std::ostream &os) const {
    os << "Transaction ID: " << m_transactionId
        << ", Protocol ID: " << m_protocolId
        << ", Length: " << m_length
        << ", Unit ID: " << m_unitId;
}

uint32_t ModbusHeader::GetSerializedSize() const {
    return 7;
}

void ModbusHeader::Serialize(Buffer::Iterator start) const {
    start.WriteHtonU16(m_transactionId);
    start.WriteHtonU16(m_protocolId);
    start.WriteHtonU16(m_length);
    start.WriteU8(m_unitId);
}

uint32_t ModbusHeader::Deserialize(Buffer::Iterator start) {
    m_transactionId = start.ReadNtohU16();
    m_protocolId = start.ReadNtohU16();
    m_length = start.ReadNtohU16();
    m_unitId = start.ReadU8();

    return GetSerializedSize();
}