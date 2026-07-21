# 智能课程规划系统 — 本人的随记笔记：能学到新东西

## 一、项目构建与多平台支持

### （一）[main.cpp 里面](src/main.cpp)

#### 1. `#pragma once`——头文件保护的新写法

```cpp
#pragma once
```

- 以前我只会用 `#ifndef ... #define ... #endif` 防止头文件重复包含
- `#pragma once` 更简洁，一行搞定，几乎所有现代编译器都支持（MSVC、GCC、Clang）
- 注意：它不是 C++ 标准的一部分，是编译器扩展，但已事实上标准化

#### 2. `namespace`——C++ 命名空间

```cpp
namespace course_planner { ... }
namespace course_planner::utils { ... }
```

- `namespace course_planner::utils` 是 C++17 的**嵌套命名空间**写法，等价于 `namespace course_planner { namespace utils { ... } }`
- 作用：避免命名冲突，把代码按模块分组
- 常见用法：`using namespace course_planner;` 在当前文件引入整个命名空间

#### 3. `#ifdef _WIN32`——条件编译与跨平台代码

```cpp
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif
```

- `_WIN32` 是 MSVC 编译器预定义的宏，在 Windows 平台编译时自动定义
- 用 `#ifdef` 可以编写**平台相关的代码**：一套代码编译出 Windows 和 Linux 两个版本
- 本项目用 `GetModuleFileNameA`（Windows API）和 `readlink(/proc/self/exe)`（Linux）分别获取可执行文件路径

#### 4. `SetConsoleOutputCP(CP_UTF8)`——Windows 控制台编码设置

```cpp
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
```

- Windows 的 `cmd.exe` 默认使用 GBK 编码（代码页 936），输出 UTF-8 编码的中文会乱码
- `SetConsoleOutputCP(CP_UTF8)` 设置控制台输出代码页为 UTF-8（代码页 65001）
- `SetConsoleCP(CP_UTF8)` 设置控制台输入代码页为 UTF-8
- 这是解决"控制台中文乱码"问题的标准做法

#### 5. `GetModuleFileNameA`——获取当前 exe 文件路径

```cpp
char buffer[MAX_PATH];
DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
```

- `GetModuleFileNameA`：Windows API，获取当前进程的可执行文件完整路径
- 第一个参数传 `nullptr` 表示获取当前进程自身的 exe 路径
- `MAX_PATH`：Windows 定义的常量（260），表示文件路径最大长度
- 然后通过 `find_last_of("\\/")` 取目录部分，得到 `data/` 文件夹的相对基准路径

---

## 二、Header-Only 库的设计

### （一）[string_utils.h 里面](src/utils/string_utils.h)

#### 1. Header-Only 库的概念 + `inline` 关键字 + ODR（单一定义规则）

```cpp
// header-only：指这个库的实现全部写在 .h 头文件里，没有对应的 .cpp 文件。
// 我只要 #include 头文件，就能直接用，不需要链接额外的库。
```

**ODR（One Definition Rule，单一定义规则）**：

当在头文件里**定义**一个函数（而不只是声明）如 `void foo() { ... }`，如果这个头文件被两个 `.cpp` 文件 `#include`，那就相当于在两个翻译单元里都出现了这个函数的定义。编译器分别编译不报错，但**链接器**在合并目标文件时会看到两个一模一样的函数实现，不知道该用哪个，于是报 **"multiple definition"** 错误。

**`inline` 的作用**：加上 `inline` 后，C++ 标准允许内联函数在多个翻译单元中定义，链接器会接受它，并且只保留一份副本。因此 header-only 库里的函数**必须**标记 `inline`。

**总结**：`inline` 两个作用：① 建议编译器原地展开优化速度；② 在 header-only 场景下，让链接器接受多个翻译单元中的同名定义。

#### 2. `using` 类型别名——现代版 `typedef`

```cpp
using TimeSlotList = std::vector<TimeSlot>;     //给一个复杂的类型起个外号（别名）
```

- `using TimeSlotList = std::vector<TimeSlot>;` 等价于 `typedef std::vector<TimeSlot> TimeSlotList;`
- **更推荐 `using`**：语法更直观（`新名字 = 旧类型`），且支持模板别名
- 大幅提高长类型名的可读性

#### 3. `std::find_if_not` + Lambda——查找"不满足条件"的元素

```cpp
auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
    return std::isspace(ch);        //不是空白的位置
});
```

- `std::find_if_not` 返回**第一个不满足 lambda 条件**的迭代器
- Lambda `[](参数){ 函数体 }` 是 C++11 引入的匿名函数
- `std::isspace(ch)` 判断字符是否为空白（空格、制表、回车等）

#### 4. `rbegin()` / `rend()` + `.base()`——反向迭代器转换

```cpp
auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
    return std::isspace(ch);
}).base();
```

- `rbegin()` / `rend()`：**反向迭代器**，从末尾向开头遍历
- 反向迭代器的 `++` 是"向前一个元素移动"
- `.base()`：把**反向迭代器转换回普通迭代器**，用于构造子串
- 配合 `substr(start, end - start)` 就能截掉首尾空白

#### 5. UTF-8 BOM 是什么，以及如何处理

```cpp
// UTF-8 BOM 是某些软件（比如 Windows 记事本）在保存 UTF-8 文本文件时，
// 偷偷在文件最前面塞的三个特殊字节：EF BB BF。
// 肉眼看不见，但会影响程序解析（比如被当成你数据的第一行第一列）。
```

- **BOM** = Byte Order Mark（字节序标记）
- UTF-8 不需要 BOM（因为 UTF-8 没有字节序问题），但 Windows 记事本会强行加
- 处理方法：检查文件前 3 字节，如果是 `EF BB BF` 就跳过

```cpp
if (content.size() >= 3
    && static_cast<unsigned char>(content[0]) == 0xEF
    && static_cast<unsigned char>(content[1]) == 0xBB
    && static_cast<unsigned char>(content[2]) == 0xBF) {
    return content.substr(3);
}
```

- 这里必须用 `static_cast<unsigned char>` 转换，因为 `char` 可能是有符号的（-128~127），而 `0xEF = 239` 在有符号 char 里可能会变成负数，导致比较失败

#### 6. `std::stoi` / `std::stod`——字符串转数字的"安全"版本

```cpp
inline int safe_stoi(const std::string& s, int default_val = 0) {
    try {
        if (s.empty()) return default_val;
        return std::stoi(s);
    } catch (...) {
        return default_val;
    }
}
```

- `std::stoi` = **s**tring **to** **i**nt，`std::stod` = string to **d**ouble
- 如果字符串格式不对（如空字符串、`"abc"`），`stoi` 会抛异常
- 包装成 `safe_stoi`，用 `try-catch` 兜底，转换失败返回默认值

---

## 三、与文件有关的数据流

### （一）[csv_loader.h 里面](src/data/csv_loader.h)

#### 1. `std::string_view`——不拷贝的字符串视图

- C++17 引入，类似一个 `{指针, 长度}` 的轻量结构体
- 只"看"字符串，不拥有内存，不拷贝
- `read_file_content()` 读完文件后，将全部内容存为 `std::string`（拥有内存），后续操作才安全

#### 2. `std::optional`——可能没有值的返回值

```cpp
std::optional<ConflictInfo> check_conflict_with_list(...);
```

- `std::optional<T>` 是 C++17 引入的"可能为空的值"
- 有值 → 用 `.value()` 或 `*` 取；没值 → 返回 `std::nullopt`
- 比用指针（`nullptr`）更安全：明确表达了"可能无结果"的语义
- 比用 `bool` 加输出参数更优雅：不用额外参数来传递结果

---

## 四、JSON 解析器的设计与实现（零外部依赖）

### （一）[json_parser.h 里面](src/utils/json_parser.h)

#### 1. `std::variant` + 递归下降解析——C++17 的核心新知

```cpp
std::variant<bool, int64_t, double, std::string, Array, Object> value_;
// 键值对的值，std::variant 是 C++17 的"类型安全的联合体"
// 它可以在运行时持有这 6 种类型中的任意一种，但同一时刻只持有一种
```

**`std::variant` 是什么**：

- C++17 新增的**类型安全联合体**（type-safe union）
- 可以持有多种类型中的一种，通过 `std::get<T>()` 或 `std::visit()` 安全访问
- 比 C 语言的 `union` 更安全：访问类型不匹配会抛异常，而不是读到垃圾数据

**项目中的应用**：

- JSON 值可能是 bool、int、double、string、array、object 六种
- 用 `std::variant<bool, int64_t, double, std::string, Array, Object>` 统一存储
- number 类型进一步分 int64_t 和 double：JSON 的 `3` 和 `3.0` 是不同的

**递归下降解析器（Recursive Descent Parser）**：

```
parse_json("{\"a\":[1,2]}") 的思路：
  看到 { → 调用 parse_object()
    → 读 key "a"
    → 看到 : 后调用 parse_value()
      → 看到 [ → 调用 parse_array()
        → 读 1, 2
        → 遇到 ] 返回 array
    → 遇到 } 返回 object
```

每一步根据当前字符递归调用对应的解析函数，直到读出完整的嵌套结构。

---

## 五、先修关系图的拓扑排序

### （一）[prerequisite_graph.cpp 里面](src/core/prerequisite_graph.cpp)

#### 1. Kahn 算法——拓扑排序的标准解法

```cpp
// 原理：如果有环的话，有部分节点永远 in_degree != 0，
// 所以当 q 中 empty 的时候，还没有遍历完所有的node
// （result.size() != all_nodes_.size()）
```

**Kahn 算法步骤**：

1. 统计每个节点的入度（有多少条边指向它）
2. 入度为 0 的节点入队（不需要前置课程，可以"第一学期"学）
3. 出队 → 加入结果 → 其所有后继节点的入度减 1
4. 如果后继节点入度变为 0，入队
5. 循环直到队列为空
6. 如果结果大小 < 总节点数，则有环：剩余入度 > 0 的节点都是环参与者

**应用**：检查先修关系有没有死循环（A 需要先学 B，B 需要先学 A），如果有，报告哪些课程在环里。

#### 2. `map::count()` vs `map::find()`——检查键是否存在的两种写法

`if (dataset.course_map.count(pre_id) > 0)` 等价于 `if (dataset.course_map.find(pre_id) != dataset.course_map.end())`，前者更简洁。

注意：`map::count()` 对于 `std::map` 和 `std::set` 返回值只能是 0 或 1（因为键唯一），所以 `count > 0` 就是在判断"是否存在"。

#### 3. `std::queue`——广度优先搜索的基础容器

```cpp
std::queue<std::string> q;
q.push(node);           // 入队
std::string node = q.front();  // 取队首
q.pop();                // 出队（不返回值！）
```

- `pop()` 不返回值，必须先 `front()` 再 `pop()`——这是 C++ 的设计哲学：异常安全

---

## 六、调度算法的贪心策略

### （一）[scheduler.cpp 里面](src/core/scheduler.cpp)

#### 1. `std::function` + Lambda 递归——把函数当变量存

```cpp
std::function<int(const std::string&)> dfs = [&](const std::string& id) -> int {
    if (depth.count(id)) return depth[id];  // 记忆化
    int max_depth = 0;
    for (const auto& pre_id : prereqs) {
        max_depth = std::max(max_depth, dfs(pre_id) + 1);  // 递归
    }
    return depth[id] = max_depth;
};
```

- `std::function<int(const string&)>`：一个可以持有"接受 string 返回 int"的可调用对象的变量
- `[&]`：引用捕获外部所有变量（`depth`, `prereqs`）
- Lambda 递归必须用 `std::function`，不能直接用 `auto`——因为 `auto` 推导不出自己的类型
- **记忆化（Memoization）**：用 `depth` map 缓存已计算的结果，避免重复 DFS。每个节点只算一次，总复杂度 O(N+E)

#### 2. 浮点数比较加 epsilon——防止精度问题

```cpp
if (current_credit + basic.credit > max_credit + 0.001) continue;
// + 0.001：怕 current_credit + basic.credit 在 max_credit 下面一点点（浮点数精度影响）
```

浮点数 `0.1 + 0.2` 可能等于 `0.30000000000000004` 而不是 `0.3`。如果直接比较 `> max_credit`，本应等价的判断可能误判。加一个小 epsilon（容差值）是最简单有效的方法。

#### 3. 贪心算法的"最优点"理解

**负载均衡优化**：

```cpp
// 按当前学分从小到大排序的学期列表（空闲学期优先）
std::sort(ordered_terms.begin(), ordered_terms.end(), [&](int a, int b) {
    return result.semester_credits[a] < result.semester_credits[b];
    // 总学分小的学期排前面 → 选课时优先往空闲学期填
});
```

这是**贪心策略**的一个体现：每门选修课总是选择当前负载最低的合法学期。不保证全局最优，但在多数学分场景下能获得很好的均衡效果。

#### 4. `std::set` 追踪已完成状态

```cpp
std::set<std::string> scheduled_basic_ids_;  // 已安排课程的集合
if (scheduled_basic_ids_.count(elec_id)) continue;  // 同一门课只安排一次
```

- `std::set` 底层是红黑树，插入/查找都是 O(log n)
- 适合"是否已安排"、"是否已完成"这类去重场景
- `count()` 在这里只返回 0 或 1，用来判断"是否在集合中"

#### 5. `std::ostringstream`——把"流式输出"收集成字符串

```cpp
std::ostringstream oss;
oss << "某个变量: " << var << "\n";
return oss.str();  // 把前面 << 的所有内容合成一个 std::string
```

- 类似 `std::cout`，但输出目标是内存中的字符串
- 比 `+` 拼接长字符串更高效、可读性更好

---

## 七、`const` 成员函数——承诺"不修改对象状态"

```cpp
int end() const { return beg + last - 1; }
// 在 time_slot.h 中
```

- `const` 放在成员函数括号后面，表示**这个函数不会修改任何成员变量**
- 编译器会帮你检查：如果在 `const` 函数里改了成员变量，直接报错
- `const` 对象只能调用 `const` 成员函数，非 `const` 对象两者都能调

---

## 八、C++ 标准库容器选择总结

| 容器 | 底层结构 | 查找 | 插入 | 适用场景 |
|------|---------|------|------|---------|
| `std::vector` | 动态数组 | O(n) | O(1) amortized | 顺序遍历、按索引访问 |
| `std::map` | 红黑树 | O(log n) | O(log n) | 需要按键排序遍历 |
| `std::unordered_map` | 哈希表 | O(1) avg | O(1) avg | 纯按键查找、不关心顺序 |
| `std::set` | 红黑树 | O(log n) | O(log n) | 去重 + 自动排序 |
| `std::queue` | deque 适配器 | O(1) front | O(1) push | BFS / Kahn 拓扑排序 |
| `std::vector<bool>` | bitset 特化 | O(1) | O(1) | 访问比数组快，占内存小 |

本项目选择 `std::map<std::string, CourseBasic>` 而非 `unordered_map`，因为课程 ID 有排序需求（字典序输出），且 1000 条数据量下红黑树和哈希表的性能差异可以忽略。