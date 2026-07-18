#pragma once

#include <string>
#include <vector>
#include <map>

#include "models/course.h"

namespace course_planner {

/// 课程数据集
struct CourseDataset {
    /// 所有课程基础信息，key = course_basic_ID
    std::map<std::string, CourseBasic> course_map;

    /// 所有教学班（平铺）
    std::vector<CourseOffering> all_offerings;

    /// 根据 course_basic_ID 和 course_sp_ID 查找教学班
    const CourseOffering* find_offering(const std::string& basic_id,
                                        const std::string& sp_id) const;

    /// 获取指定课程的所有教学班
    const std::vector<CourseOffering>* get_offerings(const std::string& basic_id) const;

    /// 数据集统计信息
    int total_offerings = 0;
    int total_courses = 0;      // 不同的course_basic_ID数量
    int total_time_slots = 0;
};

/// 从CSV文件加载课程信息
/// @param csv_content 文件内容字符串（已去除BOM）
/// @param dataset 输出数据集
/// @return 加载成功的记录数
int load_course_info_csv(const std::string& csv_content, CourseDataset& dataset);

/// 从CSV文件加载开课时间信息
/// @param csv_content 文件内容字符串（已去除BOM）
/// @param dataset 已加载课程信息的数据集（会被修改，填充time_slots）
/// @return 加载成功的时间记录数
int load_course_time_csv(const std::string& csv_content, CourseDataset& dataset);

/// 读取整个文件内容为字符串
std::string read_file_content(const std::string& filepath);

/// CSV行解析：处理带引号的字段
std::vector<std::string> parse_csv_line(const std::string& line);

}  // namespace course_planner