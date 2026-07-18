/**
 * @file    string_utils.h
 * @brief   字符串工具函数集合
 * @details 提供课程规划系统中常用的字符串处理函数：
 *          - trim: 去除首尾空白
 *          - split: 按分隔符拆分（自动trim每个token）
 *          - safe_stoi / safe_stod: 安全字符串转数字（异常时返回默认值）
 *          - remove_bom: 去除 UTF-8 BOM 头
 *          所有函数均为 header-only inline 实现。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace course_planner::utils {

/**
 * @brief 去除字符串首尾空白字符（空格、制表符、回车等）
 * @param s 输入字符串
 * @return 去除空白后的字符串
 */
inline std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

/**
 * @brief 按分隔符拆分字符串，每个 token 自动 trim
 * @param str 输入字符串
 * @param delimiter 分隔符
 * @return 拆分后的字符串数组（空 token 自动过滤）
 * @note 用于解析分号分隔的先修课ID，逗号分隔的CSV字段等
 */
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

/**
 * @brief 安全地将字符串转为整数，失败时返回默认值
 * @param s 输入字符串
 * @param default_val 转换失败时的默认值
 * @return 转换后的整数或默认值
 */
inline int safe_stoi(const std::string& s, int default_val = 0) {
    try {
        if (s.empty()) return default_val;
        return std::stoi(s);
    } catch (...) {
        return default_val;
    }
}

/**
 * @brief 安全地将字符串转为浮点数，失败时返回默认值
 * @param s 输入字符串
 * @param default_val 转换失败时的默认值
 * @return 转换后的浮点数或默认值
 */
inline double safe_stod(const std::string& s, double default_val = 0.0) {
    try {
        if (s.empty()) return default_val;
        return std::stod(s);
    } catch (...) {
        return default_val;
    }
}

/**
 * @brief 去除 UTF-8 BOM 头（字节序标记 EF BB BF）
 * @param content 文件内容字符串
 * @return 去除 BOM 后的字符串（如果没有 BOM 则原样返回）
 * @note 某些 CSV 编辑器会在文件开头插入 BOM，需要先去除才能正确解析
 */
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