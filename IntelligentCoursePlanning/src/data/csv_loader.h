/**
 * @file    csv_loader.h
 * @brief   CSV 数据加载器
 * @details 负责从 course_info.csv 和 course_time.csv 加载课程数据。
 *          包含 CourseDataset 聚合结构、CSV 行解析器、文件读取工具等。
 *          会自动处理 UTF-8 BOM 头、CSV 引号转义、字段缺失等边界情况。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <map>

#include "models/course.h"

namespace course_planner {

/**
 * @brief 课程数据集（课程规划系统的数据核心）
 *
 * 聚合了两张 CSV 表的数据：
 * - course_map: 以 course_basic_ID 为键的课程基础映射
 * - all_offerings: 所有教学班的平铺列表
 *
 * 通过先从 course_info.csv 加载课程和教学班基本信息，
 * 再从 course_time.csv 补充每个教学班的上课时间槽来完成数据加载。
 */
struct CourseDataset {
    std::map<std::string, CourseBasic> course_map;      ///< 课程基础映射 key=course_basic_ID
    std::vector<CourseOffering> all_offerings;           ///< 所有教学班（平铺）

    int total_offerings = 0;    ///< 教学班总数
    int total_courses = 0;      ///< 不同的 course_basic_ID 数量
    int total_time_slots = 0;   ///< 加载的时间槽记录数

    /**
     * @brief 根据 course_basic_ID 和 course_sp_ID 查找指定教学班
     * @param basic_id 课程基础序号
     * @param sp_id 教学班序号
     * @return 找到则返回指针，否则返回 nullptr
     */
    const CourseOffering* find_offering(const std::string& basic_id,
                                        const std::string& sp_id) const;

    /**
     * @brief 获取指定课程的所有教学班列表
     * @param basic_id 课程基础序号
     * @return 找到则返回指针，否则返回 nullptr
     */
    const std::vector<CourseOffering>* get_offerings(const std::string& basic_id) const;
};

/**
 * @brief 从 course_info.csv 内容加载课程信息和教学班
 * @param csv_content 文件内容字符串（需已去除 UTF-8 BOM）
 * @param dataset 输出数据集（会被清空并重新填充）
 * @return 成功加载的教学班记录数
 */
int load_course_info_csv(const std::string& csv_content, CourseDataset& dataset);

/**
 * @brief 从 course_time.csv 内容为教学班补充上课时间信息
 * @param csv_content 文件内容字符串（需已去除 UTF-8 BOM）
 * @param dataset 已加载课程信息的数据集（会被修改，为教学班填充 time_slots）
 * @return 成功加载的时间槽记录数
 */
int load_course_time_csv(const std::string& csv_content, CourseDataset& dataset);

/**
 * @brief 读取整个文件内容为字符串，并自动去除 UTF-8 BOM
 * @param filepath 文件路径
 * @return 文件内容字符串
 * @throws std::runtime_error 如果文件无法打开
 */
std::string read_file_content(const std::string& filepath);

/**
 * @brief 解析一行 CSV 文本为字段数组
 * @param line 一行 CSV 文本
 * @return 字段数组
 * @details 正确处理双引号包裹的字段和引号内的转义双引号 ""。
 *          示例：hello,"world, foo",bar -> ["hello", "world, foo", "bar"]
 */
std::vector<std::string> parse_csv_line(const std::string& line);

}  // namespace course_planner