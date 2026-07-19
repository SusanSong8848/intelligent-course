/**
 * @file    scheduler.cpp
 * @brief   课程规划主调度器实现
 * @details 实现"拓扑排序 + 贪心按学期分配"的八学期规划算法。
 *
 *          算法概述：
 *          1. 构建先修关系图，计算每个必修课的先修链深度
 *          2. 按学期依次(1→8)贪心分配必修课：
 *             - 选候选：先修已满足 + 季节匹配 + recommended_term≤当前学期
 *             - 优先级：先修链深度大者优先 → 学分高者优先
 *             - 选教学班：取第一个不冲突的；全冲突则报告
 *             - 学分超限则推迟到后续同季节学期
 *          3. 选修课阶段：同样贪心，直到总选修≥derived_min_elective
 *          4. 验证冲突/先修/学分 + 生成报告
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include "scheduler.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <functional>

namespace course_planner {

Scheduler::Scheduler(const CourseDataset& dataset, const Constraint& constraint)
    : dataset_(dataset), constraint_(constraint) {
    // 构建先修关系图
    prereq_graph_.build(dataset_);
}

// ============================================================================
// 计算先修链深度（DFS + 记忆化）
// ============================================================================

std::map<std::string, int> Scheduler::compute_prerequisite_depth() const {
    std::map<std::string, int> depth;
    // 对每门必修课，DFS 计算最长先修链深度
    std::function<int(const std::string&)> dfs = [&](const std::string& id) -> int {
        if (depth.count(id)) return depth[id];

        int max_depth = 0;
        const auto& prereqs = prereq_graph_.get_prerequisites(id);
        for (const auto& pre_id : prereqs) {
            max_depth = std::max(max_depth, dfs(pre_id) + 1);
        }
        depth[id] = max_depth;
        return max_depth;
    };

    for (const auto& id : constraint_.required_course_basic_IDs) {
        dfs(id);
    }
    return depth;
}

// ============================================================================
// 获取某学期可考虑的必修课候选
// ============================================================================

std::vector<std::string> Scheduler::get_candidates_for_term(int term) {
    std::vector<std::string> candidates;

    for (const auto& id : pending_required_ids_) {
        auto it = dataset_.course_map.find(id);
        if (it == dataset_.course_map.end()) continue;

        const auto& basic = it->second;

        // 条件1：该学期未被封锁（交换/实习）
        if (constraint_.is_term_blocked(term)) continue;

        // 条件2：季节匹配（秋季→奇数学期，春季→偶数学期）
        if (!term_matches_semester(term, basic.semester)) continue;

        // 条件3：recommended_term ≤ 当前学期（课程可以后移）
        if (basic.recommended_term > term) continue;

        // 条件4：先修课已全部完成
        if (!prereq_graph_.are_prerequisites_satisfied(id, scheduled_basic_ids_)) continue;

        candidates.push_back(id);
    }

    return candidates;
}

// ============================================================================
// 为一门课选择最佳教学班
// ============================================================================

const CourseOffering* Scheduler::select_best_offering(
    const std::string& course_id,
    int term,
    const std::vector<const CourseOffering*>& current_semester_courses,
    bool soft) {

    auto it = dataset_.course_map.find(course_id);
    if (it == dataset_.course_map.end()) return nullptr;

    const auto& basic = it->second;

    // 收集所有能排在此学期的教学班
    std::vector<const CourseOffering*> feasible;
    for (const auto& off : basic.offerings) {
        if (!off.can_schedule_in_term(term)) continue;

        // 检查是否与当前已选课程冲突
        auto conflict = ConflictDetector::check_conflict_with_list(off, current_semester_courses);
        if (!conflict.has_value()) {
            feasible.push_back(&off);
        }
    }

    if (feasible.empty()) return nullptr;

    // 如果使用软约束评分（选修课阶段），选得分最高的
    if (soft && !constraint_.preferred_time_blocks.empty()) {
        int best_score = -1;
        const CourseOffering* best = nullptr;
        for (const auto* off : feasible) {
            int score = 0;
            for (const auto& ts : off->time_slots) {
                for (const auto& pref : constraint_.preferred_time_blocks) {
                    if (pref.contains(ts)) score++;
                }
                for (const auto& avoid : constraint_.avoid_time_blocks) {
                    if (avoid.contains(ts)) score--;
                }
            }
            if (score > best_score) {
                best_score = score;
                best = off;
            }
        }
        return best ? best : feasible[0];
    }

    // 默认返回第一个可行的（教学班序号最小的）
    return feasible[0];
}

// ============================================================================
// 选修课阶段
// ============================================================================

void Scheduler::fill_electives(ScheduleResult& result) {
    std::set<std::string> elective_pool = constraint_.elective_candidate_course_basic_IDs;

    for (int term = 1; term <= 8; ++term) {
        if (constraint_.is_term_blocked(term)) continue;

        double current_credit = result.semester_credits[term];
        double max_credit = constraint_.max_credit_for_term(term);

        for (const auto& elec_id : elective_pool) {
            // 已被安排过（同一门课只安排一次）
            if (scheduled_basic_ids_.count(elec_id)) continue;

            // 检查先修是否满足
            if (!prereq_graph_.are_prerequisites_satisfied(elec_id, scheduled_basic_ids_)) continue;

            auto it = dataset_.course_map.find(elec_id);
            if (it == dataset_.course_map.end()) continue;
            const auto& basic = it->second;

            // 季节匹配
            if (!term_matches_semester(term, basic.semester)) continue;

            // 学分上限检查
            if (current_credit + basic.credit > max_credit) continue;

            // 选择最佳教学班（使用软约束评分）
            auto& semester_courses = result.semester_courses[term];
            const CourseOffering* best = select_best_offering(elec_id, term, semester_courses, true);
            if (best == nullptr) continue;

            // 加入
            semester_courses.push_back(best);
            scheduled_basic_ids_.insert(elec_id);
            current_credit += best->credit;
            result.semester_credits[term] = current_credit;
            result.total_credit += best->credit;
            result.elective_credit_earned += best->credit;

            // 选修学分已足？
            if (result.elective_credit_earned >= constraint_.derived_min_elective_credit_for_total
                && result.total_credit >= constraint_.min_total_credit) {
                return;  // 选修学分和总学分都够了
            }
        }
    }
}

// ============================================================================
// 结果验证
// ============================================================================

void Scheduler::validate_result(ScheduleResult& result) {
    // 1. 时间冲突检查
    for (int t = 1; t <= 8; ++t) {
        auto conflicts = ConflictDetector::check_semester_conflicts(
            result.semester_courses[t]);
        if (!conflicts.empty()) {
            result.conflicts[t] = conflicts;
        }
    }

    // 2. 先修关系检查（仅检查必修课，选修课在 fill_electives 阶段已做先修检查）
    std::set<std::string> scheduled_so_far;
    for (int t = 1; t <= 8; ++t) {
        for (const auto* off : result.semester_courses[t]) {
            bool is_required = constraint_.is_required_course(off->course_basic_ID);

            if (is_required && !prereq_graph_.are_prerequisites_satisfied(
                    off->course_basic_ID, scheduled_so_far)) {
                result.unassigned_courses.push_back({
                    off->course_basic_ID,
                    off->course_name,
                    "第" + std::to_string(t) + "学期：先修课程尚未完成"
                });
            }
            scheduled_so_far.insert(off->course_basic_ID);
        }
    }

    // 3. 学分检查
    for (int t = 1; t <= 8; ++t) {
        double max_cr = constraint_.max_credit_for_term(t);
        if (result.semester_credits[t] > max_cr) {
            std::cerr << "[验证] ⚠ 第" << t << "学期学分超限: "
                      << result.semester_credits[t] << " > " << max_cr << std::endl;
        }
    }

    // 4. 判断是否成功
    result.success = result.unassigned_courses.empty()
                  && result.conflicts.empty()
                  && result.total_credit >= constraint_.min_total_credit
                  && result.elective_credit_earned >= constraint_.derived_min_elective_credit_for_total;
}

// ============================================================================
// 生成摘要
// ============================================================================

void Scheduler::generate_summary(ScheduleResult& result) {
    std::ostringstream oss;
    oss << "\n========================================\n";
    oss << "  课程规划摘要\n";
    oss << "========================================\n";
    oss << "规划结果: " << (result.success ? "✅ 成功" : "❌ 存在未解决的问题") << "\n\n";

    oss << "--- 学分统计 ---\n";
    oss << "总学分:    " << result.total_credit
        << " (要求≥" << constraint_.min_total_credit << ")\n";
    oss << "必修学分:  " << result.required_credit_earned
        << " (要求≥" << constraint_.required_credit << ")\n";
    oss << "选修学分:  " << result.elective_credit_earned
        << " (要求≥" << constraint_.derived_min_elective_credit_for_total << ")\n\n";

    oss << "--- 每学期学分 ---\n";
    for (int t = 1; t <= 8; ++t) {
        oss << "第" << t << "学期: " << result.semester_credits[t]
            << " / " << constraint_.max_credit_for_term(t)
            << " (" << result.semester_courses[t].size() << " 门课)";
        if (constraint_.is_term_blocked(t)) oss << " [封锁]";
        oss << "\n";
    }

    if (!result.unassigned_courses.empty()) {
        oss << "\n--- 未安排课程 ---\n";
        for (const auto& uc : result.unassigned_courses) {
            oss << "  " << uc.course_name << ": " << uc.reason << "\n";
        }
    }

    if (!result.conflicts.empty()) {
        oss << "\n--- 冲突详情 ---\n";
        for (const auto& [term, conflicts] : result.conflicts) {
            for (const auto& ci : conflicts) {
                oss << "  第" << term << "学期: " << ci.detail << "\n";
            }
        }
    }

    result.summary = oss.str();
}

// ============================================================================
// 主调度入口
// ============================================================================

ScheduleResult Scheduler::run() {
    ScheduleResult result;
    diagnostics_.clear();

    // 初始化各学期
    for (int t = 1; t <= 8; ++t) {
        result.semester_courses[t] = {};
        result.semester_credits[t] = 0.0;
    }

    // Phase 0：检查缺失先修课
    auto missing = prereq_graph_.check_missing_prerequisites(
        constraint_.required_course_basic_IDs);
    if (!missing.empty()) {
        std::cerr << "[调度] ⚠ 检测到 " << missing.size() << " 门缺失的先修课程" << std::endl;
        for (const auto& mid : missing) {
            result.unassigned_courses.push_back({
                "", "缺失先修课: " + mid,
                "该先修课程不在数据集中"
            });
        }
    }

    // Phase 1：初始化待安排列表
    pending_required_ids_ = constraint_.required_course_basic_IDs;
    scheduled_basic_ids_.clear();

    // 计算优先级（按先修链深度降序 → 学分降序）
    auto depth_map = compute_prerequisite_depth();

    // Phase 2：贪心按学期分配必修课
    for (int term = 1; term <= 8; ++term) {
        if (constraint_.is_term_blocked(term)) {
            diagnostics_.push_back("第" + std::to_string(term) + "学期被封锁（交换/实习），跳过");
            continue;
        }

        double current_credit = 0.0;
        double max_credit = constraint_.max_credit_for_term(term);

        // 本轮尝试安排的最大次数（防止无限循环）
        int max_attempts = 200;
        int attempts = 0;

        while (attempts < max_attempts) {
            attempts++;

            auto candidates = get_candidates_for_term(term);
            if (candidates.empty()) break;

            // 按优先级排序：深度大 → 学分高 → ID字典序
            std::sort(candidates.begin(), candidates.end(),
                [&](const std::string& a, const std::string& b) {
                    int da = depth_map.count(a) ? depth_map[a] : 0;
                    int db = depth_map.count(b) ? depth_map[b] : 0;
                    if (da != db) return da > db;

                    auto it_a = dataset_.course_map.find(a);
                    auto it_b = dataset_.course_map.find(b);
                    double ca = (it_a != dataset_.course_map.end()) ? it_a->second.credit : 0;
                    double cb = (it_b != dataset_.course_map.end()) ? it_b->second.credit : 0;
                    if (ca != cb) return ca > cb;
                    return a < b;
                });

            bool placed_any = false;
            for (const auto& id : candidates) {
                auto it = dataset_.course_map.find(id);
                if (it == dataset_.course_map.end()) continue;
                const auto& basic = it->second;

                // 学分上限检查
                if (current_credit + basic.credit > max_credit + 0.001) {
                    continue;  // 学分超限，留到后续学期
                }

                // 选择教学班
                auto& semester_courses = result.semester_courses[term];
                const CourseOffering* selected = select_best_offering(
                    id, term, semester_courses, false);

                if (selected != nullptr) {
                    // 成功加入
                    semester_courses.push_back(selected);
                    scheduled_basic_ids_.insert(id);
                    pending_required_ids_.erase(id);
                    current_credit += selected->credit;
                    result.semester_credits[term] = current_credit;
                    result.total_credit += selected->credit;
                    result.required_credit_earned += selected->credit;
                    placed_any = true;
                }
                // 如果所有教学班都冲突，不在此学期安排，留给后续学期
            }

            if (!placed_any) break;  // 本轮无法再安排新课
        }

        result.semester_credits[term] = current_credit;
    }

    // Phase 2.5：处理仍未安排的必修课
    for (const auto& id : pending_required_ids_) {
        auto it = dataset_.course_map.find(id);
        std::string name = (it != dataset_.course_map.end()) ? it->second.course_name : id;
        result.unassigned_courses.push_back({
            id, name,
            "所有教学班时间冲突或学分上限不足，无法安排"
        });
    }

    // Phase 3：选修课
    fill_electives(result);

    // Phase 4：验证
    validate_result(result);

    // Phase 5：摘要
    generate_summary(result);

    prereq_graph_.print_stats();
    std::cout << result.summary << std::endl;

    return result;
}

}  // namespace course_planner