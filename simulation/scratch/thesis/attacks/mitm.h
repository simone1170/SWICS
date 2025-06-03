#ifndef MITM_H
#define MITM_H

#include "attack-base.h"

using namespace ns3;

namespace testbed {
    /// \brief Implementation of a simplified Man-in-the-Middle attack
    class MitM : public AttackBase {
    public:
        /// \brief Instantiates a new Man-in-the-Middle attack
        /// \param testbed the testbed on which the attack is meant to be run
        /// \param device the compromised device
        /// \param delay delay to be added to messages simulating a Man-in-the-Middle
        /// \param target target of the attack
        /// \param requestHook callback method for requests
        /// \param responseHook callback method for responses
        MitM(Ptr<TestbedHelper> testbed, Ptr<IndustrialDevice> device, double delay, Ptr<IndustrialDevice> target = nullptr, std::function<void(IndustrialDevice*, uint16_t*, uint8_t*, std::vector<uint8_t>*, RequestContext*)> requestHook = std::function<void(IndustrialDevice*, uint16_t*, uint8_t*, std::vector<uint8_t>*, RequestContext*)>(), std::function<void(InetSocketAddress*, ModbusHeader*, uint8_t*, std::vector<uint8_t>*)> responseHook = std::function<void(InetSocketAddress*, ModbusHeader*, uint8_t*, std::vector<uint8_t>*)>());
        ~MitM() {};

        /// \brief Callback method to intercept requests by the attack target and (possibly) change the outcome
        /// \param server target of the intended message
        /// \param port port of the intended message
        /// \param functionCode function code of the intended message
        /// \param message message body of the intended message
        /// \param context context set by the sender to correlate responses to this request
        /// \return delay in ms before this message is sent (-1 if not to be sent)
        double SendModbusRequestHook(IndustrialDevice *server, uint16_t *port, uint8_t *functionCode, std::vector<uint8_t> *message, RequestContext *context) override;

        /// \brief Callback method to intercept responses from the attack target and (possibly) change them
        /// \param modbusHeader the header containing response information
        /// \param functionCode the function code of the response
        /// \param message the message body of the response
        /// \return delay in ms before this response is sent by the attack target (-1 if never to be sent)
        double SendModbusResponseHook(InetSocketAddress *to, ModbusHeader *modbusHeader, uint8_t *functionCode, std::vector<uint8_t> *message) override;
    private:
        Ptr<IndustrialDevice> m_target;
        double m_delay;

        std::function<void(IndustrialDevice*, uint16_t*, uint8_t*, std::vector<uint8_t>*, RequestContext*)> m_requestHook;
        std::function<void(InetSocketAddress*, ModbusHeader*, uint8_t*, std::vector<uint8_t>*)> m_responseHook;

        virtual std::string GetAttackPoint() override { return m_target != nullptr ? m_target->GetIdentifier() : "all"; };
        virtual std::string GetDescription() override { return "MitM attack with a delay of " + std::to_string(m_delay); };
    };
}

#endif // MITM_H