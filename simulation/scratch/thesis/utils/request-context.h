#ifndef REQUEST_CONTEXT_H
#define REQUEST_CONTEXT_H

#include <string>
#include "ns3/core-module.h"

using namespace ns3;

namespace testbed {
    /// \brief Stores the context of Modbus requests
    class RequestContext {
    public:
        /// \brief Create context for a Modbus request
        /// \param name Identifier (e.g., device name)
        /// \param address Address requested
        /// \param count Number of coils/registers requested
        RequestContext(std::string name, uint16_t address, uint16_t count) : m_name(name), m_address(address), m_count(count) {};

        /// \brief Standard constructor required for copying objects
        RequestContext() {};
        ~RequestContext() {};

        /// \brief Retrieves the identifier for this request's context
        /// \return the identifier (name) that was set on request
        std::string GetName() { return m_name; };

        /// \brief Retrieves the requested address
        /// \return the requested address
        uint16_t GetAddress() { return m_address; };

        /// \brief Retrieves the count of requested coils/registers
        /// \return the requested count
        uint16_t GetCount() { return m_count; };
        Time GetTime() { return m_time; };

        /// \brief Sets the identifier (name) for this request's context (e.g., device name that was requested)
        /// \param name the name
        void SetName(std::string name) { m_name = name; };

        /// \brief Sets the time of this request
        /// \param time the time
        void SetTime(Time time) { m_time = time; };

        /// \brief Method to allow printing a request context to stdout
        /// \param os the stdout stream
        /// \param rc the RequestContext
        /// \return the stdout stream with this context appended
        friend std::ostream& operator<<(std::ostream& os, const RequestContext& rc) {
            os << "{" << rc.m_name << ": (addr: " << rc.m_address << ", count: " << rc.m_count << ", time: " << rc.m_time << ")}";
            return os;
        }
    private:
        std::string m_name;
        uint16_t m_address;
        uint16_t m_count;
        Time m_time;
    };
};

#endif // REQUEST_CONTEXT_H