#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>

#include "time_slot.h"

namespace course_planner {

/// 时间区间约束（避让或偏好）
struct TimeBlock {
    std::string day;   // Mon~Sun
    int beg;           // 开始节数
    int last;          // 持续节数
    bool hard;         // 是否为硬约束
    std::string reason; // 约束原因

    /// 判断一个TimeSlot是否落入此区间
    bool contains(const TimeSlot& slot) const {
        return slot.isWithin(day, beg, last);
    }
};

/// 约束条件（从JSON加载）
struct Constraint {
    std::string profile_name;                  // 配置名称
    std::string target_department;             // 目标专业院系

    // 目标课程集合
    std::set<std::string> required_course_basic_IDs;       // 必修课程基础ID集合
    std::set<std::string> elective_candidate_course_basic_IDs; // 选修候选课程基础ID集合

    // 每学期学分限制
    std::map<int, double> max_credit_per_semester;  // 学期号 -> 学分上限
    std::map<int, double> min_credit_per_semester;  // 学期号 -> 学分下限（可选）

    // 总学分要求
    double min_total_credit = 0.0;
    double required_credit = 0.0;              // 必修课程总学分
    double elective_min_credit = 0.0;          // 选修学分下限
    double derived_min_elective_credit_for_total = 0.0; // 需要从选修池补足的学分

    // 必选规则
    std::vector<std::string> required_categories_for_display;

    // 时间段避让（软约束或硬约束）
    std::vector<TimeBlock> avoid_time_blocks;

    // 时间段偏好（软约束）
    std::vector<TimeBlock> preferred_time_blocks;

    // 特殊学期限制
    std::set<int> exchange_terms;    // 交换学期，不安排课程
    std::set<int> internship_terms;  // 实习学期，不安排课程

    /// 获取指定学期的学分上限（默认40）
    double max_credit_for_term(int term) const {
        auto it = max_credit_per_semester.find(term);
        if (it != max_credit_per_semester.end()) return it->second;
        return 40.0; // 默认
    }

    /// 获取指定学期的学分下限（默认0）
    double min_credit_for_term(int term) const {
        auto it = min_credit_per_semester.find(term);
        if (it != min_credit_per_semester.end()) return it->second;
        return 0.0; // 默认
    }

    /// 判断指定学期是否因交换/实习不可排课
    bool is_term_blocked(int term) const {
        return exchange_terms.count(term) > 0
            || internship_terms.count(term) > 0;
    }

    /// 判断一门课是否为必修课
    bool is_required_course(const std::string& course_basic_ID) const {
        return required_course_basic_IDs.count(course_basic_ID) > 0;
    }

    /// 判断一门课是否在选修候选池中
    bool is_elective_candidate(const std::string& course_basic_ID) const {
        return elective_candidate_course_basic_IDs.count(course_basic_ID) > 0;
    }
};

}  // namespace course_planner