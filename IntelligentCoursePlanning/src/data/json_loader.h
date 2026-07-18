/**
 * @file    json_loader.h
 * @brief   JSON 约束配置加载器
 * @details 负责从 JSON 文件（sample_constraints.json 及 major_profiles/*.json）
 *          解析规划约束条件，构造 Constraint 对象。
 *          使用内嵌的 jutil::JsonParser 进行解析，无需外部依赖。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>

#include "models/constraint.h"

namespace course_planner {

/**
 * @brief 从 JSON 文件路径加载约束条件
 * @param filepath JSON 文件路径
 * @return 解析后的 Constraint 对象
 * @details 先读取文件内容，再调用 parse_constraints_json 解析。
 */
Constraint load_constraints_from_json(const std::string& filepath);

/**
 * @brief 从 JSON 字符串解析约束条件
 * @param json_content JSON 格式的约束配置字符串
 * @return 解析后的 Constraint 对象
 * @details 支持从 sample_constraints.json 和 major_profiles/*.json 的格式解析：
 *          - target_course_scope: 必修和选修候选课程ID集合
 *          - max/min_credit_per_semester: 每学期学分上下限
 *          - min_total_credit: 总学分下限
 *          - avoid_time_blocks / preferred_time_blocks: 时间偏好
 */
Constraint parse_constraints_json(const std::string& json_content);

}  // namespace course_planner