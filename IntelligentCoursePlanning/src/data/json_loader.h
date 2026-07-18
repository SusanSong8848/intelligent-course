#pragma once

#include <string>

#include "models/constraint.h"

namespace course_planner {

/// 从JSON文件加载约束条件
/// @param filepath JSON文件路径
/// @return 解析后的Constraint对象
Constraint load_constraints_from_json(const std::string& filepath);

/// 从JSON字符串解析约束条件
/// @param json_content JSON字符串
/// @return 解析后的Constraint对象
Constraint parse_constraints_json(const std::string& json_content);

}  // namespace course_planner