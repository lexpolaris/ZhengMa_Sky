# 郑码天空（ZhengMa Sky）设计说明文档

> 本文档面向开发者，说明软件的**整体设计思路**、**主要模块与函数功能**，以及核心算法所依据的**算法公式**。
> 软件基于对原 Windows 版《郑码天空》（Borland C++ Builder，作者：上水，2006）的分析，使用 **Qt 6 + C++17** 重新实现，跨平台运行。

---

## 目录

1. [总体设计思路](#1-总体设计思路)
2. [模块结构](#2-模块结构)
3. [数据模型与文件格式](#3-数据模型与文件格式)
4. [核心算法与公式](#4-核心算法与公式)
5. [主要类与函数功能说明](#5-主要类与函数功能说明)
6. [运行流程](#6-运行流程)
7. [设计取舍与已知限制](#7-设计取舍与已知限制)

---

## 1. 总体设计思路

### 1.1 设计目标

- **忠实还原原版训练算法**：逆向分析原程序的题目调度、速度评估、单元体系，在新框架下等价实现。
- **跨平台**：用 Qt 6 原生跨平台能力替换 Win32 / VCL 依赖。
- **数据兼容**：复用原版数据文件语义（单元、题目、码表、字根字体、帮助），但改为明文 UTF-8 以便维护。
- **UI 与逻辑分离**：界面（Widget）只负责展示与输入，训练逻辑集中在 `UnitSession` / `SpeedTracker` / `QuestionPool` 三个无界面类中。

### 1.2 核心设计思想：**"末位训练"（Tail Training）**

原版最核心的教学理念：**让练习始终集中在用户最不熟练的题目上**。

- 每道题都维护一个 **速度值**（即用户的反应时间，单位 ms）。反应越慢 → 速度值越大 → 越"靠后"（末位）。
- 每一轮训练时，按速度值从慢到快排序，只抽取最慢的若干题作为本轮题目，并对其做随机洗牌（避免顺序固定）。
- 用户答对后速度值变小，答错则被打成极大错误标记，下一轮必被选中。
- 随着训练推进，"末位"的范围不断滑动，整体水平被持续向上"抬升"。

这一思想贯穿：训练模式、测试模式、单元进度判定（`Used`）均以速度表（`speedTable`）为核心状态。

### 1.3 三种学习模式

| 模式 | 枚举 | 说明 |
|------|------|------|
| 单元训练 | `LearnMode::Train` | 有效计时，末位训练算法，多轮循环 |
| 单元测试 | `LearnMode::Test` | 秒表计时，覆盖性出题，分页全屏作答，记录成绩 |
| 游戏闯关 | （预留） | 难度递进，使用 `QuestionPanel::Mode::Speed` 预留 |

---

## 2. 模块结构

### 2.1 目录结构

```
zhengma-sky/
├── CMakeLists.txt          # 构建脚本（Qt6 Widgets / Xml / Charts）
├── src/                    # 源代码
│   ├── main.cpp            # 程序入口：加载字体、设置调色板、启动主窗口
│   ├── MainWindow.*        # 主窗口：编排各面板、模式切换、数据加载/保存
│   ├── InfoPanel.*         # 左侧信息面板：等级/速度/进度/查码
│   ├── QuestionPanel.*     # 中部答题区：训练视图 + 测试视图（QStackedWidget）
│   ├── TestView.*          # 测试视图：一屏多题、颜色状态、错题重测
│   ├── StatusBar.*         # 顶部状态栏：单元名、模式、时钟
│   ├── UserData.*          # 用户数据模型：单元、题目、配置、模式状态、存档
│   ├── ZmmbTable.*         # 郑码码表：字/词 -> 编码查询
│   ├── Judge.*             # 判定：输入与编码匹配
│   ├── QuestionPool.*      # 题目池：末位排序 + Fisher-Yates 洗牌
│   ├── SpeedTracker.*      # 速度/积分/等级/连对统计
│   ├── UnitSession.*       # 单元会话：串联题目池、速度表、判定、积分
│   ├── UnitSelectDialog.*  # 选单元对话框（5 类 Tab + 单元帮助）
│   ├── SettingsDialog.*    # 参数设置对话框
│   ├── TestHistory.*       # 测试记录持久化
│   ├── TestResultDialog.*  # 测试记录/曲线展示（Qt Charts）
│   └── HelpDialog.*        # 帮助文档显示
└── data/                   # 运行时数据（随可执行文件分发）
    ├── Train.xml           # 单元/题目模板
    ├── user.xml            # 用户进度（运行时生成/更新）
    ├── zmmb.txt            # 郑码码表
    ├── zmzg.ttf            # 字根字体
    ├── help.txt            # 帮助文档（XML）
    ├── UnitHelp.txt        # 单元说明（XML）
    └── test_history.xml    # 测试记录（运行时生成）
```

### 2.2 分层视图

```
┌───────────────────────────────────────────────┐
│ 表现层 (UI)                                     │
│ MainWindow / InfoPanel / QuestionPanel /        │
│ TestView / StatusBar / 各 Dialog                │
├───────────────────────────────────────────────┤
│ 逻辑层 (Logic)                                  │
│ UnitSession ── QuestionPool                     │
│      │                                          │
│      ├── SpeedTracker                           │
│      └── Judge                                  │
├───────────────────────────────────────────────┤
│ 数据层 (Data)                                   │
│ UserData / ZmmbTable / TestHistory              │
└───────────────────────────────────────────────┘
```

- **表现层** 只与 `UnitSession`（逻辑层）交互，不直接操作 `speedTable` 等内部状态。
- **逻辑层** 无 Qt Widget 依赖（`UnitSession` 仅为 `QObject` 以发信号），便于测试与复用。
- **数据层** 负责文件的读写与内存模型。

---

## 3. 数据模型与文件格式

### 3.1 内存模型

```cpp
// 题目
struct ZbItem {
    QString charText;    // c = 字/词/字根（题目）
    QString code;        // m = 郑码编码（可能为空，需查码表）
    QString split;       // s = 拆分说明
    QString associate;   // a = 联想记忆提示
};

// 单元
struct ZbUnit {
    QString libName;     // 单元名
    int libNo;           // 单元编号（决定分类与算法分支）
    int count;           // 题目数
    int wrongCount;      // 累计答错
    int rightCount;      // 累计答对
    int used;            // 是否已完成（1/0）
    QList<ZbItem> items;
    QList<int> speedTable;  // ★ 每题速度值（ms），核心状态
    QList<int> indexArray;  // 题目池索引（兼容字段）
};

// 模式状态（Train / Test 各一份）
struct ZbModeState {
    QString libName;
    int grade, streak, wrongCount, rightCount;
    long long xp;        // 累计积分
    int totalTime, time, roundCount;
};
```

### 3.2 数据文件

| 文件 | 用途 | 格式 | 是否随程序写回 |
|------|------|------|----------------|
| `Train.xml` | 单元/题目模板（只读） | XML | 否 |
| `user.xml` | 用户进度 + 速度表 + 配置 | XML | 是 |
| `zmmb.txt` | 郑码码表 | 文本（`词<Tab>编码...`） | 否 |
| `zmzg.ttf` | 字根字体 | 字体 | 否 |
| `help.txt` | 帮助文档 | XML | 否 |
| `UnitHelp.txt` | 单元说明 | XML | 否 |
| `test_history.xml` | 测试记录 | XML | 是 |

**user.xml 结构示例**：

```xml
<ZmSoft StatusName='Train' Score='0' Date='...' DisplayType='0'
         TailTrainMax='3' HotKey='121' ShowSimilarRoot='0'
         WindowsLayout='0' AutoTailTrainCount='0' ShowHitSpeed='1'>
  <Train LibName='笔画' Grade='..' Xp='..' Streak='..' .../>
  <Test  LibName='笔画' .../>
  <Libs>
    <Lib LibName='笔画' LibNo='0' Count='24' Used='0'>
      <SpeedTable>
        <s i='0' v='320'/>   <!-- 第 0 题速度 320ms -->
        <s i='5' v='1'/>     <!-- 第 5 题速度 1ms（已熟练） -->
      </SpeedTable>
    </Lib>
  </Libs>
</ZmSoft>
```

> 速度表**只保存非零项**（压缩存储），`v=0` 表示该题尚未训练。

### 3.3 单元分类（5 类，共 44 单元）

分类逻辑见 `UnitSelectDialog::categorize()`：

| 分类 | LibNo 范围 | 说明 |
|------|-----------|------|
| 字根单元 `CatRoot`   | `0`、`2`、`10~19` | 笔画、主根、副根、全部字根 |
| 简码单元 `CatSimple` | `20~34`           | 一级/二级/三级简码及组合 |
| 词汇单元 `CatWord`   | `50~53`           | 二字/三字/四字/多字词 |
| 常用单元 `CatCommon` | `100~105`         | 常用字/词、冠军字、核心字词 |
| 提速单元 `CatSpeed`  | `200~207`         | 按基根数分类的字词 |

---

## 4. 核心算法与公式

### 4.1 答案判定（`Judge::isCorrect`）

**思路**：把输入和正确编码都用空格包裹，再做**子串包含**判断，从而支持"多个编码任选其一"和"编码可部分匹配一个完整答案"。

公式（伪代码）：

```
输入包裹 = " " + trim(用户输入) + " "
for each 编码 in 正确编码列表:
    编码包裹 = " " + trim(编码) + " "
    if 编码包裹 包含 输入包裹 (忽略大小写):
        return true
return false
```

- 例：正确编码 `"aa avai"`，参考答案列表为 `["aa", "avai"]`，输入 `"aa"` 或 `"avai"` 均判对。
- 用空格包裹可避免 `"a"` 误匹配 `"avai"`（因为 `" a "` 不会出现在 `" avai "` 中作为独立片段）。

相关常量：
- `Judge::kErrorMark = 50002`：答错写入速度表的错误标记（大于任何正常反应时间）。
- `Judge::kMaxElapsedMs = 20000`：反应时间超过 20s 视为无效，不更新速度表。

### 4.2 速度表更新（`UnitSession::updateSpeedTable`）

速度表是教学算法的核心状态，记录每题反应时间。更新规则：

```
若 elapsedMs >= 20000:           # 离开太久，忽略
    不更新
否则:
    若答对:
        # 若之前是错误标记（>=50000），先逐步减小（"复活"）
        if table[idx] >= 50000: table[idx] -= 1
        if table[idx] < 50000:
            factor = 按码长的折算系数
            table[idx] = round(elapsedMs × factor)
    否则（答错）:
        table[idx] = 50002          # 错误标记，下轮必被选中
```

**按码长折算系数**（对应原版 `sub_0040D3A6~D3ED`，短码反应时间应相对"更值钱"）：

| 编码长度 | 折算系数 `factor` |
|:--------:|:-----------------:|
| 2 码 | 0.7 |
| 3 码 | 0.5 |
| 4 码 | 0.3 |
| 其他 | 1.0 |

公式：

$$
\text{table}[idx] = \big\lceil \text{elapsedMs} \times factor \big\rfloor
\quad\text{（四舍五入取整）}
$$

### 4.3 末位排序 + 洗牌（`QuestionPool::buildIndexArray`）

这是**末位训练**的算法主体。设题目总数 $N$，本轮题数 $n = \min(N, roundCount)$。

**步骤 1：索引初始化**
$$
\text{index}[i] = i,\quad i = 0,1,\dots,N-1
$$

**步骤 2：按速度降序（慢→快）局部排序**
只排前 $n$ 个位置。实现采用冒泡：对 $k = 0 \dots n-1$，从尾部向 $k$ 冒泡，若 `speed[index[m]] > speed[index[m-1]]` 则交换。效果是每轮把当前最慢的题目推到最前。等价于：

$$
\text{index}[0..n-1] = \underset{\text{按 speedTable 降序}}{\text{partial\_sort}}(N, n)
$$

**步骤 3：Fisher-Yates 洗牌（仅前 $n$ 个）**
对 $i = 0 \dots n-1$，随机选取交换目标：

$$
j = (i + \text{random}(0,\  roundCount - i)) \bmod N
$$

然后交换 `index[i]` 与 `index[j]`。

> 原版随机式：`v57 = (n + Random(401512 - n)) % 401476`，此处的 `% N` 取其同义。

**出题**：`next()` 依次返回 `index[0], index[1], ...`。`isRoundEnd()` 在 `currentIndex >= roundCount - 1` 时为 true。

### 4.4 每轮题数（末位训练题数）

**训练模式**（对应原版 `sub_411B58`，见 `UnitSession::start`）：

$$
\text{roundCount} =
\begin{cases}
\text{count}, & \text{libNo} \ge 200 \text{ 且 } count < 15\\
\lfloor count/2 \rfloor, & \text{libNo}<200,\ count < 100\\
\lfloor count/3 \rfloor, & \text{libNo}<200,\ 100 \le count < 300\\
\lfloor count/4 \rfloor, & \text{libNo}<200,\ 300 \le count < 500\\
200, & \text{libNo}<200,\ count \ge 500\\
50, & \text{libNo} \ge 200
\end{cases}
$$

即：

| 条件（libNo < 200） | roundCount |
|---------------------|-----------|
| count < 15 | count |
| 15 ≤ count < 100 | count / 2 |
| 100 ≤ count < 200 | count / 3 |
| 200 ≤ count < 300 | count / 3 |
| 300 ≤ count < 500 | count / 4 |
| count ≥ 500 | 200 |
| **libNo ≥ 200（提速单元）** | **50** |

**测试模式**（`UnitSession::calcTailTestItems`）：

$$
\text{roundCount} =
\begin{cases}
2 \times count, & count < 50\\
count, & 50 \le count < 400\\
300, & count \ge 400
\end{cases}
$$

用户可在参数设置中**覆盖**测试题数（`m_testItemsOverride >= 0` 时直接采用）。

### 4.5 速度计算（`SpeedTracker::updateSpeed`）

**平均速度（字/分钟）**：

$$
\text{currentSpeed} = \left\lfloor \frac{60000 \times \text{charCount}}{\text{totalMs}} \right\rfloor
$$

**最近速度（最近 20 题窗口，手感更稳定）**（`recentSpeed`）：

$$
\text{recentSpeed} = \left\lfloor \frac{60000 \times \text{winKeys}}{\text{winMs}} \right\rfloor
$$

窗口满 `kRecentWindow = 20` 题或累计达 `kIdleThresholdMs × 4 = 20000ms` 时结算并清零。

**击键速度（键/秒）**：

$$
\text{hitSpeed} = \frac{1000 \times \text{keyStrokes}}{\text{totalMs}}
$$

**最高速度**：`bestSpeed = max(bestSpeed, currentSpeed)`。

### 4.6 有效计时 vs 秒表计时

- **训练（有效计时，`m_effectiveTiming = true`）**：两次输入间隔超过 `kIdleThresholdMs = 5000ms` 时，仅按 5000ms 计入（离开不计时）：

$$
\text{countedMs} = \min(\text{elapsedMs},\ 5000)
$$

- **测试（秒表计时，`m_effectiveTiming = false`）**：无条件累计：

$$
\text{countedMs} = \text{elapsedMs}
$$

### 4.7 等级与积分（`SpeedTracker`）

- 每题答对获得积分（原版 `[self+62074]`，见 `UnitSession::scoreForItem`）：

$$
\text{points} = \begin{cases} 3, & \text{libNo} < 20\\ 1, & \text{libNo} \ge 20 \end{cases}
$$

- 累计积分**只增不减**，等级由积分派生：

$$
\text{grade} = \left\lfloor \frac{\text{score}}{kScorePerLevel} \right\rfloor + 1,
\quad kScorePerLevel = 10
$$

- 当前等级内进度：

$$
\text{xpAtThisLevel} = \text{score} \bmod 10,\qquad
\text{xpForNextLevel} = \text{grade} \times 10
$$

### 4.8 测试成绩（`TestRecord::score` / `SpeedTracker::score`）

$$
\text{score} = \text{speed} - \text{wrongCount}
$$

即"平均速度减去错误数"，可为负，最高等于平均速度。

### 4.9 单元完成判定（`UserData::isUnitCompleted`）

设已训练题数（速度表中非零项个数）为 $T$，题目总数为 $N$：

$$
\text{Used} = 1 \iff T \ge N
$$

### 4.10 测试速度（分页测试，`MainWindow::onTestFinished`）

以完整测试题目数为分母、秒表总时间为分母：

$$
\text{speed} = \left\lfloor \frac{60000 \times \text{itemTotal}}{\text{testElapsedMs}} \right\rfloor
$$

---

## 5. 主要类与函数功能说明

### 5.1 `Judge`（判定）

| 函数 | 功能 |
|------|------|
| `static bool isCorrect(input, codes)` | 输入是否匹配编码列表中的任意一个（空格包裹 + 包含判定，忽略大小写） |
| `static bool isCorrect(input, code)` | 单编码的便捷重载 |
| `kErrorMark = 50002` | 错题标记值 |
| `kMaxElapsedMs = 20000` | 超时忽略阈值 |

### 5.2 `QuestionPool`（题目池）

负责"末位排序 + 洗牌 + 顺序出题"。

| 函数 | 功能 |
|------|------|
| `init(totalItems, roundCount, speedTable)` | 初始化题目池，设置速度表并构建索引数组 |
| `buildIndexArray()` | ★ 核心：局部按速度降序排列 + Fisher-Yates 洗牌 |
| `next()` | 取下一题，返回题目在单元中的索引 |
| `isRoundEnd()` | 本轮是否结束 |
| `reshuffle()` | 重新排序 + 洗牌（速度表变化后调用） |
| `reset()` | 将当前指针归零 |
| `setSpeedTable(speeds)` | 更新速度表 |
| `currentQuestionIndex()` | 当前题目的原始索引 |

### 5.3 `SpeedTracker`（速度/积分统计）

| 函数 | 功能 |
|------|------|
| `startSession(keepProgress)` | 开始一次训练；`keepProgress=false` 时清零积分/连对 |
| `setEffectiveTiming(bool)` | 切换有效计时（训练）/ 秒表计时（测试） |
| `recordKey(correct, itemCount, elapsedMs, keyStrokes)` | 记录一次答题：累计时间、字词数、击键、连对、窗口统计，并调用 `updateSpeed()` |
| `updateSpeed()` | 计算 `currentSpeed` 与 `hitSpeed`，更新 `bestSpeed` |
| `currentSpeed()` / `recentSpeed()` / `bestSpeed()` | 各类速度 |
| `grade()` / `scoreTotal()` / `xp()` | 等级与积分 |
| `addScore(points)` | 累加积分 |
| `setXp()` / `setStreak()` | 跨会话恢复进度 |
| `accuracy()` | 正确率（0–100） |
| `score()` | 测试成绩 = 平均速度 − 错误数 |
| `formatDuration(ms)` | 毫秒格式化为 `HH:MM:SS` |

### 5.4 `UnitSession`（单元会话，逻辑层核心）

串联题目池、速度表、判定与积分，是 UI 与算法之间的门面。

| 函数 | 功能 |
|------|------|
| `start(unit, zmmb, trainMax)` | 开始单元：计算每轮题数、初始化速度表与题目池 |
| `currentItem()` | 当前题目 |
| `submit(input)` | 提交答案：判定 → 更新时间/速度表/积分 → 刷新 `Used` |
| `updateSpeedTable(idx, correct, elapsed, codeLen)` | ★ 按 4.2 规则更新速度表 |
| `getCorrectCodes(item)` | 取题目编码（item.code 为空则查码表） |
| `isRoundEnd()` / `isAllRoundsEnd()` | 本轮/全部轮是否结束 |
| `nextRound()` | 进入下一轮（重排 + 洗牌） |
| `next()` | 取下一题 |
| `enterTestMode()` | 进入测试模式（切换秒表计时） |
| `getTestItemsWithIndex()` | 取本测试全部题目（文本、编码、原始索引） |
| `reportTestAnswer(idx, correct)` | 测试答对/答错回写速度表（答错加重、答对清错标） |
| `calcTailTestItems(count)` | ★ 测试题数公式（见 4.4） |
| `scoreForItem(libNo)` | ★ 每题积分（见 4.7） |

### 5.5 `UserData`（数据模型）

| 函数 | 功能 |
|------|------|
| `loadTemplate(path)` | 加载 `Train.xml`（单元与题目，只读） |
| `loadUserData(path)` | 加载 `user.xml`（配置、模式状态、单元进度与速度表） |
| `saveUserData(path)` | 保存 `user.xml`（速度表压缩存储，只写非零项） |
| `findUnit(libNo)` | 按编号查找单元 |
| `isUnitCompleted(unit)` | ★ 单元完成判定（见 4.9） |
| `trainState()` / `testState()` | 取模式状态 |

### 5.6 `ZmmbTable`（码表）

| 函数 | 功能 |
|------|------|
| `load(path)` | 逐行解析 `zmmb.txt`（`词<Tab>编码...`），构建 `QHash<词, 编码列表>` |
| `lookup(word)` | 查询某字的全部编码 |
| `contains(word)` | 是否存在 |

### 5.7 `TestHistory`（测试记录）

| 函数 | 功能 |
|------|------|
| `load(path)` / `save(path)` | 读写 `test_history.xml` |
| `addRecord(rec)` | 新增记录（前插），每个单元仅保留最近 `kMaxRecords = 10` 条 |
| `recordsByUnit(libNo)` | 取某单元的历史记录 |

### 5.8 `MainWindow`（主窗口）

| 函数 | 功能 |
|------|------|
| `loadData()` | 加载模板 → 用户数据 → 码表 → 测试记录 |
| `loadUnit(libNo)` | 载入单元、启动会话、恢复跨会话进度、刷新面板 |
| `onInputSubmitted(text)` | 训练模式提交处理：判对进入下一题/下一轮/结束；判错提示正确编码 |
| `onTrainRoundsFinished()` | 训练结束弹窗（测试 / 下一单元 / 继续 / 关闭） |
| `enterTestMode()` / `loadTestPage()` | 测试模式：分页加载题目 |
| `onTestItemAnswered()` / `onTestPageFinished()` / `onTestFinished()` | 测试答题/翻页/结算 |
| `applyLearnMode(mode)` / `switchToNextMode()` | 模式切换与状态栏刷新 |
| `syncProgressToData()` / `restoreProgressFromData()` | 会话积分/连对与存档互转 |
| `saveUserData()` / `saveTestHistory()` | 持久化（关闭时也在 `closeEvent` 保存） |

### 5.9 `TestView`（测试视图）

| 函数 | 功能 |
|------|------|
| `loadItems(items, indices)` | 加载本页题目 |
| `submitInput(input)` | 判定并推进；答错累计错误、连错 3 次显示提示 |
| `startRetest()` | ★ 收集本页错题，进入"错题重测"循环，直到全部通过 |
| `pageCapacity()` | 根据字体行高与视口高度计算本页可容纳题数（每行 15 字） |
| `elapsedMs()` / `keyStrokes()` | 本页耗时/击键数，用于成绩统计 |

### 5.10 辅助对话框

| 类 | 功能 |
|----|------|
| `UnitSelectDialog` | 5 类 Tab 选单元，右侧显示单元说明（读 `UnitHelp.txt`）；`categorize()` 决定分类 |
| `SettingsDialog` | 显示方式、末位训练数量/次数、测试题数等参数 |
| `TestResultDialog` | 展示测试记录与速度/成绩曲线（Qt Charts） |
| `HelpDialog` | 显示 `help.txt` |

---

## 6. 运行流程

### 6.1 启动

```
main()
 ├─ 设置应用名 / Fusion 风格 / QPalette 配色
 ├─ 加载字根字体 data/zmzg.ttf
 └─ 创建 MainWindow
      ├─ setupUi()                 构建三区布局（状态栏 / 信息面板 / 答题区）
      ├─ loadData()                train.xml → user.xml → zmmb.txt → test_history.xml
      ├─ 恢复上次单元/模式（已完成则跳下一个未完成单元）
      └─ loadUnit()                UnitSession::start()（计算每轮题数、初始化题目池）
```

### 6.2 训练模式一轮

```
出题 = pool.next()  →  用户输入  →  Enter/空格提交
   └─ UnitSession::submit()
        ├─ Judge::isCorrect()                       判定
        ├─ updateSpeedTable()                       更新速度表
        ├─ SpeedTracker::recordKey()                统计速度/时间/击键
        └─ addScore(scoreForItem())                 累加积分（答对）
   └─ 若本轮结束:
        ├─ 还有轮次 → nextRound()（重排 + 洗牌）
        └─ 全部结束 → onTrainRoundsFinished()（弹窗：测试/下一单元/继续/关闭）
```

### 6.3 测试模式

```
enterTestMode()
 ├─ session.enterTestMode()          切秒表计时
 ├─ getTestItemsWithIndex()          取末位排序后的测试题
 ├─ 按 pageCapacity() 分页
 └─ 每页 TestView::loadItems()
      ├─ 答对 → 绿色
      ├─ 答错 → 红色 + reportTestAnswer() 回写速度表（加重）
      └─ 一页跑完 → 错题重测，直到全通过 → finished()
 └─ 全部页完成 → onTestFinished()     计算成绩、写历史、回训练模式
```

---

## 7. 设计取舍与已知限制

### 7.1 与原版的差异

| 项目 | 原版 | 本项目 |
|------|------|--------|
| 平台 | Windows only | Qt 跨平台 |
| 框架 | Borland VCL | Qt 6 Widgets |
| 数据 | 加密（LCG + XOR） | 明文 UTF-8 |
| 图表 | TeeChart | Qt Charts |
| 帮助系统 | RichEdit | QTextEdit |
| 字体渲染 | GDI | Qt 原生 |

### 7.2 设计取舍

- **逻辑层与 UI 解耦**：`UnitSession` 只发信号、暴露纯数据接口，方便未来替换 UI 或加入单元测试。
- **速度表压缩存储**：`user.xml` 仅存非零项，减少文件体积（大型单元可达数百题）。
- **积分只增不减 + 等级由积分派生**：避免"换单元/重开清空等级"，等级不会倒退。
- **有效计时保护**：训练时离开（>5s）不计时，防止挂机刷速度。

### 7.3 已知限制

- **热键注册**：全局热键仅在 Windows 上生效，Linux/macOS 需额外配置。
- **帮助系统**：主帮助仅加载 `help.txt`，单元帮助在选单元对话框内加载 `UnitHelp.txt`。
- **多用户支持**：原版支持 `user0/user1/...`，本项目暂未实现。
- **数据持久化**：训练进度（速度表）会写回 `user.xml`；测试记录写回 `test_history.xml`。

---

## 附录：常量与阈值速查

| 常量 | 值 | 含义 |
|------|----|------|
| `Judge::kErrorMark` | 50002 | 错题标记 |
| `Judge::kMaxElapsedMs` | 20000 | 反应超时忽略阈值（ms） |
| `SpeedTracker::kIdleThresholdMs` | 5000 | 有效计时离开阈值（ms） |
| `SpeedTracker::kRecentWindow` | 20 | 最近速度窗口题数 |
| `SpeedTracker::kScorePerLevel` | 10 | 每级所需积分 |
| `TestHistory::kMaxRecords` | 10 | 每单元保留测试记录数 |
| `TestView::kWrongStreakForHint` | 3 | 连错显示提示的阈值 |
| `TestView::kCharsPerLine` | 15 | 测试每行显示字数 |
| `MainWindow::m_testPageSize` | 100（默认） | 测试分页大小（运行时按视口计算） |

---

*本文档基于当前源码整理，如实现变更请同步更新。*
