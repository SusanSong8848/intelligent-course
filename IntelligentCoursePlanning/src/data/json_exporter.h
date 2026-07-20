/**
 * @file    json_exporter.h
 * @brief   规划结果 JSON 导出器
 * @details 将 ScheduleResult 导出为 JSON 文件，供前端读取展示。
 *          使用内嵌的 JsonValue 构建 JSON 结构，无需外部依赖。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>

#include "core/scheduler.h"
#include "models/constraint.h"

namespace course_planner {

/**
 * @brief 将规划结果导出为 JSON 文件
 * @param result 规划结果
 * @param constraint 约束配置
 * @param dataset 课程数据集
 * @param filepath 输出文件路径
 * @return 成功返回 true
 */
bool export_result_json(const ScheduleResult& result,
                        const Constraint& constraint,
                        const CourseDataset& dataset,
                        const std::string& filepath);

/**
 * @brief 将课程数据集导出为 JSON（供前端筛选展示）
 * @param dataset 课程数据集
 * @param filepath 输出文件路径
 * @return 成功返回 true
 */
bool export_dataset_json(const CourseDataset& dataset,
                         const std::string& filepath);

}  // namespace course_planner