/**
 * @file    prerequisite_graph.h
 * @brief   先修关系图与拓扑排序
 * @details 基于课程数据集构建有向无环图（DAG），节点为 course_basic_ID，
 *          边表示"先修 → 后继"关系。提供 Kahn 拓扑排序、环检测、
 *          缺失先修课程检测、先修链可达性查询等功能。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <iostream>

#include "data/csv_loader.h"

namespace course_planner {

/**
 * @brief 先修关系图
 *
 * 使用邻接表存储有向边：pre_id → course_id。
 * 以 Kahn 算法进行拓扑排序，同时检测环路和缺失先修课。
 */
class PrerequisiteGraph {
public:
    /** @brief 构建空的先修关系图 */
    PrerequisiteGraph() = default;

    /**
     * @brief 从数据集构建先修关系图
     * @param dataset 已加载的课程数据集
     * @details 遍历 course_map 中每门课的 prereq_IDs，建立有向边。
     *          只包含数据集中实际存在的课程节点。
     */
    void build(const CourseDataset& dataset);

    /**
     * @brief 获取某门课程的直接先修课程列表
     * @param course_id 课程基础序号
     * @return 先修课程ID列表（如不存在则返回空vector）
     */
    const std::vector<std::string>& get_prerequisites(const std::string& course_id) const;

    /**
     * @brief Kahn 拓扑排序，返回一个可行的线性顺序
     * @return 拓扑排序后的课程ID序列
     * @throws std::runtime_error 如果图中存在环
     */
    std::vector<std::string> topological_sort() const;

    /**
     * @brief 检查图中是否存在环路
     * @return 如果存在环返回true
     */
    bool has_cycle() const;

    /**
     * @brief 获取检测到的环路信息（调用 has_cycle 或 topological_sort 后有效）
     * @return 参与环路的课程ID集合
     */
    const std::set<std::string>& get_cycle_nodes() const { return cycle_nodes_; }

    /**
     * @brief 检查先修课程是否全部可达
     * @param target_ids 目标课程集合（如必修课集合）
     * @return 缺失的先修课ID集合（prereq 在数据中不存在）
     */
    std::set<std::string> check_missing_prerequisites(const std::set<std::string>& target_ids) const;

    /**
     * @brief 判断 course_id 的所有先修课程是否都已满足
     * @param course_id 待检查的课程
     * @param completed 已完成（已安排）的课程集合
     * @return 全部满足返回true
     */
    bool are_prerequisites_satisfied(const std::string& course_id,
                                     const std::set<std::string>& completed) const;

    /** @brief 获取图中节点总数 */
    int node_count() const { return static_cast<int>(adj_list_.size()); }

    /** @brief 获取图中边总数 */
    int edge_count() const { return edge_count_; }

    /** @brief 打印图的基本统计信息 */
    void print_stats() const;

private:
    /// 邻接表：pre_id → [后继课程ID列表]
    std::map<std::string, std::vector<std::string>> adj_list_;

    /// 逆邻接表：course_id → [先修课程ID列表]（用于快速查先修）
    std::map<std::string, std::vector<std::string>> reverse_adj_list_;

    /// 图中所有节点的集合
    std::set<std::string> all_nodes_;

    /// 环路检测结果（调试用，mutable 允许在 const 方法中写入）
    mutable std::set<std::string> cycle_nodes_;

    int edge_count_ = 0;

    /**
     * @brief Kahn 拓扑排序（内部实现），可控制是否抛出异常
     * @param throw_on_cycle 是否在检测到环时抛出异常
     * @return 排序结果（如果有环且 throw_on_cycle=false，返回部分结果）
     */
    std::vector<std::string> kahn_sort(bool throw_on_cycle) const;
};

}  // namespace course_planner