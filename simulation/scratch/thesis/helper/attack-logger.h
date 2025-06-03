#ifndef ATTACK_LOGGER_H
#define ATTACK_LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include "command-line-helper.h"
#include "ns3/core-module.h"

using namespace ns3;

/// \brief Class creating the attack log in IPAL format
class AttackLogger {
public:
    /// \brief Retrieve the Logger instance
    /// \return the logger instance
    static AttackLogger& GetInstance() {
        static AttackLogger instance;
        return instance;
    }

    /// \brief Set the logger path. Only possible before initialization
    /// \param path the log file path
    void SetPath(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex);
        if (initialized) {
            std::cerr << "Logger path cannot be changed after initialization!";
        }
        logFilePath = path;
        initialized = true;
        OpenLogFile();
    }

    /// \brief Add a log file entry
    /// \param start start time
    /// \param stop end time
    /// \param attackPoint attack point
    /// \param description attack description
    void Log(Time start, Time stop, std::string attackPoint, std::string description) {
        std::lock_guard<std::mutex> lock(mutex);
        if (id > 1) {
            logFile << "," << std::endl;
        }
        logFile.fill('0');
        logFile << "    {" << std::endl;
        logFile << "        \"id\": " << id << "," << std::endl;
        logFile << "        \"start\": " << std::to_string(start.GetSeconds()) << "," << std::endl;
        logFile << "        \"end\": " << std::to_string(stop.GetSeconds()) << "," << std::endl;
        logFile << "        \"attack_point\": \"" << attackPoint << "\"," << std::endl;
        logFile << "        \"description\": \"" << description << "\"" << std::endl;
        logFile << "    }";

        id++;
    }

private:
    std::ofstream logFile;
    std::string logFilePath = "attacks.json";
    std::mutex mutex;
    uint16_t id = 1;
    bool initialized = false;

    /// \brief Private constructor of the attack logger
    AttackLogger() {
    }

    /// \brief Opens the log file and starts the list
    void OpenLogFile() {
        logFile.open(logFilePath);
        if(!logFile) {
            std::cerr << "Failed to open log file!";
            exit(EXIT_FAILURE);
        }
        logFile << "[" << std::endl;
    }

    /// \brief Private desctructor of AttackLogger
    ~AttackLogger() {
        Cleanup();
    }

    static void Cleanup() {
        AttackLogger& instance = GetInstance();
        std::lock_guard<std::mutex> lock(instance.mutex);

        if (instance.logFile.is_open()) {
            instance.logFile << std::endl;
            instance.logFile << "]" << std::endl;
            instance.logFile.close();
        }
    }
};

#endif // ATTACK_LOGGER_H