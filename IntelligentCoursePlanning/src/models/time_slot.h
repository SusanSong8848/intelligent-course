/**
 * @file    time_slot.h
 * @brief   上课时间段数据结构
 * @details 定义 TimeSlot 结构体，表示一门课单次上课的时间信息（星期、开始节数、持续节数）。
 *          提供时间段重叠检测、范围包含判断等基础操作。
 *          TimeSlotList 用于表示一门课可能有多个上课时段（如理论课+实验课）。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace course_planner {

/**
 * @brief 上课时间段
 *
 * 表示单次上课的星期、开始节数和持续节数。
 * 例如：Mon 3-4 表示周一第3-4节课。
 * 注意：此处不包含周次信息，周次在 CourseOffering 中通过 beg_week/last_week 表示。
 */
struct TimeSlot {
    std::string day;    ///< 上课日期：Mon, Tue, Wed, Thu, Fri, Sat, Sun
    int beg;            ///< 开始节数，1-13
    int last;           ///< 持续节数，1-13

    /** @brief 计算结束节数 = beg + last - 1 */
    int end() const { return beg + last - 1; }

    /**
     * @brief 判断两个时间段是否在节次上重叠
     * @param other 另一个时间段
     * @return 如果day相同且节次区间有交集则返回true
     * @note 此方法只检查 day+节次，不检查周次重叠。
     *       周次重叠需要在冲突检测层额外判断。
     */
    bool overlaps(const TimeSlot& other) const {
        if (day != other.day) return false;
        return !(end() < other.beg || other.end() < beg);
    }

    /**
     * @brief 判断本时间段是否完全包含于给定的星期和节次范围
     * @param d 星期
     * @param b 范围开始节数
     * @param l 范围持续节数
     * @return 如果完全在范围内返回true
     */
    bool isWithin(const std::string& d, int b, int l) const {
        if (day != d) return false;
        int e = b + l - 1;
        return beg >= b && end() <= e;
    }
};

/// 时间段列表，一门课可能有多个上课时段（如理论+实验）
using TimeSlotList = std::vector<TimeSlot>;     //给一个复杂的类型起个外号（别名）

}  // namespace course_planner