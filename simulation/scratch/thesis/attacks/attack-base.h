#ifndef ATTACK_BASE_H
#define ATTACK_BASE_H

#include "ns3/core-module.h"
#include "../helper/testbed-helper.h"
#include "../model/industrial-device.h"

using namespace ns3;

namespace testbed {
    /// \brief Base class for attack implementations including attack logging, callback templates, and attack control
    class AttackBase : public Object {
    public:
        /// \brief Instantiates a new AttackBase
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        AttackBase(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device);
        ~AttackBase();

        /// \brief Starts the attack
        void StartAttack();

        /// \brief Stops the attack
        void StopAttack();

        /// \brief Method to retrieve whether the attack is active
        /// \return whether the attack is active
        bool IsActive() const { return m_active; };

        /// \brief Callback method to intercept requests by the attack target and (possibly) change the outcome
        /// \param server target of the intended message
        /// \param port port of the intended message
        /// \param functionCode function code of the intended message
        /// \param message message body of the intended message
        /// \param context context set by the sender to correlate responses to this request
        /// \return delay in ms before this message is sent (-1 if not to be sent)
        virtual double SendModbusRequestHook(IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) { return 0; };

        /// \brief Callback method to intercept responses from the attack target and (possibly) change them
        /// \param modbusHeader the header containing response information
        /// \param functionCode the function code of the response
        /// \param message the message body of the response
        /// \return delay in ms before this response is sent by the attack target (-1 if never to be sent)
        virtual double SendModbusResponseHook(InetSocketAddress *to, ModbusHeader *modbusHeader, uint8_t *functionCode, std::vector<uint8_t> *message) { return 0; };

        /// \brief Callback method to intercept messages to the attack target and (possibly) change them
        /// \param header the header containing message information
        /// \param functionCode the function code of the message
        /// \param data message body of the message
        /// \param context context of the message
        /// \return delay in ms before this response is handled by the attack target (-1 if never to be handled)
        virtual double ReceiveModbusMessageHook(ModbusHeader *header, uint8_t *functionCode, std::vector<uint8_t> *data, RequestContext *context) { return 0; };
    protected:
        /// \brief the attack target device
        Ptr<IndustrialDevice> m_device;
    private:
        bool m_active = false;

        // Parameters for the attacks.json
        Ptr<TestbedHelper> m_testbed;

        Time m_startTime;
        virtual std::string GetDescription() = 0;
        virtual std::string GetAttackPoint() = 0;
    };
}

#endif // ATTACK_BASE_H