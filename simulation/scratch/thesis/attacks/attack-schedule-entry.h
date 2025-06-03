#ifndef ATTACK_SCHEDULE_ENTRY_H
#define ATTACK_SCHEDULE_ENTRY_H

#include "ns3/core-module.h"

using namespace ns3;

namespace testbed {
    /// \brief An attack scheduled for testbed runs including necessary information on start and end time
    class AttackScheduleEntry {
    public:
        /// \brief Information on scheduled attacks
        /// \param attackIdentifier name of the scheduled attack
        /// \param startTime start time of the attack
        /// \param endTime end time of the attack
        AttackScheduleEntry(std::string attackIdentifier, Time startTime, Time endTime) : m_attackIdentifier(attackIdentifier), m_startTime(startTime), m_endTime(endTime) {}
        ~AttackScheduleEntry() {}

        /// \brief Retrieves the attack name
        /// \return the attack name
        std::string GetAttackIdentifier() { return m_attackIdentifier; };

        /// \brief Retrieves the attack start time
        /// \return the attack start time
        Time GetStartTime() { return m_startTime; };

        /// \brief Retrieves the attack stop time
        /// \return the attack stop time
        Time GetEndTime() { return m_endTime; };
    private:
        std::string m_attackIdentifier;
        Time m_startTime;
        Time m_endTime;
    };
}

#endif // ATTACK_SCHEDULE_ENTRY_H