/**
 * @file    scheduler.h
 * @brief   课程规划主调度器
 * @details 实现"拓扑排序 + 按学期贪心分配"的八学期课程规划算法：
 *          Phase 1: 对必修课进行拓扑排序，确定可行的修读顺序
 *          Phase 2: 按学期迭代(1→8)，贪心分配必修课（检测冲突+学分上限）
 *          Phase 3: 从选修池补足选修学分
 *          Phase 4: 方案验证（冲突/先修/学分全面检查）
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>

#include "models/course.h"
#include "models/constraint.h"
#include "data/csv_loader.h"
#include "conflict_detector.h"
#include "prerequisite_graph.h"

namespace course_planner {

/**
 * @brief 八学期课程规划结果
 */
struct ScheduleResult {
    /// 每学期的选课列表（学期号 1-8 → 教学班指针列表）
    std::map<int, std::vector<const CourseOffering*>> semester_courses;

    /// 每学期学分小计
    std::map<int, double> semester_credits;

    /// 总学分
    double total_credit = 0.0;

    /// 必修学分
    double required_credit_earned = 0.0;

    /// 选修学分
    double elective_credit_earned = 0.0;

    /// 无法安排的课程及原因
    struct UnassignedCourse {
        std::string course_basic_ID;
        std::string course_name;
        std::string reason;   ///< 无法安排的原因
    };
    std::vector<UnassignedCourse> unassigned_courses;

    /// 各学期的冲突列表
    std::map<int, std::vector<ConflictInfo>> conflicts;

    /// 规划是否成功（所有必修课已安排 + 满足学分要求）
    bool success = false;

    /// 汇总信息字符串
    std::string summary;

    /// 个性化时间安排：偏好满足度
    struct PreferenceReport {
        int total_preferred = 0;          ///< 偏好时段总数
        int total_avoid = 0;              ///< 避让时段总数
        int satisfied_preferred = 0;      ///< 满足的偏好时段数
        int satisfied_avoid = 0;          ///< 成功避让的时段数
        double satisfaction_rate = 0.0;   ///< 满足率 (0~1)
        std::string detail;               ///< 文字说明
    };
    std::map<int, PreferenceReport> semester_preferences; ///< 每学期的偏好报告
};

/**
 * @brief 课程规划主调度器
 *
 * 算法核心流程：
 * 1. 构建先修关系图，对必修课进行拓扑排序
 * 2. 按学期依次遍历(1→8)，在每个学期中：
 *    a. 筛选出"先修已满足 + 推荐学期≤当前 + 季节匹配"的必修课候选
 *    b. 按先修链长度优先排序（先修链越长越优先安排）
 *    c. 贪心选课：遍历候选 → 检查冲突 → 检查学分上限 → 加入
 *    d. 如果所有教学班都冲突，选择一个冲突最少的，或报告无法安排
 * 3. 从选修候选池中补足选修学分（同样贪心）
 * 4. 全面验证并生成报告
 */
class Scheduler {
public:
    /**
     * @brief 构造调度器
     * @param dataset 已加载的课程数据集
     * @param constraint 规划约束
     */
    Scheduler(const CourseDataset& dataset, const Constraint& constraint);

    /**
     * @brief 执行课程规划
     * @return 规划结果
     */
    ScheduleResult run();

    /** @brief 获取调度过程中产生的诊断信息 */
    const std::vector<std::string>& get_diagnostics() const { return diagnostics_; }

private:
    const CourseDataset& dataset_;
    const Constraint& constraint_;  //约束条件课程信息集合
    PrerequisiteGraph prereq_graph_;    //PrerequisiteGraph prereq_graph_;先修关系图
    ConflictDetector conflict_detector_;
    std::vector<std::string> diagnostics_;  ///< 诊断日志

    /// 已安排课程的集合（用于追踪先修满足情况）
    std::set<std::string> scheduled_basic_ids_;

    /// 剩余待安排的必修课ID集合。pending : 待处理的
    std::set<std::string> pending_required_ids_;

    /**
     * @brief 计算每个必修课的"先修链深度"（用于优先级排序）
     * @return course_id → 最长先修链长度
     */
    std::map<std::string, int> compute_prerequisite_depth() const;

    /**
     * @brief 筛选当前学期可考虑的必修课候选
     * @param term 当前学期号(1-8)
     * @return 候选课程ID列表（先修已满足 + 季节匹配）
     */
    std::vector<std::string> get_candidates_for_term(int term);

    /**
     * @brief 为一门课选择最佳教学班
     * @param course_id 课程ID
     * @param term 目标学期
     * @param current_semester_courses 当前学期已选课程
     * @param soft 是否使用软约束评分（用于选修课阶段）
     * @return 选中的教学班指针，如果全冲突返回nullptr
     */
    const CourseOffering* select_best_offering(
        const std::string& course_id,
        int term,
        const std::vector<const CourseOffering*>& current_semester_courses,
        bool soft = false);

    /**
     * @brief 选修课阶段：从候选池补足学分
     * @param result 规划结果（会被修改）
     */
    void fill_electives(ScheduleResult& result);

    /**
     * @brief 对规划结果进行全面验证
     * @param result 规划结果
     */
    void validate_result(ScheduleResult& result);

    /**
     * @brief 生成规划摘要
     * @param result 规划结果
     */
    void generate_summary(ScheduleResult& result);

    /**
     * @brief 分析个性化时间安排满足度（拓展功能）
     * @param result 规划结果（会被修改，填充 semester_preferences）
     */
    void analyze_preferences(ScheduleResult& result);
};

}  // namespace course_planner