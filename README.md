# SnakeGame

一个用 **C 语言 + raylib** 实现的单机版霓虹风贪吃蛇小游戏。蛇身使用**带哨兵头结点的双向循环链表**表示，逻辑与渲染严格分层，核心逻辑可脱离图形库用 googletest 单元测试。

## 玩法规则

- 棋盘 64 × 48 格（细密网格，格子 15px）；**蛇身每节占 2×2 格**，食物与道具占 1 格。
- 蛇每步移动 1 格，渲染带像素插值动画；由于格子很细、每步位移小，视觉上几乎看不出“跳格”，移动更丝滑。
- 撞墙、撞内部障碍会死亡；**撞到自己身体不会死亡**，可以覆盖自己、也可以直接反向往回走。
- 吃到普通食物：长度 +1、分数 +10。
- **视觉覆盖即拾取**：蛇头在视觉上是 2×2 方块，只要覆盖到食物/道具所在格（3×3 邻域）就会吃掉。
- 吃到道具后，**蛇身会永久变成对应道具的颜色**（加速=黄、减速=蓝、护盾=白、缩短=紫红），直到再次吃到其它道具；重开时恢复默认色。
- 每吃 8 个食物升 1 级；每升 1 级步进加快 4ms（起始 75ms/格，下限 35ms）。
- 每升 3 级增加 1 个障碍，最多 15 个。
- 道具（同时最多 2 个，每 6~10 秒刷新一个，存活 8 秒）：
  - **加速**（黄色三角）：5 秒内移动更快。
  - **减速**（蓝色圆环）：5 秒内移动更慢。
  - **缩短**（紫红箭头）：立即减少 3 节身体（最短保留 3 节）。
  - **护盾**（白色六边形）：5 秒内免疫一次死亡，触发后消耗。
  - 重复吃到同类效果会**刷新持续时间**，不会叠加。
- 无尽模式，无胜利条件；最高分会保存到本地文件 `highscore.dat`。

## 操作

| 按键 | 功能 |
|---|---|
| 方向键 / WASD | 移动（允许反向；支持输入缓冲，快速连转不丢键） |
| P | 暂停 / 继续 |
| R | 重新开始 |
| Esc | 退出 |
| F11 | 切换全屏 |

游戏结束时可用鼠标点击 **RESTART** 按钮重来，或点击 **QUIT** 退出。

## 构建与运行

需要 CMake ≥ 3.20 与 Visual Studio（或其它 C/C++ 工具链）。依赖（raylib、googletest）由 CMake FetchContent 自动下载，无需手动安装。

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Debug
.\build\bin\Debug\SnakeGame.exe
```

关闭测试可加 `-DENABLE_TESTS=OFF`。

## 运行测试

```powershell
.\build\bin\Debug\snake_tests.exe
```

或用 ctest：

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

## 目录结构

```
SnakeGame/
├─ CMakeLists.txt        # 构建脚本（FetchContent 拉取 raylib / googletest）
├─ project.md            # 需求、架构、分阶段 TODO 与验收标准
├─ src/
│  ├─ core/              # 纯 C 逻辑层（零 raylib 依赖，可独立测试）
│  │  ├─ types.h         # 公共数据类型（Cell / Direction / ItemType）
│  │  ├─ config.h        # 棋盘与玩法常量
│  │  ├─ list.[ch]       # 双向循环链表（哨兵）ADT
│  │  ├─ rng.[ch]        # 可注入种子的 PRNG（xorshift32）
│  │  ├─ game.[ch]       # 游戏状态、移动、碰撞、难度
│  │  ├─ items.[ch]      # 道具系统与效果
│  │  └─ persist.[ch]    # 最高分存档
│  ├─ ui/                # raylib 渲染/输入/音效层
│  │  ├─ theme.[ch]      # 布局常量与霓虹配色
│  │  ├─ render.[ch]     # 绘制蛇/食物/道具/障碍/HUD/覆盖层
│  │  ├─ input.[ch]      # 按键到抽象动作的映射
│  │  └─ audio.[ch]      # 代码合成音效
│  └─ main.c             # 主循环（固定步长 + 插值 + 虚拟分辨率缩放）
└─ tests/                # googletest 单元测试（C++）
```

## 技术要点

- **双向循环链表 + 哨兵**：蛇头为 `sentinel->next`，蛇尾为 `sentinel->prev`；每步“头插新蛇头 + 删蛇尾”，吃到食物时只头插不删尾，长度自然 +1。
- **逻辑与渲染分层**：`src/core` 不含任何 raylib 头文件，可被 C++ 测试通过 `extern "C"` 直接链接。
- **可复现随机**：所有随机行为（食物/道具/障碍位置）都经由可注入种子的 xorshift32，测试可固定种子复现。
- **固定步长 + 插值**：逻辑按 `speed_ms` 步进，渲染在相邻逻辑位置间线性插值。
- **虚拟分辨率**：按 1120×840 绘制到 RenderTexture，再等比缩放到窗口，支持窗口放大与全屏且布局不变。
