/**
 * @file    prerequisite_graph.cpp
 * @brief   先修关系图实现
 * @details 从 CourseDataset 构建 DAG，使用 Kahn 算法进行拓扑排序。
 *          同时支持环检测、缺失先修课检测和先修满足性判断。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include "prerequisite_graph.h"

#include <algorithm>
#include <stdexcept>

namespace course_planner {

void PrerequisiteGraph::build(const CourseDataset& dataset) {
    adj_list_.clear();
    reverse_adj_list_.clear();
    all_nodes_.clear();
    edge_count_ = 0;

    for (const auto& [id, basic] : dataset.course_map) {
        all_nodes_.insert(id);
        // 确保每个节点都有邻接表条目（即使出度为0）
        if (adj_list_.find(id) == adj_list_.end()) {
            adj_list_[id] = {};
        }

        for (const auto& pre_id : basic.prereq_IDs) {
            // 只在先修课存在于数据集中时才添加边
            if (dataset.course_map.count(pre_id) > 0) {
                adj_list_[pre_id].push_back(id);
                reverse_adj_list_[id].push_back(pre_id);
                edge_count_++;
            }
        }
    }
}

const std::vector<std::string>& PrerequisiteGraph::get_prerequisites(
    const std::string& course_id) const {
    static const std::vector<std::string> empty;
    auto it = reverse_adj_list_.find(course_id);
    if (it != reverse_adj_list_.end()) return it->second;
    return empty;
}

std::vector<std::string> PrerequisiteGraph::kahn_sort(bool throw_on_cycle) const {
    // 计算每个节点的入度
    std::map<std::string, int> in_degree;
    for (const auto& node : all_nodes_) {
        in_degree[node] = 0;
    }
    for (const auto& [pre, successors] : adj_list_) {
        for (const auto& succ : successors) {
            in_degree[succ]++;
        }
    }

    // 入度为0的节点入队
    std::queue<std::string> q;
    for (const auto& [node, deg] : in_degree) {
        if (deg == 0) q.push(node);
    }

    std::vector<std::string> result;
    while (!q.empty()) {
        std::string node = q.front();
        q.pop();
        result.push_back(node);

        auto it = adj_list_.find(node);
        if (it != adj_list_.end()) {
            for (const auto& succ : it->second) {
                in_degree[succ]--;
                if (in_degree[succ] == 0) {
                    q.push(succ);
                }
            }
        }
    }

    // 检查是否所有节点都被处理了
    if (result.size() != all_nodes_.size()) {
        // 有环：收集剩余节点
        cycle_nodes_.clear();
        for (const auto& [node, deg] : in_degree) {
            if (deg > 0) cycle_nodes_.insert(node);
        }

        if (throw_on_cycle) {
            throw std::runtime_error(
                "先修关系图中存在环路！涉及 " + std::to_string(cycle_nodes_.size()) + " 门课程");
        }
    }

    return result;
}

std::vector<std::string> PrerequisiteGraph::topological_sort() const {
    return kahn_sort(true);
}

bool PrerequisiteGraph::has_cycle() const {
    try {
        kahn_sort(false);
        return cycle_nodes_.size() > 0;
    } catch (...) {
        return true;
    }
}

std::set<std::string> PrerequisiteGraph::check_missing_prerequisites(
    const std::set<std::string>& target_ids) const {
    std::set<std::string> missing;

    for (const auto& course_id : target_ids) {
        auto it = reverse_adj_list_.find(course_id);
        if (it == reverse_adj_list_.end()) continue;

        for (const auto& pre_id : it->second) {
            // 先修课不在图中（即不在数据集中）
            if (all_nodes_.count(pre_id) == 0) {
                missing.insert(pre_id);
            }
        }
    }

    return missing;
}

bool PrerequisiteGraph::are_prerequisites_satisfied(
    const std::string& course_id,
    const std::set<std::string>& completed) const {
    auto it = reverse_adj_list_.find(course_id);
    if (it == reverse_adj_list_.end()) return true;  // 无先修要求

    for (const auto& pre_id : it->second) {
        if (completed.count(pre_id) == 0) {
            return false;  // 存在未完成的先修课
        }
    }
    return true;
}

void PrerequisiteGraph::print_stats() const {
    std::cout << "[先修图] 节点数: " << node_count()
              << ", 边数: " << edge_count_ << std::endl;

    if (has_cycle()) {
        std::cout << "[先修图] ⚠ 检测到环路！涉及 " << cycle_nodes_.size() << " 门课程" << std::endl;
        for (const auto& id : cycle_nodes_) {
            std::cout << "  - " << id << std::endl;
        }
    } else {
        std::cout << "[先修图] ✅ 无环路" << std::endl;
    }
}

}  // namespace course_planner