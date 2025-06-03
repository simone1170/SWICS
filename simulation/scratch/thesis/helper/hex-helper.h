#ifndef HEX_HELPER_H
#define HEX_HELPER_H

#include <vector>

/// \brief Helper methods to convert uint8_t and std::vector<uint8_t> to hex strings
namespace hex {
    /// \brief Helper array to simply convert values 0..15 to hex
    static const char hex_lookup[] = "0123456789ABCDEF";

    /// \brief Conversion of one byte to a hex string
    static std::string toHex(uint8_t data) {
        std::string result = "0x";
        result.push_back(hex_lookup[data >> 4]);
        result.push_back(hex_lookup[data & 0xF]);
        return result;
    }

    /// \brief Conversion of a byte vector to a hex string
    static std::string toHex(std::vector<uint8_t> data) {
        std::string result;
        for(uint8_t byte : data) {
            result += toHex(byte);
            result += " ";
        }
        return result;
    }
}

#endif