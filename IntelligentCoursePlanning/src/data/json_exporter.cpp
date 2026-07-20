/**
 * @file    json_exporter.cpp
 * @brief   JSON 导出器实现
 * @details 将规划结果和课程数据导出为前端可读的 JSON 文件。
 *          使用内嵌的字符串拼接生成 JSON（简化但足够用）。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include "json_exporter.h"
#include "csv_loader.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace course_planner {

/** @brief 将字符串中的双引号和反斜杠转义 */
static std::string json_escape(const std::string& s) {
    std::string result;
    for (char ch : s) {
        if (ch == '"') result += "\\\"";
        else if (ch == '\\') result += "\\\\";
        else if (ch == '\n') result += "\\n";
        else if (ch == '\r') result += "\\r";
        else if (ch == '\t') result += "\\t";
        else result += ch;
    }
    return result;
}
/*json_escape 的作用是确保字符串在 JSON 中是合法、安全的。
因为 JSON 标准规定：

字符串必须用双引号括起来。
如果字符串内部有双引号，必须写成 \"
如果内部有反斜杠，必须写成 \\
换行符必须写成 \n，回车写成 \r，制表符写成 \t
（实际上文件里是有这些反斜杠的，只是用文本编辑器打开时，转义序列会显示成原字符。）
*/

bool export_result_json(const ScheduleResult& result,
                        const Constraint& constraint,
                        const CourseDataset& /*dataset*/,
                        const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "{\n";
    out << "  \"success\": " << (result.success ? "true" : "false") << ",\n";
    out << "  \"summary\": {\n";
    out << "    \"total_credit\": " << result.total_credit << ",\n";
    out << "    \"required_credit\": " << constraint.required_credit << ",\n";
    out << "    \"elective_credit\": " << result.elective_credit_earned << ",\n";
    out << "    \"min_total_credit\": " << constraint.min_total_credit << ",\n";
    out << "    \"min_elective_credit\": " << constraint.derived_min_elective_credit_for_total << "\n";
    out << "  },\n";

    // 学期课程
    out << "  \"semesters\": [\n";
    for (int t = 1; t <= 8; ++t) {
        out << "    {";
        out << "\"term\": " << t << ", ";
        out << "\"credit\": " << (result.semester_credits.count(t) ? result.semester_credits.at(t) : 0.0) << ", ";
        out << "\"max_credit\": " << constraint.max_credit_for_term(t) << ", ";
        out << "\"blocked\": " << (constraint.is_term_blocked(t) ? "true" : "false") << ", ";
        out << "\"courses\": [\n";

        auto it = result.semester_courses.find(t);
        if (it != result.semester_courses.end()) {
            const auto& courses = it->second;
            for (size_t i = 0; i < courses.size(); ++i) {
                const auto* off = courses[i];
                out << "        {";
                out << "\"id\": \"" << json_escape(off->course_basic_ID) << "\", ";
                out << "\"sp_id\": \"" << json_escape(off->course_sp_ID) << "\", ";
                out << "\"name\": \"" << json_escape(off->course_name) << "\", ";
                out << "\"category\": \"" << json_escape(off->category) << "\", ";
                out << "\"credit\": " << off->credit << ", ";
                out << "\"teacher\": \"" << json_escape(off->teacher) << "\", ";
                out << "\"classroom\": \"" << json_escape(off->classroom) << "\", ";
                out << "\"timeslots\": [";
                for (size_t k = 0; k < off->time_slots.size(); ++k) {
                    if (k > 0) out << ", ";
                    out << "{\"day\": \"" << off->time_slots[k].day
                        << "\", \"beg\": " << off->time_slots[k].beg
                        << ", \"last\": " << off->time_slots[k].last << "}";
                }
                out << "]}";
                if (i < courses.size() - 1) out << ",";
                out << "\n";
            }
        }
        out << "      ]}";
        if (t < 8) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // 未安排课程
    out << "  \"unassigned\": [\n";
    for (size_t i = 0; i < result.unassigned_courses.size(); ++i) {
        const auto& uc = result.unassigned_courses[i];
        out << "    {\"id\": \"" << json_escape(uc.course_basic_ID)     //将字符串中的双引号和反斜杠转义
            << "\", \"name\": \"" << json_escape(uc.course_name)
            << "\", \"reason\": \"" << json_escape(uc.reason) << "\"}";
        if (i < result.unassigned_courses.size() - 1) out << ",";
        out << "\n";
    }
    out << "  ],\n";

    // 约束偏好
    out << "  \"preferences\": {\n";
    out << "    \"avoid\": [";
    for (size_t i = 0; i < constraint.avoid_time_blocks.size(); ++i) {
        const auto& tb = constraint.avoid_time_blocks[i];
        out << "{\"day\": \"" << tb.day << "\", \"beg\": " << tb.beg
            << ", \"last\": " << tb.last << ", \"hard\": " << (tb.hard ? "true" : "false")
            << ", \"reason\": \"" << json_escape(tb.reason) << "\"}";
        if (i < constraint.avoid_time_blocks.size() - 1) out << ",";
    }
    out << "],\n";
    out << "    \"preferred\": [";
    for (size_t i = 0; i < constraint.preferred_time_blocks.size(); ++i) {
        const auto& tb = constraint.preferred_time_blocks[i];
        out << "{\"day\": \"" << tb.day << "\", \"beg\": " << tb.beg
            << ", \"last\": " << tb.last << "}";
        if (i < constraint.preferred_time_blocks.size() - 1) out << ",";
    }
    out << "]\n  },\n";

    // 个性化偏好报告（确保每学期都输出完整字段）
    out << "  \"preference_reports\": [\n";
    for (int t = 1; t <= 8; ++t) {
        auto it = result.semester_preferences.find(t);
        if (it != result.semester_preferences.end()) {
            const auto& r = it->second;
            out << "    {\"term\": " << t
                << ", \"satisfied_preferred\": " << r.satisfied_preferred
                << ", \"total_preferred\": " << r.total_preferred
                << ", \"satisfied_avoid\": " << r.satisfied_avoid
                << ", \"total_avoid\": " << r.total_avoid
                << ", \"satisfaction_rate\": " << r.satisfaction_rate
                << ", \"detail\": \"" << json_escape(r.detail) << "\"}";
        } else {
            out << "    {\"term\": " << t
                << ", \"satisfied_preferred\": 0, \"total_preferred\": 0"
                << ", \"satisfied_avoid\": 0, \"total_avoid\": 0"
                << ", \"satisfaction_rate\": 0, \"detail\": \"\"}";
        }
        if (t < 8) out << ",";
        out << "\n";
    }
    out << "  ]\n";

    out << "}\n";
    return true;
}

bool export_dataset_json(const CourseDataset& dataset,
                         const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "[\n";
    int count = 0;
    for (const auto& [id, basic] : dataset.course_map) {
        if (count > 0) out << ",\n";
        out << "  {";
        out << "\"id\": \"" << json_escape(basic.course_basic_ID) << "\", ";
        out << "\"name\": \"" << json_escape(basic.course_name) << "\", ";
        out << "\"department\": \"" << json_escape(basic.department) << "\", ";
        out << "\"semester\": \"" << json_escape(basic.semester) << "\", ";
        out << "\"recommended_term\": " << basic.recommended_term << ", ";
        out << "\"category\": \"" << json_escape(basic.category) << "\", ";
        out << "\"credit\": " << basic.credit << ", ";
        out << "\"prereq_ids\": [";
        for (size_t j = 0; j < basic.prereq_IDs.size(); ++j) {
            if (j > 0) out << ", ";
            out << "\"" << json_escape(basic.prereq_IDs[j]) << "\"";
        }
        out << "], ";
        out << "\"offerings\": [";
        for (size_t j = 0; j < basic.offerings.size(); ++j) {
            const auto& off = basic.offerings[j];
            if (j > 0) out << ", ";
            out << "{\"sp_id\": \"" << json_escape(off.course_sp_ID)
                << "\", \"teacher\": \"" << json_escape(off.teacher)
                << "\", \"classroom\": \"" << json_escape(off.classroom)
                << "\", \"limits\": " << off.limits
                << ", \"timeslots\": [";
            for (size_t k = 0; k < off.time_slots.size(); ++k) {
                if (k > 0) out << ", ";
                out << "{\"day\": \"" << off.time_slots[k].day
                    << "\", \"beg\": " << off.time_slots[k].beg
                    << ", \"last\": " << off.time_slots[k].last << "}";
            }
            out << "]}";
        }
        out << "]}";
        count++;
    }
    out << "\n]\n";
    return true;
}

}  // namespace course_planner