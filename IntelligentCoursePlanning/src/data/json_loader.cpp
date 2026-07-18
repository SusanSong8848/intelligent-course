#include "json_loader.h"

#include <iostream>
#include <stdexcept>

#include "csv_loader.h"
#include "utils/json_parser.h"

namespace course_planner {

using utils::JsonValue;

Constraint parse_constraints_json(const std::string& json_content) {
    Constraint constraint;
    JsonValue j = utils::parse_json(json_content);

    // --- 基本信息 ---
    if (j.contains("profile_name")) {
        constraint.profile_name = j["profile_name"].get_string();
    }
    if (j.contains("target_department")) {
        constraint.target_department = j["target_department"].get_string();
    }

    // --- 目标课程集合 ---
    if (j.contains("target_course_scope")) {
        const auto& scope = j["target_course_scope"];

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
        // 遍历object的所有key
        for (int t = 1; t <= 8; ++t) {
            std::string key = std::to_string(t);
            if (obj.contains(key)) {
                constraint.max_credit_per_semester[t] = obj[key].get_double();
            }
        }
    }

    // --- 每学期学分下限（可选） ---
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
        && j["required_rule"].contains("required_categories_for_display")) {
        const auto& cats = j["required_rule"]["required_categories_for_display"];
        for (size_t i = 0; i < cats.size(); ++i) {
            constraint.required_categories_for_display.push_back(cats[i].get_string());
        }
    }

    // --- 时间段避让 ---
    if (j.contains("avoid_time_blocks")) {
        const auto& blocks = j["avoid_time_blocks"];
        for (size_t i = 0; i < blocks.size(); ++i) {
            const auto& block = blocks[i];
            TimeBlock tb;
            tb.day    = block.value("day", "");
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

Constraint load_constraints_from_json(const std::string& filepath) {
    std::string content = read_file_content(filepath);
    return parse_constraints_json(content);
}

}  // namespace course_planner