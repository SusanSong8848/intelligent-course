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

/// 轻量级 JSON 解析器（内嵌实现，零外部依赖）
/// 支持 object, array, string, number (int/double), bool, null
class JsonValue {
public:
    enum Type { NUL, BOOL, INT, DOUBLE, STRING, ARRAY, OBJECT };

    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

private:
    Type type_ = NUL;
    std::variant<bool, int64_t, double, std::string, Array, Object> value_;

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

    bool contains(const std::string& key) const {
        if (type_ != OBJECT) return false;
        return std::get<Object>(value_).count(key) > 0;
    }

    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null_val;
        const auto& obj = std::get<Object>(value_);
        auto it = obj.find(key);
        if (it != obj.end()) return it->second;
        return null_val;
    }

    const JsonValue& operator[](size_t index) const {
        return std::get<Array>(value_).at(index);
    }

    size_t size() const {
        if (type_ == ARRAY) return std::get<Array>(value_).size();
        if (type_ == OBJECT) return std::get<Object>(value_).size();
        return 0;
    }

    // value 方法：如果存在则返回，否则返回默认值
    std::string value(const std::string& key, const char* default_val) const {
        if (contains(key)) return (*this)[key].get_string();
        return default_val;
    }
    int value(const std::string& key, int default_val) const {
        if (contains(key)) return (*this)[key].get_int();
        return default_val;
    }
    double value(const std::string& key, double default_val) const {
        if (contains(key)) return (*this)[key].get_double();
        return default_val;
    }
    bool value(const std::string& key, bool default_val) const {
        if (contains(key)) return (*this)[key].get_bool();
        return default_val;
    }
};

/// JSON 解析器实现
class JsonParser {
public:
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

    static JsonValue parse_value(const std::string& s, size_t& pos) {
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

    static JsonValue parse_object(const std::string& s, size_t& pos) {
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

    static JsonValue parse_array(const std::string& s, size_t& pos) {
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

    static std::string parse_string_raw(const std::string& s, size_t& pos) {
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
                        // 简单处理 \\uXXXX（取码点低字节）
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

    static JsonValue parse_string(const std::string& s, size_t& pos) {
        return JsonValue(parse_string_raw(s, pos));
    }

    static JsonValue parse_number(const std::string& s, size_t& pos) {
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

    static JsonValue parse_bool(const std::string& s, size_t& pos) {
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

    static JsonValue parse_null(const std::string& s, size_t& pos) {
        if (s.substr(pos, 4) == "null") {
            pos += 4;
            return JsonValue();
        }
        throw std::runtime_error("Invalid null at position " + std::to_string(pos));
    }
};

/// 便捷解析函数
inline JsonValue parse_json(const std::string& json_str) {
    return JsonParser::parse(json_str);
}

}  // namespace course_planner::utils