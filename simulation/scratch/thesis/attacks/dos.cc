#include "dos.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("DoS");

DoS::DoS(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> message) : AttackBase(testbed, device), m_target(target), m_functionCode(functionCode), m_message(message) {

};

void DoS::StartAttack() {
    AttackBase::StartAttack();
    CMD_LOG_WARN("Started DoS Attack");
    RunAttack();
}

void DoS::RunAttack() {
    if (IsActive()) {
        m_device->SendModbusRequest(m_target, 502, m_functionCode, m_message);
        Simulator::Schedule(MicroSeconds(800), &DoS::RunAttack, this);
    }
}
