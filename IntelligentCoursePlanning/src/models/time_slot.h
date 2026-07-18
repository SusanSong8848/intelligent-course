#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace course_planner {

/// 上课时间段
struct TimeSlot {
    std::string day;    // Mon, Tue, Wed, Thu, Fri, Sat, Sun
    int beg;            // 开始节数，1-13
    int last;           // 持续节数，1-13

    /// 结束节数 = beg + last - 1
    int end() const { return beg + last - 1; }

    /// 判断两个时间段是否重叠（仅day+节次，不含周次）
    bool overlaps(const TimeSlot& other) const {
        if (day != other.day) return false;
        return !(end() < other.beg || other.end() < beg);
    }

    /// 判断是否包含于给定的星期和节次范围内
    bool isWithin(const std::string& d, int b, int l) const {
        if (day != d) return false;
        int e = b + l - 1;
        return beg >= b && end() <= e;
    }
};

/// 时间段集合，一门课可能有多个上课时段
using TimeSlotList = std::vector<TimeSlot>;

}  // namespace course_planner