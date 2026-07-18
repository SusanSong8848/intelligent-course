/**
 * @file    main.cpp
 * @brief   智能课程规划系统 - 程序入口
 * @details 阶段一的数据加载验证程序，后续将扩展为完整的规划调度器。
 *
 *          当前功能：
 *          1. 加载 course_info.csv 和 course_time.csv 数据
 *          2. 加载 JSON 约束配置文件
 *          3. 打印数据集统计信息和课程详情用于验证
 *          4. 通过 get_exe_dir() 自动定位 data 目录
 *
 *          构建方式：
 *          cmake -B build -S . && cmake --build build --config Release
 *          运行：build/Release/course_planner.exe
 *
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include <iostream>
#include <string>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include "data/csv_loader.h"
#include "data/json_loader.h"
#include "models/course.h"
#include "models/constraint.h"

using namespace course_planner;

/**
 * @brief 获取可执行文件所在目录的绝对路径
 *
 * Windows 使用 GetModuleFileNameA API，
 * Linux 使用 /proc/self/exe 符号链接。
 *
 * @return exe 所在目录路径字符串
 */
std::string get_exe_dir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (len > 0) {
        std::string path(buffer, len);
        auto pos = path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return path.substr(0, pos);
        }
    }
    return ".";
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string path(buffer);
        auto pos = path.find_last_of('/');
        if (pos != std::string::npos) {
            return path.substr(0, pos);
        }
    }
    return ".";
#endif
}

/**
 * @brief 根据文件名构造 data 目录下的完整路径
 * @param filename data 目录下的文件名
 * @return 完整文件路径
 */
std::string data_path(const std::string& filename) {
    return get_exe_dir() + "/data/" + filename;
}

/**
 * @brief 打印数据集统计信息
 *
 * 输出内容包括：
 * - 总教学班数、课程基础数、时间记录数
 * - 按开课季节、课程类别、推荐学期的课程分布
 *
 * @param dataset 已加载的课程数据集
 */
void print_dataset_stats(const CourseDataset& dataset) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  课程数据集统计" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "总教学班数: " << dataset.total_offerings << std::endl;
    std::cout << "课程基础数: " << dataset.total_courses << std::endl;
    std::cout << "时间记录数: " << dataset.total_time_slots << std::endl;

    std::map<std::string, int> semester_count;
    std::map<std::string, int> category_count;
    std::map<int, int> term_count;

    for (const auto& [id, basic] : dataset.course_map) {
        semester_count[basic.semester]++;
        category_count[basic.category]++;
        term_count[basic.recommended_term]++;
    }

    std::cout << "\n--- 按开课季节 ---" << std::endl;
    for (const auto& [sem, cnt] : semester_count) {
        std::cout << "  " << sem << ": " << cnt << " 门" << std::endl;
    }

    std::cout << "\n--- 按课程类别 ---" << std::endl;
    for (const auto& [cat, cnt] : category_count) {
        std::cout << "  " << cat << ": " << cnt << " 门" << std::endl;
    }

    std::cout << "\n--- 按推荐学期 ---" << std::endl;
    for (int t = 1; t <= 8; ++t) {
        std::cout << "  第" << t << "学期: " << term_count[t] << " 门" << std::endl;
    }
}

/**
 * @brief 打印指定课程的详细信息
 *
 * 输出课程名称、院系、学分、先修关系、所有教学班的教师/教室/时间等信息。
 *
 * @param dataset 课程数据集
 * @param basic_id 课程基础序号
 */
void print_course_detail(const CourseDataset& dataset, const std::string& basic_id) {
    auto it = dataset.course_map.find(basic_id);
    if (it == dataset.course_map.end()) {
        std::cout << "[错误] 课程 " << basic_id << " 未找到" << std::endl;
        return;
    }

    const auto& basic = it->second;
    std::cout << "\n--- 课程详情 ---" << std::endl;
    std::cout << "ID:       " << basic.course_basic_ID << std::endl;
    std::cout << "名称:     " << basic.course_name << std::endl;
    std::cout << "院系:     " << basic.department << std::endl;
    std::cout << "季节:     " << basic.semester << std::endl;
    std::cout << "推荐学期: " << basic.recommended_term << std::endl;
    std::cout << "类别:     " << basic.category << std::endl;
    std::cout << "学分:     " << basic.credit << std::endl;

    if (!basic.prereq_IDs.empty()) {
        std::cout << "先修课:   ";
        for (size_t i = 0; i < basic.prereq_IDs.size(); ++i) {
            if (i > 0) std::cout << "; ";
            std::cout << basic.prereq_IDs[i];
        }
        std::cout << std::endl;
    }

    std::cout << "教学班数: " << basic.offerings.size() << std::endl;
    for (const auto& off : basic.offerings) {
        std::cout << "  班级 " << off.course_sp_ID
                  << " | 教师: " << off.teacher
                  << " | 教室: " << off.classroom
                  << " | 容量: " << off.limits
                  << " | 周次: " << off.beg_week << "-" << (off.beg_week + off.last_week - 1);

        if (!off.time_slots.empty()) {
            std::cout << " | 时间: ";
            for (size_t i = 0; i < off.time_slots.size(); ++i) {
                if (i > 0) std::cout << ", ";
                const auto& ts = off.time_slots[i];
                std::cout << ts.day << " " << ts.beg << "-" << ts.end();
            }
        }
        std::cout << std::endl;
    }
}

/**
 * @brief 打印约束配置的详细信息
 *
 * 输出目标院系、必修/选修课程数、学分上下限、避让时段、偏好时段等。
 *
 * @param constraint 从 JSON 加载的约束对象
 */
void print_constraint_info(const Constraint& constraint) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  约束配置信息" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "配置名称:   " << constraint.profile_name << std::endl;
    std::cout << "目标院系:   " << constraint.target_department << std::endl;
    std::cout << "必修课程数: " << constraint.required_course_basic_IDs.size() << std::endl;
    std::cout << "选修候选数: " << constraint.elective_candidate_course_basic_IDs.size() << std::endl;
    std::cout << "必修学分:   " << constraint.required_credit << std::endl;
    std::cout << "选修最低:   " << constraint.elective_min_credit << std::endl;
    std::cout << "总学分下限: " << constraint.min_total_credit << std::endl;
    std::cout << "补足选修:   " << constraint.derived_min_elective_credit_for_total << std::endl;

    std::cout << "\n--- 每学期学分上限 ---" << std::endl;
    for (int t = 1; t <= 8; ++t) {
        std::cout << "  第" << t << "学期: " << constraint.max_credit_for_term(t) << std::endl;
    }

    if (!constraint.avoid_time_blocks.empty()) {
        std::cout << "\n--- 避让时段 ---" << std::endl;
        for (const auto& tb : constraint.avoid_time_blocks) {
            std::cout << "  " << tb.day << " " << tb.beg << "-" << (tb.beg + tb.last - 1)
                      << " [" << (tb.hard ? "硬约束" : "软约束") << "] " << tb.reason << std::endl;
        }
    }

    if (!constraint.preferred_time_blocks.empty()) {
        std::cout << "\n--- 偏好时段 ---" << std::endl;
        for (const auto& tb : constraint.preferred_time_blocks) {
            std::cout << "  " << tb.day << " " << tb.beg << "-" << (tb.beg + tb.last - 1) << std::endl;
        }
    }
}

/**
 * @brief 程序主入口
 *
 * 执行三阶段数据加载：
 * 1. 加载课程基本信息（course_info.csv）
 * 2. 加载开课时间信息（course_time.csv）
 * 3. 加载规划约束（sample_constraints.json）
 *
 * 然后打印数据集统计、约束信息和示例课程详情。
 *
 * @return 0 成功，1 失败
 */
int main() {
    try {
        std::cout << "========================================" << std::endl;
        std::cout << "  智能课程规划系统 - 数据加载测试" << std::endl;
        std::cout << "========================================" << std::endl;

        // Step 1: 加载 course_info.csv
        std::cout << "\n[1/3] 加载课程信息..." << std::endl;
        std::string info_csv = read_file_content(data_path("course_info.csv"));
        CourseDataset dataset;
        int info_count = load_course_info_csv(info_csv, dataset);

        // Step 2: 加载 course_time.csv
        std::cout << "\n[2/3] 加载开课时间..." << std::endl;
        std::string time_csv = read_file_content(data_path("course_time.csv"));
        int time_count = load_course_time_csv(time_csv, dataset);

        // Step 3: 加载约束配置
        std::cout << "\n[3/3] 加载约束配置..." << std::endl;
        Constraint constraint = load_constraints_from_json(data_path("sample_constraints.json"));

        // 打印统计信息
        print_dataset_stats(dataset);
        print_constraint_info(constraint);

        // 示例：查看某门必修课的详细信息
        std::cout << "\n========================================" << std::endl;
        std::cout << "  示例：查看必修课程" << std::endl;
        std::cout << "========================================" << std::endl;

        if (!constraint.required_course_basic_IDs.empty()) {
            std::string sample_id = *constraint.required_course_basic_IDs.begin();
            print_course_detail(dataset, sample_id);
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "  阶段一：数据加载验证完成！" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "加载教学班: " << info_count << " 条" << std::endl;
        std::cout << "加载时间记录: " << time_count << " 条" << std::endl;
        std::cout << "必修课程: " << constraint.required_course_basic_IDs.size() << " 门" << std::endl;
        std::cout << "选修候选: " << constraint.elective_candidate_course_basic_IDs.size() << " 门" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[致命错误] " << e.what() << std::endl;
        return 1;
    }
}