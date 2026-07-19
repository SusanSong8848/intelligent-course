/**
 * @file    conflict_detector.h
 * @brief   时间冲突检测器
 * @details 在同一学期内检查课程之间的时间冲突。
 *          冲突条件：两门课 day 相同 && 节次区间重叠 && 周次区间重叠。
 *          一门课如果有多个 TimeSlot，所有时间段都要参与检查。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <optional>

#include "models/course.h"

namespace course_planner {

/**
 * @brief 冲突描述
 */
struct ConflictInfo {
    std::string course1_id;    ///< 课程1的 basic_ID
    std::string course2_id;    ///< 课程2的 basic_ID
    std::string course1_name;  ///< 课程1名称
    std::string course2_name;  ///< 课程2名称
    std::string detail;        ///< 冲突详情（如 "Mon 3-4 vs Mon 3-5"）
};

/**
 * @brief 时间冲突检测器
 *
 * 检测同一学期内选课之间是否存在时间冲突。
 * 冲突判断同时考虑：星期(day)、节次(beg,end)、周次(beg_week,last_week)。
 */
class ConflictDetector {
public:
    /**
     * @brief 检查两门课的所有时间段是否冲突（含周次）
     * @param a 教学班A
     * @param b 教学班B
     * @return 存在任何冲突返回true
     * @note 同时检查 day+节次重叠 和 周次重叠
     */
    static bool has_conflict(const CourseOffering& a, const CourseOffering& b);

    /**
     * @brief 检查一学期内所有课程是否两两冲突
     * @param semester_courses 某学期的选课列表
     * @return 所有冲突的列表（空列表表示无冲突）
     */
    static std::vector<ConflictInfo> check_semester_conflicts(
        const std::vector<const CourseOffering*>& semester_courses);

    /**
     * @brief 检查一门课是否与已选课程列表中的任何一门冲突
     * @param new_course 待检查的新课程
     * @param existing 已选课程列表
     * @return 如果冲突返回冲突对象，否则返回std::nullopt
     */
    static std::optional<ConflictInfo> check_conflict_with_list(
        const CourseOffering& new_course,
        const std::vector<const CourseOffering*>& existing);

    /**
     * @brief 检查两个周次区间是否重叠
     * @param beg1 课1开始周
     * @param len1 课1持续周数
     * @param beg2 课2开始周
     * @param len2 课2持续周数
     * @return 重叠返回true
     */
    static bool weeks_overlap(int beg1, int len1, int beg2, int len2);
};

};  // namespace course_planner