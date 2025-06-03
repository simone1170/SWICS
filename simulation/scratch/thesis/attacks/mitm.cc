#include "mitm.h"
#include "ns3/ipv4.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("MitMAttack");

MitM::MitM(
    Ptr<TestbedHelper> testbed,
    Ptr<IndustrialDevice> device,
    double delay,
    Ptr<IndustrialDevice> target,
    std::function<void(IndustrialDevice*, uint16_t*, uint8_t*, std::vector<uint8_t>*, RequestContext*)> requestHook,
    std::function<void(InetSocketAddress*, ModbusHeader*, uint8_t*, std::vector<uint8_t>*)> responseHook
    ): AttackBase(testbed, device), m_target(target), m_delay(delay), m_requestHook(requestHook), m_responseHook(responseHook) {
}

double MitM::SendModbusRequestHook(IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) {
    if (IsActive()) {
        if (m_requestHook) {
            // Only intercept messages intended for the set target (if set)
            if (m_target == nullptr || m_target == server) {
                m_requestHook(server, port, functionCode, message, context);
            }
        }
        return m_delay;
    }

    return 0;
}

double MitM::SendModbusResponseHook(InetSocketAddress *to, ModbusHeader *modbusHeader, uint8_t *functionCode, std::vector<uint8_t> *message) {
    if (IsActive()) {
        if (m_responseHook) {
            // Only intercept messages intended for the set target (if set)
            if (m_target == nullptr || m_target->GetNode()->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() == to->GetIpv4()) {
                m_responseHook(to, modbusHeader, functionCode, message);
            }
        }
        return m_delay;
    }
    return 0;
}