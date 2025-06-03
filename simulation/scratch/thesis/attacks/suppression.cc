#include "suppression.h"

using namespace testbed;

SuppressionAttack::SuppressionAttack(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, IndustrialDevice &target) : AttackBase(testbed, device), m_target(target) {

}

double SuppressionAttack::SendModbusRequestHook(IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) {
    if (IsActive()) {
        return -1;
    }
    return 0;
}