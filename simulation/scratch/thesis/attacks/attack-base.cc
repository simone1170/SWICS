#include <iomanip>
#include "attack-base.h"
#include "../helper/command-line-helper.h"
#include "../helper/attack-logger.h"

using namespace testbed;

NS_LOG_COMPONENT_DEFINE("AttackBase");

AttackBase::AttackBase(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device) : m_device(device), m_testbed(testbed) {}

AttackBase::~AttackBase() {
}

void AttackBase::StartAttack() {
    if (!m_active) {
        // Install callbacks
        m_device->m_attackRequestHook = MakeCallback(&AttackBase::SendModbusRequestHook, this);
        m_device->m_attackResponseHook = MakeCallback(&AttackBase::SendModbusResponseHook, this);
        m_device->m_attackMessageHook = MakeCallback(&AttackBase::ReceiveModbusMessageHook, this);

        m_startTime = Simulator::Now();
        m_active = true;
        CMD_LOG_INFO("Starting attack " << GetDescription());
    } else {
        CMD_LOG_WARN("Attack is already running.");
    }
}

void AttackBase::StopAttack() {
    if (m_active) {
        Time stopTime = Simulator::Now();
        m_active = false;

        if (m_testbed->CreateLogFilesEnabled()) {
            AttackLogger::GetInstance().Log(m_startTime, stopTime, GetAttackPoint(), GetDescription());
        }
        CMD_LOG_INFO("Stopping attack " << GetDescription());
    }
}