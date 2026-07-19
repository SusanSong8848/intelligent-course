/**
 * @file    string_utils.h
 * @brief   字符串工具函数集合
 * @details 提供课程规划系统中常用的字符串处理函数：
 *          - trim: 去除首尾空白
 *          - split: 按分隔符拆分（自动trim每个token）
 *          - safe_stoi / safe_stod: 安全字符串转数字（异常时返回默认值）
 *          - remove_bom: 去除 UTF-8 BOM 头
 *          所有函数均为 header-only inline 实现。      //header-only：指这个库的实现全部写在 .h 头文件里，没有对应的 .cpp 文件。我只要 #include 头文件，就能直接用，不需要链接额外的库。
 *                                                     
 *                                                      //这涉及到 C++ 的“单一定义规则”（One Definition Rule, ODR）。
 *                                                       当在头文件里定义一个函数（而不只是声明），比如 void foo() { ... }，如果这个头文件被两个 .cpp 文件 #include，那就相当于在两个翻译单元里都出现了这个函数的定义。
 *                                                       编译器分别编译 a.cpp 和 b.cpp 不会报错，但链接器在把生成的目标文件合并成一个可执行文件时，会看到两个一模一样的函数实现，它不知道该用哪个，于是报“multiple definition”错误。
 *                                                       加上 inline 后，C++ 标准允许内联函数在多个翻译单元中定义，链接器会接受它，并且只保留一份副本。因此 header-only 库里的函数必须标记 inline。
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
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {        /*inline：关键字，建议编译器把函数的代码直接“粘贴”到调用它的地方，而不是做函数调用。对于非常简单、频繁调用的函数（比如 trim, safe_stoi），这样能提高一点点速度。
                                                                                    此外，在 header-only 的情况下，如果函数定义在头文件中，必须加 inline，否则多个 .cpp 包含它时会报“重复定义”错误。这是规定。*/
        return std::isspace(ch);        //不是空白的位置
    });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {    //r : 反向找
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
inline std::string remove_bom(const std::string& content) {     /*UTF-8 BOM 是某些软件（比如 Windows 记事本）在保存 UTF-8 文本文件时，偷偷在文件最前面塞的三个特殊字节：EF BB BF。肉眼看不见，但会影响程序解析（比如被当成你数据的第一行第一列）。*/
    if (content.size() >= 3
        && static_cast<unsigned char>(content[0]) == 0xEF
        && static_cast<unsigned char>(content[1]) == 0xBB
        && static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }
    return content;
}

}  // namespace course_planner::utils