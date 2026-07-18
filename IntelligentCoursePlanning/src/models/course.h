#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <set>

#include "time_slot.h"

namespace course_planner {

/// 教学班（course_info.csv 中的一行）
struct CourseOffering {
    std::string course_basic_ID;   // 课程基础序号
    std::string course_sp_ID;      // 教学班序号，如 "01"
    std::string course_name;       // 课程名称
    std::string department;        // 开课院系
    std::string semester;          // "秋季学期" 或 "春季学期"
    int recommended_term;          // 建议/最早修读学期，1-8
    std::string category;          // "专业必修课"、"学科基础课"、"专业选修课"
    double credit;                 // 学分
    int beg_week;                  // 开始周数
    int last_week;                 // 持续周数
    std::string teacher;           // 授课教师
    std::string classroom;         // 上课教室
    int limits;                    // 选课人数上限
    std::string prereq_ID_raw;     // 前置课程原始字符串（分号分隔）

    TimeSlotList time_slots;       // 该教学班的上课时间段（从course_time.csv加载）

    /// 获取该教学班的唯一标识
    std::string unique_id() const {
        return course_basic_ID + "_" + course_sp_ID;
    }

    /// 判断是否为秋季学期课程
    bool is_autumn() const { return semester == "秋季学期"; }

    /// 判断是否为春季学期课程
    bool is_spring() const { return semester == "春季学期"; }

    /// 判断该教学班是否可以安排在指定学期
    /// 规则：秋季课程只能排1/3/5/7，春季课程只能排2/4/6/8
    bool can_schedule_in_term(int term) const {
        if (term < 1 || term > 8) return false;
        if (is_autumn()) return term % 2 == 1;  // 1,3,5,7
        else return term % 2 == 0;               // 2,4,6,8
    }
};

/// 课程基础（同一门课可能有多个教学班）
struct CourseBasic {
    std::string course_basic_ID;           // 课程基础序号
    std::string course_name;               // 课程名称
    std::string department;                // 开课院系
    std::string semester;                  // "秋季学期" 或 "春季学期"
    int recommended_term;                  // 建议/最早修读学期
    std::string category;                  // 课程类别
    double credit;                         // 学分
    std::vector<std::string> prereq_IDs;   // 先修课程ID列表（已按分号拆分）
    std::vector<CourseOffering> offerings; // 该课程的所有教学班

    /// 是否为必修类课程
    bool is_required_category() const {
        return category == "专业必修课" || category == "学科基础课";
    }
};

/// 学期枚举
enum class SemesterType {
    AUTUMN = 0,   // 秋季学期
    SPRING = 1    // 春季学期
};

/// 将学期号(1-8)映射为季节
inline SemesterType term_to_semester(int term) {
    return (term % 2 == 1) ? SemesterType::AUTUMN : SemesterType::SPRING;
}

/// 判断学期季节是否匹配
inline bool term_matches_semester(int term, const std::string& semester) {
    if (semester == "秋季学期") return term % 2 == 1;
    if (semester == "春季学期") return term % 2 == 0;
    return false;
}

}  // namespace course_planner