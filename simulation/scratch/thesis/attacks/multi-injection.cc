#include "multi-injection.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("MultiInjectionAttack");

MultiInjectionAttack::MultiInjectionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target, uint8_t functionCode, std::vector<uint8_t> messageA, std::vector<uint8_t> messageB) : AttackBase(testbed, device), m_testbed(testbed), m_target(target), m_functionCode(functionCode), m_messageA(messageA), m_messageB(messageB) {
}

void MultiInjectionAttack::StartAttack() {
    m_packetNum = 0;
    AttackBase::StartAttack();
    CMD_LOG_WARN("Started injection Attack");
    Time startTime = Simulator::Now();
    RequestContext context = RequestContext("request", 0, 0);
    for (uint32_t i = 0; i < m_testbed->GetAttackNumber(); i++) { //Alternate between messages A and B
        if (i % 2 == 0) {
            Simulator::Schedule(startTime, &IndustrialDevice::SendModbusRequest, m_device, m_target, 502, m_functionCode, m_messageA, context);
        } else {
            Simulator::Schedule(startTime, &IndustrialDevice::SendModbusRequest, m_device, m_target, 502, m_functionCode, m_messageB, context);
        }
        startTime += Seconds(m_testbed->GetAttackInterval());
    }
    //m_device->SendModbusRequest(m_target, 502, m_functionCode, m_message);
    Simulator::Schedule(Simulator::Now() + (m_testbed->GetAttackNumber() * Seconds(m_testbed->GetAttackInterval())), &AttackBase::StopAttack, this);
}