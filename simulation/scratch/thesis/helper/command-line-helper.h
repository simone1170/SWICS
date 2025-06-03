#ifndef COMMAND_LINE_HELPER_H
#define COMMAND_LINE_HELPER_H

#include <ostream>
#include "ns3/log.h"
#include "ns3/core-module.h"

/// \brief The Cmd namespace provides custom logging capabilities
///
/// This namespace includes helper functionality to include component names and current simulation time in log messages.
/// Helper methods to format console output is also provided.
namespace Cmd {
    /// \brief Provides console color codes for formatted logging messages
    enum Code {
        CLR_RED = 31,
        CLR_GREEN = 32,
        CLR_ORANGE = 33,
        CLR_BLUE = 34,
        CLR_RESET = 39,
        CLR_GREY = 90,
        MOD_BOLD = 1,
        MOD_UNDERLINE = 4,
        RESET = 0,
    };

    /// \brief Provides modification method for formatting console output
    class Mod {
        Code m_code;
    public:
        Mod(Code p_code) : m_code(p_code) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Mod& mod) {
            return os << "\033[" << mod.m_code << "m";
        }
    };

    template <typename K, typename V>
    static std::string Preprocess(std::unordered_map<K, V>& map) {
        std::ostringstream oss;
        oss << "{ ";
        for (auto it = map.begin(); it != map.end(); it++) {
            oss << it->first << ": " << it->second;
            if (std::next(it) != map.end()) {
                oss << ", ";
            }
        }
        oss << " }";
        return oss.str();
    }

    template <typename K, typename V>
    static std::string Preprocess(const std::pair<K, V> pair) {
        std::ostringstream oss;
        oss << "(" << pair.first << ", " << pair.second << ")";
        return oss.str();
    }
}

/// \brief A custom macro to add component names and current simulation time to the output
#define CMD_LOG_PREFIX Cmd::Mod(Cmd::MOD_BOLD) << "[" << g_log.Name() << "] " << Cmd::Mod(Cmd::RESET) << "(" << (Simulator::Now().GetSeconds()) << "s) "

/// \brief A custom macro adding [ERROR] in red to error messages in the console output
#define CMD_LOG_ERROR_PREFIX Cmd::Mod(Cmd::CLR_RED) << "[ERROR] " << Cmd::Mod(Cmd::CLR_RESET)
/// \brief A custom macro logging formatted error messages if component logging is enabled
#define CMD_LOG_ERROR(msg)                                                          \
    if (g_log.IsEnabled(LOG_ERROR)) {                                               \
        std::cerr << CMD_LOG_ERROR_PREFIX << CMD_LOG_PREFIX << msg << std::endl;    \
    }

/// \brief A custom macro adding [WARN] in orange to error messages in the console output
#define CMD_LOG_WARN_PREFIX Cmd::Mod(Cmd::CLR_ORANGE) << "[WARN] " << Cmd::Mod(Cmd::CLR_RESET)
/// \brief A custom macro logging formatted warning messages if component logging is enabled
#define CMD_LOG_WARN(msg)                                                          \
    if (g_log.IsEnabled(LOG_WARN)) {                                               \
        std::cerr << CMD_LOG_WARN_PREFIX << CMD_LOG_PREFIX << msg << std::endl;    \
    }

/// \brief A custom macro adding [INFO] in blue to error messages in the console output
#define CMD_LOG_INFO_PREFIX Cmd::Mod(Cmd::CLR_BLUE) << "[INFO] " << Cmd::Mod(Cmd::CLR_RESET)
/// \brief A custom macro logging formatted info messages if component logging is enabled
#define CMD_LOG_INFO(msg)                                                           \
    if (g_log.IsEnabled(LOG_INFO)) {                                                \
        std::cerr << CMD_LOG_INFO_PREFIX << CMD_LOG_PREFIX << msg << std::endl;     \
    }

/// \brief A custom macro adding [FUNC] in grey to error messages in the console output
#define CMD_LOG_FUNCTION_PREFIX Cmd::Mod(Cmd::CLR_GREY) << "[FUNC] " << Cmd::Mod(Cmd::CLR_RESET)
#define CMD_LOG_FUNCTION(msg)                                                       \
    if (g_log.IsEnabled(LOG_FUNCTION)) {                                            \
        std::clog << CMD_LOG_FUNCTION_PREFIX << CMD_LOG_PREFIX;                     \
        std::clog << g_log.Name() << ":" << __FUNCTION__ << "(";                    \
        auto flags = std::clog.setf(std::ios_base::boolalpha);                      \
        ns3::ParameterLogger(std::clog) << msg;                                     \
        std::clog.flags(flags);                                                     \
        std::clog << ")" << std::endl;                                              \
    }

#endif // COMMAND_LINE_HELPER_H