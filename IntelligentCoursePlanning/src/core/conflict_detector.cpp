/**
 * @file    conflict_detector.cpp
 * @brief   时间冲突检测器实现
 * @details 实现三维时间冲突检测：星期(day) + 节次(beg,end) + 周次(beg_week,last_week)。
 *          两门课只要在任意一个维度上不重叠，就不算冲突。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include "conflict_detector.h"

#include <optional>
#include <sstream>

namespace course_planner {

bool ConflictDetector::weeks_overlap(const int& beg1, const int& len1, const int& beg2, const int& len2) {
    int end1 = beg1 + len1 - 1;
    int end2 = beg2 + len2 - 1;
    return !(end1 < beg2 || end2 < beg1);
}

bool ConflictDetector::has_conflict(const CourseOffering& a, const CourseOffering& b) {
    // 周次不重叠 → 绝对不冲突
    if (!weeks_overlap(a.beg_week, a.last_week, b.beg_week, b.last_week)) {     //重叠返回true
        return false;
    }

    // 检查所有时间段的组合
    for (const auto& slot_a : a.time_slots) {
        for (const auto& slot_b : b.time_slots) {
            if (slot_a.overlaps(slot_b)) {
                return true;
            }
        }
    }
    return false;
}

std::vector<ConflictInfo> ConflictDetector::check_semester_conflicts(
    const std::vector<const CourseOffering*>& semester_courses) {

    std::vector<ConflictInfo> conflicts;
    int n = static_cast<int>(semester_courses.size());

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (has_conflict(*semester_courses[i], *semester_courses[j])) {     //再检查一遍本学期课表任意两门课（单个教学班）之间是否有冲突
                ConflictInfo ci;
                ci.course1_id   = semester_courses[i]->course_basic_ID;
                ci.course2_id   = semester_courses[j]->course_basic_ID;
                ci.course1_name = semester_courses[i]->course_name;
                ci.course2_name = semester_courses[j]->course_name;

                // 生成详细的冲突描述
                std::ostringstream oss;
                oss << "[" << ci.course1_name << "(" << semester_courses[i]->course_sp_ID << ")";
                bool first = true;
                for (const auto& ts : semester_courses[i]->time_slots) {        //所有time_slot都要写出来
                    if (!first) oss << ",";
                    oss << " " << ts.day << " " << ts.beg << "-" << ts.end();
                    first = false;
                }
                oss << "] vs [" << ci.course2_name << "(" << semester_courses[j]->course_sp_ID << ")";
                first = true;
                for (const auto& ts : semester_courses[j]->time_slots) {
                    if (!first) oss << ",";
                    oss << " " << ts.day << " " << ts.beg << "-" << ts.end();
                    first = false;
                }
                oss << "]";       //以上生成详细信息流
                ci.detail = oss.str();

                conflicts.push_back(ci);
            }
        }
    }

    return conflicts;
}

std::optional<ConflictInfo> ConflictDetector::check_conflict_with_list(
    const CourseOffering& new_course,
    const std::vector<const CourseOffering*>& existing) {

    for (const auto* existing_off : existing) {
        if (has_conflict(new_course, *existing_off)) {
            ConflictInfo ci;                            /*ConflictInfo 记录了哪两门课在什么时间段冲突。它不是仅仅为了内部使用，而是最终会展示给用户（或记录到报告中）：
                                                        这些信息会写进 result.summary，并在前端显示给用户，让用户明白“为什么我的课表排不了”、“哪些课时间撞了”。这是项目需求里“可解释性”的重要体现。记录冲突细节，可以帮助诊断问题。*/
            ci.course1_id   = new_course.course_basic_ID;
            ci.course2_id   = existing_off->course_basic_ID;
            ci.course1_name = new_course.course_name;
            ci.course2_name = existing_off->course_name;
            ci.detail = new_course.course_name + " 与 " + existing_off->course_name + " 时间冲突";
            return ci;
        }
    }

    return std::nullopt;    //表示“空”，这比用指针（nullptr 表示没有）更安全，也比用 bool 加输出参数更优雅。它清晰地表达了“可能无结果”的语义。
}

}  // namespace course_planner