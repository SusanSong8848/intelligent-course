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

bool ConflictDetector::weeks_overlap(int beg1, int len1, int beg2, int len2) {
    int end1 = beg1 + len1 - 1;
    int end2 = beg2 + len2 - 1;
    return !(end1 < beg2 || end2 < beg1);
}

bool ConflictDetector::has_conflict(const CourseOffering& a, const CourseOffering& b) {
    // 周次不重叠 → 绝对不冲突
    if (!weeks_overlap(a.beg_week, a.last_week, b.beg_week, b.last_week)) {
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
            if (has_conflict(*semester_courses[i], *semester_courses[j])) {
                ConflictInfo ci;
                ci.course1_id   = semester_courses[i]->course_basic_ID;
                ci.course2_id   = semester_courses[j]->course_basic_ID;
                ci.course1_name = semester_courses[i]->course_name;
                ci.course2_name = semester_courses[j]->course_name;

                // 生成详细的冲突描述
                std::ostringstream oss;
                oss << "[" << ci.course1_name << "(" << semester_courses[i]->course_sp_ID << ")";
                bool first = true;
                for (const auto& ts : semester_courses[i]->time_slots) {
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
                oss << "]";
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
            ConflictInfo ci;
            ci.course1_id   = new_course.course_basic_ID;
            ci.course2_id   = existing_off->course_basic_ID;
            ci.course1_name = new_course.course_name;
            ci.course2_name = existing_off->course_name;
            ci.detail = new_course.course_name + " 与 " + existing_off->course_name + " 时间冲突";
            return ci;
        }
    }

    return std::nullopt;
}

}  // namespace course_planner