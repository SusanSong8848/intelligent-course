/**
 * @file    constraint.h
 * @brief   课程规划约束条件数据结构
 * @details 定义课程规划中所有约束条件的数据结构：
 *          - TimeBlock: 时间区间约束（避让或偏好时段）
 *          - Constraint: 完整约束配置（从 JSON 加载），包含目标课程集合、
 *            学分上下限、时间段偏好/避让、交换/实习学期等
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>

#include "time_slot.h"

namespace course_planner {

/**
 * @brief 时间区间约束（用于避让或偏好时段）
 *
 * 例如：{day: "Fri", beg: 6, last: 4, hard: false, reason: "尽量避开周五下午"}
 * 表示一个 Fri 6-9 的时间区间，soft 约束，原因是"尽量避开周五下午"。
 */
struct TimeBlock {
    std::string day;     ///< 星期：Mon~Sun
    int beg;             ///< 开始节数
    int last;            ///< 持续节数
    bool hard;           ///< 是否为硬约束（true=必须满足, false=尽量满足）
    std::string reason;  ///< 约束原因描述

    /** @brief 判断一个 TimeSlot 是否落入此时间区间内 */
    bool contains(const TimeSlot& slot) const {
        return slot.isWithin(day, beg, last);
    }
};

/**
 * @brief 约束条件集合（从 JSON 配置文件加载）
 *
 * 包含课程规划所需的全部约束信息：
 * - 目标课程集合（必修 + 选修候选）
 * - 每学期学分上下限
 * - 总学分下限
 * - 时间段偏好与避让
 * - 特殊学期（交换/实习不排课）
 */
struct Constraint {
    std::string profile_name;                           ///< 配置名称
    std::string target_department;                      ///< 目标专业院系

    // ---- 目标课程集合 ----
    std::set<std::string> required_course_basic_IDs;    ///< 必修课程基础ID集合
    std::set<std::string> elective_candidate_course_basic_IDs; ///< 选修候选课程基础ID集合

    // ---- 每学期学分限制 ----
    std::map<int, double> max_credit_per_semester;      ///< 学期号 -> 学分上限
    std::map<int, double> min_credit_per_semester;      ///< 学期号 -> 学分下限（可选）

    // ---- 总学分要求 ----
    double min_total_credit = 0.0;                       ///< 八学期总学分下限
    double required_credit = 0.0;                        ///< 必修课程总学分
    double elective_min_credit = 0.0;                    ///< 选修学分下限
    double derived_min_elective_credit_for_total = 0.0;  ///< 需要从选修池补足的总选修学分

    // ---- 必选规则 ----
    std::vector<std::string> required_categories_for_display; ///< 展示用的必修类别

    // ---- 时间段约束 ----
    std::vector<TimeBlock> avoid_time_blocks;     ///< 避让时段（软/硬约束）
    std::vector<TimeBlock> preferred_time_blocks;  ///< 偏好时段（软约束）

    // ---- 特殊学期限制 ----
    std::set<int> exchange_terms;      ///< 交换学期，不安排课程
    std::set<int> internship_terms;    ///< 实习学期，不安排课程

    /**
     * @brief 获取指定学期的学分上限
     * @param term 学期号 1-8
     * @return 学分上限值（未配置则返回默认值40.0）
     */
    double max_credit_for_term(int term) const {
        auto it = max_credit_per_semester.find(term);
        if (it != max_credit_per_semester.end()) return it->second;
        return 40.0;
    }

    /**
     * @brief 获取指定学期的学分下限
     * @param term 学期号 1-8
     * @return 学分下限值（未配置则返回默认值0.0）
     */
    double min_credit_for_term(int term) const {
        auto it = min_credit_per_semester.find(term);
        if (it != min_credit_per_semester.end()) return it->second;
        return 0.0;
    }

    /**
     * @brief 判断指定学期是否因交换/实习不可排课
     * @param term 学期号 1-8
     * @return 不可排课返回 true
     */
    bool is_term_blocked(int term) const {
        return exchange_terms.count(term) > 0
            || internship_terms.count(term) > 0;
    }

    /** @brief 判断一门课是否为约束中的必修课 */
    bool is_required_course(const std::string& course_basic_ID) const {
        return required_course_basic_IDs.count(course_basic_ID) > 0;
    }

    /** @brief 判断一门课是否在选修候选池中 */
    bool is_elective_candidate(const std::string& course_basic_ID) const {
        return elective_candidate_course_basic_IDs.count(course_basic_ID) > 0;
    }
};

}  // namespace course_planner