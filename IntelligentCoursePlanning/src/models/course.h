/**
 * @file    course.h
 * @brief   课程与教学班数据结构
 * @details 定义课程规划系统的两大核心数据结构：
 *          - CourseOffering: 教学班（course_info.csv 的一行），包含教师、教室、容量、时间槽等
 *          - CourseBasic: 课程基础（同一门课的多个教学班汇总），包含先修关系、推荐学期等
 *          还提供学期季节映射等工具函数。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once                    /*#pragma once 是什么意思？
                                它写在 .h 头文件的最开头，作用是防止头文件被重复包含。*/

#include <string>
#include <vector>
#include <cstdint>
#include <set>

#include "time_slot.h"

namespace course_planner {                      //只要在同一个命名域里面就不用加<其他文件>::了

/**
 * @brief 教学班（course_info.csv 中的一行记录 + TimeSlotList(从 course_time.csv 加载)）
 *
 * 同一门课可能有多个教学班，每个教学班有不同的教师、教室、时间和容量。
 * course_basic_ID + course_sp_ID 组成唯一键。
 */
struct CourseOffering {
    std::string course_basic_ID;   ///< 课程基础序号，同一门课不同教学班共享
    std::string course_sp_ID;      ///< 教学班序号，如 "01"
    std::string course_name;       ///< 课程名称
    std::string department;        ///< 开课院系
    std::string semester;          ///< 开课季节："秋季学期" 或 "春季学期"
    int recommended_term;          ///< 建议/最早修读学期，1-8
    std::string category;          ///< 课程类别："专业必修课"/"学科基础课"/"专业选修课"
    double credit;                 ///< 学分
    int beg_week;                  ///< 开始周数
    int last_week;                 ///< 持续周数
    std::string teacher;           ///< 授课教师
    std::string classroom;         ///< 上课教室
    int limits;                    ///< 选课人数上限
    std::string prereq_ID_raw;     ///< 前置课程原始字符串（分号分隔）

    TimeSlotList time_slots;       ///< 该教学班的上课时间段（从 course_time.csv 加载）

    /** @brief 获取教学班唯一标识 "course_basic_ID_course_sp_ID" */
    std::string unique_id() const {
        return course_basic_ID + "_" + course_sp_ID;
    }

    /** @brief 是否为秋季学期课程 */
    bool is_autumn() const { return semester == "秋季学期"; }

    /** @brief 是否为春季学期课程 */
    bool is_spring() const { return semester == "春季学期"; }

    /**
     * @brief 判断该教学班是否可以安排在指定学期
     * @param term 学期号 1-8
     * @return 秋季课程只能排在奇数学期(1/3/5/7)，春季课程只能排在偶数学期(2/4/6/8)
     */
    bool can_schedule_in_term(int term) const {
        if (term < 1 || term > 8) return false;
        if (is_autumn()) return term % 2 == 1;  // 1,3,5,7
        else return term % 2 == 0;               // 2,4,6,8
    }
};

/**
 * @brief 课程基础（同一门课的所有教学班汇总）
 *
 * 将 course_basic_ID 相同的多个教学班聚合在一起。
 * 包含先修课程关系（从 prereq_ID 解析而来，用分号分隔多门先修课）。
 */
struct CourseBasic {
    std::string course_basic_ID;           ///< 课程基础序号
    std::string course_name;               ///< 课程名称
    std::string department;                ///< 开课院系
    std::string semester;                  ///< 开课季节："秋季学期" 或 "春季学期"
    int recommended_term;                  ///< 建议/最早修读学期
    std::string category;                  ///< 课程类别
    double credit;                         ///< 学分
    std::vector<std::string> prereq_IDs;   ///< 先修课程ID列表（已按分号拆分）
    std::vector<CourseOffering> offerings; ///< 该课程的所有教学班

    /** @brief 是否为必修类课程（专业必修课 或 学科基础课） */
    bool is_required_category() const {
        return category == "专业必修课" || category == "学科基础课";
    }
};

/** @brief 学期季节枚举 */
enum class SemesterType {
    AUTUMN = 0,   ///< 秋季学期
    SPRING = 1    ///< 春季学期
};

/**
 * @brief 将学期号(1-8)映射为季节
 * @param term 学期号
 * @return 奇数学期=秋季，偶数学期=春季
 */
inline SemesterType term_to_semester(int term) {
    return (term % 2 == 1) ? SemesterType::AUTUMN : SemesterType::SPRING;
}

/**
 * @brief 判断学期号与开课季节字符串是否匹配
 * @param term 学期号 1-8
 * @param semester "秋季学期" 或 "春季学期"
 * @return 匹配返回 true
 */
inline bool term_matches_semester(int term, const std::string& semester) {
    if (semester == "秋季学期") return term % 2 == 1;
    if (semester == "春季学期") return term % 2 == 0;
    return false;
}

}  // namespace course_planner