/**
 * @file    csv_loader.cpp
 * @brief   CSV 数据加载器实现
 * @details 实现课程数据的 CSV 文件加载逻辑：
 *          - parse_csv_line: 带引号转义的 CSV 行解析
 *          - read_file_content: 文件读取 + BOM 去除
 *          - load_course_info_csv: 从 course_info.csv 构建 CourseBasic 和 CourseOffering
 *          - load_course_time_csv: 从 course_time.csv 为教学班补充时间槽
 *          加载完成后同步 course_map 和 all_offerings 保证数据一致性。
 * @author  IntelligentCoursePlanning Team
 * @date    2026-07
 */

#include "csv_loader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include "utils/string_utils.h"

namespace course_planner {

// ============================================================================
// CourseDataset 成员函数
// ============================================================================

const CourseOffering* CourseDataset::find_offering(
    const std::string& basic_id, const std::string& sp_id) const {
    auto it = course_map.find(basic_id);
    if (it == course_map.end()) return nullptr;
    for (const auto& off : it->second.offerings) {
        if (off.course_sp_ID == sp_id) return &off;
    }
    return nullptr;
}

const std::vector<CourseOffering>* CourseDataset::get_offerings(
    const std::string& basic_id) const {
    auto it = course_map.find(basic_id);
    if (it == course_map.end()) return nullptr;
    return &it->second.offerings;
}

// ============================================================================
// CSV 行解析
// ============================================================================

/**
 * @brief 解析一行 CSV 文本为字段数组
 *
 * 支持双引号包裹的字段，以及引号内的转义双引号 ""。
 * 例如: hello,"world, foo",bar -> ["hello", "world, foo", "bar"]
 *       "He said ""hi""",ok -> ["He said \"hi\"", "ok"]
 *
 * @param line 一行 CSV 文本
 * @return 解析后的字段数组（每个字段已 trim）
 */
std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];

        if (in_quotes) {            //有前"的时候
            if (ch == '"') {
                // 检查是否是转义引号 ""
                if (i + 1 < line.size() && line[i + 1] == '"') {    //又出现两个""(curr和下一个都是)
                    current += '"';
                    ++i;  // 跳过下一个引号
                } else {
                    in_quotes = false;
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == '"') {
                in_quotes = true;
            } else if (ch == ',') {     //只有此时,不在""中时才能截断
                fields.push_back(utils::trim(current));     //去除字符串首尾空白字符（空格、制表符、回车等）
                current.clear();
            } else {
                // 跳过回车符（Windows换行符 \r\n 中的 \r）
                if (ch != '\r') {
                    current += ch;
                }
            }
        }
    }
    // 最后一个字段
    fields.push_back(utils::trim(current));

    return fields;
}

// ============================================================================
// 文件读取
// ============================================================================

/**
 * @brief 读取整个文件内容为字符串
 *
 * 以二进制模式打开文件，读取全部内容，然后自动去除 UTF-8 BOM 头。
 *
 * @param filepath 文件路径
 * @return 文件内容字符串
 * @throws std::runtime_error 文件无法打开时抛出
 */
std::string read_file_content(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filepath);
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();        /*std::ifstream file(...) 是连接到文件的输入水管。
                                            std::ostringstream oss 是一个内存里的字符串水槽。
                                            oss << file.rdbuf(); 相当于把 file 水管里的所有数据（文件全部内容）倒进 oss 这个水槽里。
                                            std::string content = oss.str(); 把水槽里的水倒进一个 std::string 容器里，得到文件的全部文本内容。*/

                                            /*为什么不直接 std::string content << file.rdbuf(); ？
                                             不能。 C++ 的 std::string 没有重载 << 来直接接收一个文件缓冲。根本原因是 类型不匹配*/
    // 去除 UTF-8 BOM
    content = utils::remove_bom(content);       /*UTF-8 BOM 是某些软件（比如 Windows 记事本）在保存 UTF-8 文本文件时，偷偷在文件最前面塞的三个特殊字节：EF BB BF。肉眼看不见，但会影响程序解析（比如被当成数据的第一行第一列）。*/

    return content;
}

// ============================================================================
// course_info.csv 加载
// ============================================================================

/**
 * @brief 从 course_info.csv 内容加载课程信息和教学班
 *
 * 处理流程：
 * 1. 跳过表头行
 * 2. 逐行解析 14 个字段
 * 3. 构造 CourseOffering 对象
 * 4. 首次遇到某个 course_basic_ID 时创建 CourseBasic 并解析先修关系
 * 5. 将教学班加入对应 CourseBasic 的 offerings 列表和 all_offerings
 *
 * @param csv_content CSV 文件内容
 * @param dataset 输出数据集
 * @return 成功加载的教学班数
 */
int load_course_info_csv(const std::string& csv_content, CourseDataset& dataset) {
    std::istringstream stream(csv_content);
    std::string line;
    int line_number = 0;      //用于输出错误信息
    int loaded_count = 0;       //真正被记载的好行数

    // 第1行是表头，跳过
    if (!std::getline(stream, line)) {      //getline(stream, line)这里已经完成了一次读行
        std::cerr << "[警告] CSV文件为空" << std::endl;
        return 0;
    }
    line_number++;

    // 逐行解析
    while (std::getline(stream, line)) {
        line_number++;
        if (line.empty()) continue;

        auto fields = parse_csv_line(line);     //class std::vector<std::string>
        if (fields.size() < 14) {
            std::cerr << "[警告] 第" << line_number << "行字段不足(" 
                      << fields.size() << "/14)，跳过" << std::endl;
            continue;
        }

        CourseOffering offering;
        offering.course_basic_ID = fields[0];
        offering.course_sp_ID    = fields[1];       //这里CourseOffering会记录course_sp_ID
        offering.course_name     = fields[2];
        offering.department      = fields[3];
        offering.semester        = fields[4];
        offering.recommended_term = utils::safe_stoi(fields[5], 1);
        offering.category        = fields[6];
        offering.credit          = utils::safe_stod(fields[7], 0.0);
        offering.beg_week        = utils::safe_stoi(fields[8], 1);
        offering.last_week       = utils::safe_stoi(fields[9], 1);
        offering.teacher         = fields[10];
        offering.classroom       = fields[11];
        offering.limits          = utils::safe_stoi(fields[12], 100);
        offering.prereq_ID_raw   = fields[13];      //.csv每行最后都有","，这里可能只是空string

        // 查找或创建 CourseBasic   //（1）
        auto& basic = dataset.course_map[offering.course_basic_ID];     //course_planner::CourseBasic   /*map[key] 如果 key 不存在，会默认创建一个空值插入到 map 中，然后返回这个新创建元素的引用。*/
        if (basic.course_basic_ID.empty()) {
            // 首次遇到该课程，填充基本信息
            basic.course_basic_ID = offering.course_basic_ID;
            basic.course_name     = offering.course_name;       //这里CourseBasic不会单独记录course_sp_ID，都是在 CourseBasic::offerings 里（下面）
            basic.department      = offering.department;
            basic.semester        = offering.semester;
            basic.recommended_term = offering.recommended_term;
            basic.category        = offering.category;
            basic.credit          = offering.credit;

            // 解析先修课程ID（分号分隔）
            if (!offering.prereq_ID_raw.empty()) {  //如果有先修课程
                basic.prereq_IDs = utils::split(offering.prereq_ID_raw, ';');   //inline std::vector<std::string> course_planner::utils::split(const std::string &str, char delimiter)
            }
        }

        // 将该教学班加入课程基础   //（2）
        basic.offerings.push_back(offering);    //offering 只在 course_map 里存一份（单一数据源）
        loaded_count++;
    }

    // 构建 all_offerings 指针数组（指向 course_map 中的真实对象，无需同步）
    dataset.all_offerings.clear();
    for (auto& [id, basic] : dataset.course_map) {
        for (auto& off : basic.offerings) {
            dataset.all_offerings.push_back(&off);  // 存指针，不复制
        }
    }

    dataset.total_offerings = loaded_count;
    dataset.total_courses = static_cast<int>(dataset.course_map.size());

    std::cout << "[加载] course_info.csv: 共解析 " << loaded_count
              << " 个教学班，覆盖 " << dataset.total_courses << " 门课程" << std::endl;

    return loaded_count;
}

// ============================================================================
// course_time.csv 加载
// ============================================================================

/**
 * @brief 从 course_time.csv 内容为教学班补充上课时间信息
 *
 * 处理流程：
 * 1. 跳过表头行
 * 2. 逐行解析 5 个字段 (course_basic_ID, course_sp_ID, day, beg, last)
 * 3. 构造 TimeSlot 并关联到对应的教学班
 * 4. 加载完成后同步 all_offerings（从 course_map 重建）
 *
 * 同步措施：
 * 由于 all_offerings 和 course_map 中的 offerings 是独立副本，
 * 必须在此函数末尾重建 all_offerings 以保证时间槽数据一致。
 *
 * @param csv_content CSV 文件内容
 * @param dataset 已加载课程信息的数据集
 * @return 成功加载的时间槽记录数
 */
int load_course_time_csv(const std::string& csv_content, CourseDataset& dataset) {
    std::istringstream stream(csv_content);
    std::string line;
    int line_number = 0;
    int loaded_count = 0;

    // 跳过表头
    if (!std::getline(stream, line)) {
        std::cerr << "[警告] course_time.csv为空" << std::endl;
        return 0;
    }
    line_number++;

    while (std::getline(stream, line)) {
        line_number++;
        if (line.empty()) continue;

        auto fields = parse_csv_line(line);     //class std::vector<std::string>
        if (fields.size() < 5) {
            std::cerr << "[警告] course_time.csv 第" << line_number
                      << "行字段不足(" << fields.size() << "/5)，跳过" << std::endl;
            continue;
        }

        std::string basic_id = fields[0];
        std::string sp_id    = fields[1];
        TimeSlot slot;                              //本次重点
        slot.day   = fields[2];
        slot.beg   = utils::safe_stoi(fields[3], 1);
        slot.last  = utils::safe_stoi(fields[4], 1);

        // 找到对应的课程基础
        auto course_it = dataset.course_map.find(basic_id);
        if (course_it == dataset.course_map.end()) {    //没找到该门课程
            std::cerr << "[警告] course_time.csv 第" << line_number
                      << "行: 课程基础ID " << basic_id << " 在course_info中未找到" << std::endl;
            continue;
        }

        // 在 course_map 中找到对应的教学班并添加时间段
        bool found = false;
        for (auto& offering : course_it->second.offerings) {    //course_planner::CourseBasic::offerings ： 该课程的所有教学班
            if (offering.course_sp_ID == sp_id) {
                offering.time_slots.push_back(slot);    //（1）CourseBasic::offerings 要更新 time_slots
                found = true;
                break;
            }
        }

        if (!found) {       //该门课程没找到该教学班
            std::cerr << "[警告] course_time.csv 第" << line_number
                      << "行: 教学班 " << basic_id << "_" << sp_id << " 未找到" << std::endl;
        } else {
            loaded_count++;
        }
    }

    // all_offerings 存的是指向 course_map 中真实对象的指针，
    // 对 course_map 中 offering 的修改（如添加 time_slots）会通过指针自动反映，无需同步。

    dataset.total_time_slots = loaded_count;

    std::cout << "[加载] course_time.csv: 共解析 " << loaded_count
              << " 条上课时间记录" << std::endl;

    return loaded_count;
}

}  // namespace course_planner