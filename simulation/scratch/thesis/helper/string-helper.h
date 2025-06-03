#ifndef STRING_HELPER_H
#define STRING_HELPER_H

#include <vector>

namespace StringHelper {
    /// \brief Vector to String conversion with set delimiter
    static inline std::string SerializeVector(std::vector<std::string> vector, char delimiter) {
        std::string res;
        for (size_t i = 0; i < vector.size(); ++i) {
            res += vector[i];
            if (i != vector.size() - 1) {
                res += delimiter;
            }
        }
        return res;
    }

    /// \brief String to Vector conversion (splitting) with set delimiter
    static inline std::vector<std::string> StringToVector(std::string string, char delimiter) {
        std::string temp = string;
        int pos = 0;
        std::vector<std::string> res;

        while (pos != -1) {
            pos = temp.find(delimiter);
            res.push_back(temp.substr(0,pos));
            temp.erase(0,pos+1);
        }

        return res;
    }
}

#endif // STRING_HELPER_H