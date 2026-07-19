/**
 * @file    json_loader.cpp
 * @brief   JSON 约束配置加载器实现
 * @details 解析 sample_constraints.json 及 major_profiles/*.json 中的规划约束。
 *          从 target_course_scope 提取必修/选修课程集合，从 avoid_time_blocks /
 *          preferred_time_blocks 提取时间偏好，并构造完整的 Constraint 对象。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

 /*JSON文件本质是什么？
 相当于就是一个大对象({}表示一个对象)里面有很多像键值对的字段，键值对的value又可以是字符串，数组或更多键值对，嵌套下去就形成了树状结构。*/
#include "json_loader.h"

#include <iostream>
#include <stdexcept>

#include "csv_loader.h"
#include "utils/json_parser.h"

namespace course_planner {

using utils::JsonValue;

/**
 * @brief 从 JSON 字符串解析约束条件
 *
 * 解析的字段包括：
 * - profile_name / target_department: 配置基本信息
 * - target_course_scope.required_course_basic_IDs: 必修课程ID集合
 * - target_course_scope.elective_candidate_course_basic_IDs: 选修候选ID集合
 * - max_credit_per_semester / min_credit_per_semester: 每学期学分限制
 * - min_total_credit / required_credit / elective_min_credit: 总学分要求
 * - avoid_time_blocks: 避让时段（含 hard/soft 标记）
 * - preferred_time_blocks: 偏好时段
 *
 * @param json_content JSON 字符串
 * @return 解析后的 Constraint 对象
 */
Constraint parse_constraints_json(const std::string& json_content) {
    Constraint constraint;
    JsonValue j = utils::parse_json(json_content);      //jion_parser 里 course_planner::utils::JsonValue{type_, value_}
                                                        /*当type_ = OBJECT, value_ = Object时：     //（using Object = std::map<std::string, JsonValue>键值对）
                                                        就构成了键值对嵌套树状结构（JsonValue就是随便什么类型(value_)再加上它的类型是什么(type_)的说明）*/

    // --- 基本信息 ---
    if (j.contains("profile_name")) {
        constraint.profile_name = j["profile_name"].get_string();   /*course_planner::utils::JsonValue::operator[](const std::string &key) const
                                                                        重载运算符：
                                                                        JsonValue 不是一个 std::map，它是一个自定义类，内部虽然可能持有一个 std::map<std::string, JsonValue>（当 type_ 是 OBJECT 时），但外部不能直接用 [] 来访问它，除非我们自己写了 operator[]。
                                                                        即要实现j["profile_name"]，需要找到"profile_name"所在的那个键值对pair才能找到它的 value
                                                                        pair{string, JsonValue} -> second，只有找到整个pair{key, JsonValue} 才能找到 key 匹配的JsonValue*/
    }
    if (j.contains("target_department")) {
        constraint.target_department = j["target_department"].get_string();
    }

    // --- 目标课程集合 ---
    if (j.contains("target_course_scope")) {
        const auto& scope = j["target_course_scope"];       //键值对外部{ target_course_scope : 键值对内部 }，把这个键值对内部再给 scope

        if (scope.contains("required_course_basic_IDs")) {
            for (size_t i = 0; i < scope["required_course_basic_IDs"].size(); ++i) {
                constraint.required_course_basic_IDs.insert(
                    scope["required_course_basic_IDs"][i].get_string());
            }
        }

        if (scope.contains("elective_candidate_course_basic_IDs")) {
            for (size_t i = 0; i < scope["elective_candidate_course_basic_IDs"].size(); ++i) {
                constraint.elective_candidate_course_basic_IDs.insert(
                    scope["elective_candidate_course_basic_IDs"][i].get_string());
            }
        }
    }

    // --- 每学期学分上限 ---
    if (j.contains("max_credit_per_semester")) {
        const auto& obj = j["max_credit_per_semester"];
        for (int t = 1; t <= 8; ++t) {
            std::string key = std::to_string(t);
            if (obj.contains(key)) {
                constraint.max_credit_per_semester[t] = obj[key].get_double();
            }
        }
    }

    // --- 每学期学分下限（可选字段） ---
    if (j.contains("min_credit_per_semester")) {
        const auto& obj = j["min_credit_per_semester"];
        for (int t = 1; t <= 8; ++t) {
            std::string key = std::to_string(t);
            if (obj.contains(key)) {
                constraint.min_credit_per_semester[t] = obj[key].get_double();
            }
        }
    }

    // --- 总学分要求 ---
    if (j.contains("min_total_credit")) {
        constraint.min_total_credit = j["min_total_credit"].get_double();
    }

    // --- 必修学分 ---
    if (j.contains("required_rule") && j["required_rule"].contains("required_credit")) {
        constraint.required_credit = j["required_rule"]["required_credit"].get_double();
    }

    // --- 选修学分下限 ---
    if (j.contains("elective_min_credit")) {
        constraint.elective_min_credit = j["elective_min_credit"].get_double();
    }

    // --- 需要从选修池补足的学分 ---
    if (j.contains("total_credit_rule")
        && j["total_credit_rule"].contains("derived_min_elective_credit_for_total")) {
        constraint.derived_min_elective_credit_for_total =
            j["total_credit_rule"]["derived_min_elective_credit_for_total"].get_double();
    }

    // --- 必选规则展示 ---
    if (j.contains("required_rule")
        && j["required_rule"].contains("required_categories_for_display")) {        //"专业必修课","学科基础课"
        const auto& cats = j["required_rule"]["required_categories_for_display"];       //这里 const course_planner::utils::JsonValue &cats 为{ARRAY,array}
        for (size_t i = 0; i < cats.size(); ++i) {
            constraint.required_categories_for_display.push_back(cats[i].get_string());
        }
    }

    // --- 时间段避让 ---
    if (j.contains("avoid_time_blocks")) {              //"尽量避开周五下午"，"尽量不安排周日课程"......
        const auto& blocks = j["avoid_time_blocks"];    //const course_planner::utils::JsonValue &blocks
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];      //在这里const course_planner::utils::JsonValue &block 应该是 Object
            TimeBlock tb;
            tb.day    = block.value("day", "");     //若Object里面给了"day"就返回它的value_，如果没有就使用默认值""
            tb.beg    = block.value("beg", 1);
            tb.last   = block.value("last", 1);
            tb.hard   = block.value("hard", false);
            tb.reason = block.value("reason", "");
            constraint.avoid_time_blocks.push_back(tb);
        }
    }

    // --- 时间段偏好 ---
    if (j.contains("preferred_time_blocks")) {
        const auto& blocks = j["preferred_time_blocks"];
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];
            TimeBlock tb;
            tb.day  = block.value("day", "");
            tb.beg  = block.value("beg", 1);
            tb.last = block.value("last", 1);
            tb.hard = false;
            constraint.preferred_time_blocks.push_back(tb);
        }
    }

    std::cout << "[加载] 约束配置 \"" << constraint.profile_name
              << "\": " << constraint.required_course_basic_IDs.size()
              << " 门必修, " << constraint.elective_candidate_course_basic_IDs.size()
              << " 门选修候选" << std::endl;

    return constraint;
}

/**
 * @brief 从 JSON 文件路径加载约束条件
 * @param filepath JSON 文件路径
 * @return 解析后的 Constraint 对象
 */
Constraint load_constraints_from_json(const std::string& filepath) {
    std::string content = read_file_content(filepath);
    return parse_constraints_json(content);
}

}  // namespace course_planner