/**
 * @file    json_parser.h
 * @brief   轻量级 JSON 解析器（零外部依赖，内嵌实现）
 * @details 为课程规划系统提供自给自足的 JSON 解析能力，无需下载任何第三方库。
 *
 *          主要组件：
 *          - JsonValue: 动态类型的 JSON 值容器，支持 object/array/string/number/bool/null      //*重点（1）
 *          - JsonParser: 递归下降 JSON 解析器，支持嵌套结构和 Unicode 转义                      //*重点（2）
 *          - parse_json(): 便捷解析函数
 *
 *          设计决策：
 *          - 选择自实现而非 nlohmann/json，避免 FetchContent 网络下载失败问题
 *          - 使用 C++17 std::variant 存储多态值
 *          - number 类型区分 int64_t 和 double 以精确表示 JSON 中的整数和浮点数
 *
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <stdexcept>
#include <cctype>
#include <cstdlib>
#include <cstdint>

namespace course_planner::utils {

/**
 * @brief 动态类型 JSON 值容器
 *
 * 支持的类型: Null, Bool, Int, Double, String, Array, Object
 * 使用 C++17 std::variant 内部存储，通过 type() 获取当前类型，
 * 通过 get_xxx() 方法获取对应类型的值。
 */
class JsonValue {
public:
    enum Type { NUL, BOOL, INT, DOUBLE, STRING, ARRAY, OBJECT };

    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

private:
    Type type_ = NUL;
    std::variant<bool, int64_t, double, std::string, Array, Object> value_; //键值对的值     
/*std::variant<bool, int64_t, double, std::string, Array, Object> value_; 是啥意思？
先看 JsonValue 类的基本思路：
JSON 里的一个值可能是字符串、数字、布尔、数组、对象等各种类型。C++ 是强类型语言，一个变量不能一会儿存 int，一会儿存 string。
于是我们用了一个 C++17 的新武器：std::variant。

std::variant 就像一个类型安全的 union。它内部可以存储你列出的多种类型中的某一种，但一次只能存一种。我可以查询它当前是什么类型，然后取出对应类型的值。

这里列出的类型：bool（布尔），int64_t（64 位整数），double（浮点数），std::string（字符串），Array（即 std::vector<JsonValue>），Object（即 std::map<std::string, JsonValue>）。基本覆盖了 JSON 所有可能的类型。

int64_t 是固定宽度 64 位有符号整数，在 <cstdint> 里定义，保证在所有平台上都是 64 位，可存很大的整数而不会溢出。

所以 value_ 这个成员变量可以存储任意一种 JSON 数据类型，type_ 记录当前存的是哪一种。这就是多态的一种实现方式。*/

public:
    JsonValue() : type_(NUL), value_(false) {}
    JsonValue(bool v) : type_(BOOL), value_(v) {}
    JsonValue(int64_t v) : type_(INT), value_(v) {}
    JsonValue(double v) : type_(DOUBLE), value_(v) {}
    JsonValue(const std::string& v) : type_(STRING), value_(v) {}
    JsonValue(const char* v) : type_(STRING), value_(std::string(v)) {}
    JsonValue(const Array& v) : type_(ARRAY), value_(v) {}
    JsonValue(const Object& v) : type_(OBJECT), value_(v) {}

    Type type() const { return type_; }
    bool is_null() const { return type_ == NUL; }

    bool get_bool() const { return std::get<bool>(value_); }
    int64_t get_int() const {
        if (type_ == INT) return std::get<int64_t>(value_);
        if (type_ == DOUBLE) return static_cast<int64_t>(std::get<double>(value_));
        throw std::runtime_error("JsonValue is not a number");
    }
    double get_double() const {
        if (type_ == DOUBLE) return std::get<double>(value_);
        if (type_ == INT) return static_cast<double>(std::get<int64_t>(value_));
        throw std::runtime_error("JsonValue is not a number");
    }
    std::string get_string() const { return std::get<std::string>(value_); }

    const Array& get_array() const { return std::get<Array>(value_); }
    const Object& get_object() const { return std::get<Object>(value_); }

    /** @brief 检查 object 中是否存在指定 key */
    bool contains(const std::string& key) const {
        if (type_ != OBJECT) return false;
        return std::get<Object>(value_).count(key) > 0;
    }

    /** @brief 按 key 访问 object 成员（不存在时返回静态空值） */
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_val;
        const auto& obj = std::get<Object>(value_);     /*std::get<Object>(value_) 的意思是：“从 value_ 中取出 Object 类型的值（Object 是 std::map<std::string, JsonValue> 的别名）”。
                                                            如果当前 value_ 实际存储的不是 Object，程序会抛出异常。*/
                                                        //const course_planner::utils::JsonValue::Object &obj
                                                        //using Object = std::map<std::string, JsonValue>;  //这里相当于先找到Object，再找到对应的
                                                        
        auto it = obj.find(key);
        if (it != obj.end()) return it->second;     //pair{string, JsonValue} -> second，只有找到整个pair{key, JsonValue} 才能找到 key 匹配的JsonValue
        return null_val;
    }

    /** @brief 按下标访问 array 元素 */
    const JsonValue& operator[](size_t index) const {
        return std::get<Array>(value_).at(index);
    }

    size_t size() const {
        if (type_ == ARRAY) return std::get<Array>(value_).size();
        if (type_ == OBJECT) return std::get<Object>(value_).size();
        return 0;
    }

    /** @brief 按 key 获取 string 值，不存在时返回默认值 */
    std::string value(const std::string& key, const char* default_val) const {
        if (contains(key)) return (*this)[key].get_string();
        return default_val;
    }
    /** @brief 按 key 获取 int 值，不存在时返回默认值 */
    int value(const std::string& key, int default_val) const {
        if (contains(key)) return (*this)[key].get_int();
        return default_val;
    }
    /** @brief 按 key 获取 double 值，不存在时返回默认值 */
    double value(const std::string& key, double default_val) const {
        if (contains(key)) return (*this)[key].get_double();
        return default_val;
    }
    /** @brief 按 key 获取 bool 值，不存在时返回默认值 */
    bool value(const std::string& key, bool default_val) const {
        if (contains(key)) return (*this)[key].get_bool();
        return default_val;
    }
};

/**
 * @brief 递归下降 JSON 解析器
 *
 * 从 JSON 文本字符串构建 JsonValue 树。
 * 支持标准 JSON 语法：对象、数组、字符串、数字、布尔、null。
 * 支持 Unicode 转义 \\uXXXX（转为 UTF-8 编码）。
 */
class JsonParser {
public:
    /**
     * @brief 解析 JSON 字符串并返回 JsonValue
     * @param json_str JSON 格式的字符串
     * @return 解析后的 JsonValue
     * @throws std::runtime_error 如果 JSON 格式非法
     */
    static JsonValue parse(const std::string& json_str) {
        size_t pos = 0;
        skip_ws(json_str, pos);
        auto val = parse_value(json_str, pos);
        skip_ws(json_str, pos);
        if (pos != json_str.size()) {
            throw std::runtime_error("Extra characters after JSON value at position " + std::to_string(pos));
        }
        return val;
    }

private:
    static void skip_ws(const std::string& s, size_t& pos) {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) {
            ++pos;
        }
    }

    static char peek(const std::string& s, size_t pos) {
        if (pos < s.size()) return s[pos];
        return '\0';
    }

    static void expect(const std::string& s, size_t& pos, char c) {
        if (pos >= s.size() || s[pos] != c) {
            throw std::runtime_error("Expected '" + std::string(1, c) + "' at position " + std::to_string(pos));
        }
        ++pos;
    }

    static JsonValue parse_value(const std::string& s, size_t& pos);
    static JsonValue parse_object(const std::string& s, size_t& pos);
    static JsonValue parse_array(const std::string& s, size_t& pos);
    static std::string parse_string_raw(const std::string& s, size_t& pos);
    static JsonValue parse_string(const std::string& s, size_t& pos);
    static JsonValue parse_number(const std::string& s, size_t& pos);
    static JsonValue parse_bool(const std::string& s, size_t& pos);
    static JsonValue parse_null(const std::string& s, size_t& pos);
};

// ============================================================================
// JsonParser 内联实现
// ============================================================================

inline JsonValue JsonParser::parse_value(const std::string& s, size_t& pos) {
    skip_ws(s, pos);
    if (pos >= s.size()) throw std::runtime_error("Unexpected end of JSON");

    char c = s[pos];
    if (c == '{') return parse_object(s, pos);
    if (c == '[') return parse_array(s, pos);
    if (c == '"') return parse_string(s, pos);
    if (c == 't' || c == 'f') return parse_bool(s, pos);
    if (c == 'n') return parse_null(s, pos);
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number(s, pos);

    throw std::runtime_error("Unexpected character '" + std::string(1, c) + "' at position " + std::to_string(pos));
}

inline JsonValue JsonParser::parse_object(const std::string& s, size_t& pos) {
    expect(s, pos, '{');
    JsonValue::Object obj;
    skip_ws(s, pos);
    if (peek(s, pos) == '}') {
        ++pos;
        return JsonValue(obj);
    }

    while (true) {
        skip_ws(s, pos);
        std::string key = parse_string_raw(s, pos);
        skip_ws(s, pos);
        expect(s, pos, ':');
        skip_ws(s, pos);
        obj[key] = parse_value(s, pos);
        skip_ws(s, pos);
        if (peek(s, pos) == ',') {
            ++pos;
            continue;
        }
        if (peek(s, pos) == '}') {
            ++pos;
            break;
        }
        throw std::runtime_error("Expected ',' or '}' at position " + std::to_string(pos));
    }
    return JsonValue(obj);
}

inline JsonValue JsonParser::parse_array(const std::string& s, size_t& pos) {
    expect(s, pos, '[');
    JsonValue::Array arr;
    skip_ws(s, pos);
    if (peek(s, pos) == ']') {
        ++pos;
        return JsonValue(arr);
    }

    while (true) {
        skip_ws(s, pos);
        arr.push_back(parse_value(s, pos));
        skip_ws(s, pos);
        if (peek(s, pos) == ',') {
            ++pos;
            continue;
        }
        if (peek(s, pos) == ']') {
            ++pos;
            break;
        }
        throw std::runtime_error("Expected ',' or ']' at position " + std::to_string(pos));
    }
    return JsonValue(arr);
}

/** @brief 解析 JSON 字符串，返回去引号的内容 */
inline std::string JsonParser::parse_string_raw(const std::string& s, size_t& pos) {
    expect(s, pos, '"');
    std::string result;
    while (pos < s.size()) {
        char c = s[pos];
        if (c == '"') {
            ++pos;
            return result;
        }
        if (c == '\\') {
            ++pos;
            if (pos >= s.size()) throw std::runtime_error("Unexpected end in string escape");
            char esc = s[pos];
            switch (esc) {
                case '"':  result += '"'; break;
                case '\\': result += '\\'; break;
                case '/':  result += '/'; break;
                case 'b':  result += '\b'; break;
                case 'f':  result += '\f'; break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case 'u': {
                    // 处理 \\uXXXX Unicode 转义，转为 UTF-8 编码
                    if (pos + 4 >= s.size()) throw std::runtime_error("Invalid \\u escape");
                    std::string hex = s.substr(pos + 1, 4);
                    unsigned int code = static_cast<unsigned int>(std::strtoul(hex.c_str(), nullptr, 16));
                    if (code < 0x80) {
                        result += static_cast<char>(code);
                    } else if (code < 0x800) {
                        result += static_cast<char>(0xC0 | (code >> 6));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    } else {
                        result += static_cast<char>(0xE0 | (code >> 12));
                        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                        result += static_cast<char>(0x80 | (code & 0x3F));
                    }
                    pos += 4;
                } break;
                default: throw std::runtime_error("Invalid escape char: \\" + std::string(1, esc));
            }
            ++pos;
        } else {
            result += c;
            ++pos;
        }
    }
    throw std::runtime_error("Unterminated string");
}

inline JsonValue JsonParser::parse_string(const std::string& s, size_t& pos) {
    return JsonValue(parse_string_raw(s, pos));
}

/** @brief 解析 JSON 数值，自动区分整数和浮点数 */
inline JsonValue JsonParser::parse_number(const std::string& s, size_t& pos) {
    size_t start = pos;
    bool is_double = false;
    if (peek(s, pos) == '-') ++pos;

    while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;

    if (peek(s, pos) == '.') {
        is_double = true;
        ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
    }

    if (peek(s, pos) == 'e' || peek(s, pos) == 'E') {
        is_double = true;
        ++pos;
        if (peek(s, pos) == '+' || peek(s, pos) == '-') ++pos;
        while (pos < s.size() && std::isdigit(static_cast<unsigned char>(s[pos]))) ++pos;
    }

    std::string num_str = s.substr(start, pos - start);
    if (is_double) {
        return JsonValue(std::strtod(num_str.c_str(), nullptr));
    } else {
        return JsonValue(static_cast<int64_t>(std::strtoll(num_str.c_str(), nullptr, 10)));
    }
}

inline JsonValue JsonParser::parse_bool(const std::string& s, size_t& pos) {
    if (s.substr(pos, 4) == "true") {
        pos += 4;
        return JsonValue(true);
    }
    if (s.substr(pos, 5) == "false") {
        pos += 5;
        return JsonValue(false);
    }
    throw std::runtime_error("Invalid boolean at position " + std::to_string(pos));
}

inline JsonValue JsonParser::parse_null(const std::string& s, size_t& pos) {
    if (s.substr(pos, 4) == "null") {
        pos += 4;
        return JsonValue();
    }
    throw std::runtime_error("Invalid null at position " + std::to_string(pos));
}

/** @brief 便捷 JSON 解析函数 */
inline JsonValue parse_json(const std::string& json_str) {
    return JsonParser::parse(json_str);
}

}  // namespace course_planner::utils