#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace course_planner::utils {

/// 去除字符串首尾空白字符
inline std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

/// 按分隔符拆分字符串
inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    for (char ch : str) {
        if (ch == delimiter) {
            token = trim(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
            token.clear();
        } else {
            token += ch;
        }
    }
    token = trim(token);
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

/// 安全的字符串转整数
inline int safe_stoi(const std::string& s, int default_val = 0) {
    try {
        if (s.empty()) return default_val;
        return std::stoi(s);
    } catch (...) {
        return default_val;
    }
}

/// 安全的字符串转浮点数
inline double safe_stod(const std::string& s, double default_val = 0.0) {
    try {
        if (s.empty()) return default_val;
        return std::stod(s);
    } catch (...) {
        return default_val;
    }
}

/// 去除UTF-8 BOM头（EF BB BF）
inline std::string remove_bom(const std::string& content) {
    if (content.size() >= 3
        && static_cast<unsigned char>(content[0]) == 0xEF
        && static_cast<unsigned char>(content[1]) == 0xBB
        && static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }
    return content;
}

}  // namespace course_planner::utils