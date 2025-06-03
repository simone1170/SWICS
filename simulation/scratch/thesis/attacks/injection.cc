#include "injection.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("InjectionAttack");

InjectionAttack::InjectionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> message) : AttackBase(testbed, device), m_target(target), m_functionCode(functionCode), m_message(message) {

}

void InjectionAttack::StartAttack() {
    AttackBase::StartAttack();
    CMD_LOG_WARN("Started injection Attack");
    m_device->SendModbusRequest(m_target, 502, m_functionCode, m_message);
    AttackBase::StopAttack();
}