# AI 选课建议 SKILL 定义文档

> **说明**：本系统支持两种 AI 选课建议模式：
> - 🟢 **DeepSeek API 在线模式**：用户在 AI 顾问页面输入自己的 API Key（存储在浏览器 localStorage，不暴露于代码中），系统将真实课程数据和当前规划结果作为上下文发送给 DeepSeek 大模型，获得智能化的选课建议。
> - 🟠 **离线模拟模式**（默认）：基于关键词意图识别 + 约束模板库 + 模板化解释生成，无需任何 API Key。
>
> 以下 SKILL 定义说明如何将自然语言需求转换为结构化约束、调用规划器，并解释结果。

---

## 1. SKILL 元信息

```json
{
  "skill_name": "intelligent_course_advisor",
  "version": "1.0",
  "description": "将用户自然语言选课需求转换为结构化约束，调用课程规划器，并生成可解释的选课建议",
  "input": "用户自然语言描述",
  "output": "自然语言建议 + 结构化方案",
  "mock_mode": true,
  "mock_note": "离线模拟，不使用真实 API Key，基于关键词匹配和约束模板实现"
}
```

## 2. 自然语言 → 结构化约束 转换规则

### 2.1 意图识别

| 关键词 | 意图 | 约束操作 |
|--------|------|---------|
| "学分少" / "压力小" / "轻松" / "不超过" | 降低学分上限 | `max_credit_per_semester[t] = N` |
| "不想" / "避开" / "不安排" / "尽量不要" | 添加避让时段 | `avoid_time_blocks.append(...)` |
| "周五" / "周末" / "晚课" / "早八" / "下午" | 时间段识别 | 映射为 day + beg + last |
| "尽快" / "提前" / "早点" | 缩短修读年限 | 提高前几学期下限 |
| "交换" / "实习" / "不排课" | 封锁学期 | `blocked_terms = {t}` |
| "评价" / "检查" / "当前方案" | 方案评估 | 调用 `analyze_preferences` |

### 2.2 时间段映射表

| 用户表达 | day | beg | last |
|---------|-----|-----|------|
| 周五下午 | Fri | 6 | 4 |
| 周末 | Sat/Sun | 1 | 13 |
| 晚课 | Mon-Fri | 11 | 3 |
| 早八 | Mon-Fri | 1 | 1 |
| 下午第一节 | Mon-Fri | 6 | 1 |

### 2.3 结构化约束输出格式

```json
{
  "intent": "reduce_credit_load",
  "action": "modify_constraint",
  "constraint_changes": {
    "max_credit_per_semester": { "1": 18, "2": 18, "3": 18 },
    "avoid_time_blocks": [
      { "day": "Fri", "beg": 6, "last": 4, "hard": false, "reason": "用户要求避免" }
    ]
  }
}
```

## 3. 调用规划器

```cpp
// 1. 加载原始约束
Constraint base = load_constraints_from_json("sample_constraints.json");

// 2. 应用 AI 建议的约束修改
base.max_credit_per_semester[1] = 18;
base.max_credit_per_semester[2] = 18;
base.max_credit_per_semester[3] = 18;
base.avoid_time_blocks.push_back({"Fri", 6, 4, false, "AI建议：尽量避免周五下午"});

// 3. 运行规划器
Scheduler scheduler(dataset, base);
ScheduleResult result = scheduler.run();
```

## 4. 结果解释生成

### 4.1 解释模板

```
根据您的需求"[用户输入]"，我已做出以下调整：

📊 约束调整：
- 将前X学期学分上限降至 Y
- 添加避让时段：Z

📋 规划结果：
- 总学分：A / B
- 必修课：全部完成 ✅
- 冲突数：0

💡 建议说明：
[动态生成的解释文本]
```

### 4.2 动态解释逻辑

```python
def generate_explanation(result, user_input):
    if result.total_credit < constraint.min_total_credit:
        return "当前方案无法满足毕业学分要求，建议放宽学分上限或增加选修课。"
    if result.unassigned_count > 0:
        return f"有{result.unassigned_count}门必修课无法安排，原因是学分上限过低。建议放宽或后移部分课程。"
    if result.preference_satisfaction_rate < 0.5:
        return "时间偏好满足率偏低，可能是因为基础课时间段固定。建议放宽偏好约束。"
    return "方案已生成，满足所有约束和偏好。"
```

## 5. 示例对话

```text
User: "我希望前三学期课程压力小一些，每学期不超过18学分，尽量不要安排周五下午的课程"
AI:  "我会把前3学期学分上限设置为18，并把周五下午设为软避开时段，
     然后优先安排先修链较长的基础课程。已生成方案：总学分151.5，必修课全部完成，
     选修课分布在4-6学期。周五下午课程已尽量避开（避让率49%）。"

User: "我想尽快修完机器学习相关课程"
AI:  "建议先完成数学分析、线性代数、程序设计、数据结构等先修课程。
     机器学习相关课程（机器学习实践、深度学习、强化学习实践）需要这些先修课完成后方可修读。
     在满足学分上限的前提下，这些课程将在第3-5学期安排。"

User: "请对我当前的选课情况给出评价"
AI:  "当前方案总学分151.5达到毕业要求。但第1学期负载偏高(39.5/40, 16门课)。
     如果你更重视时间均衡，可以把一门专业选修课后移。偏好满足率约48-55%。"
```

## 6. 模拟实现说明

本系统采用**离线模拟接口**，不依赖外部 LLM API。核心实现：

1. **关键词意图识别器**：基于正则匹配的意图分类
2. **约束模板库**：预设常见的约束调整模式
3. **模板化解释生成**：根据规划结果动态填充解释文本
4. **前端交互页面**：`frontend/ai-advisor.html` 提供对话式 UI

如需接入真实 LLM（如 GPT-4、Claude），只需将 `parsedIntent` 结构和 `Constraint` 对象序列化为 JSON 作为 function calling 的参数，将 `生成解释` 部分替换为 LLM 响应即可。核心管道保持不变。