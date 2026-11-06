# ShadcnWidgetTools

![Qt](https://img.shields.io/badge/Qt-6.7%2B-41CD52) ![C++](https://img.shields.io/badge/C%2B%2B-17-00599C) ![Windows](https://img.shields.io/badge/Windows-x64-61C263) ![macOS](https://img.shields.io/badge/macOS-supported-61C263) ![Linux](https://img.shields.io/badge/Linux-untested-lightgrey) ![License](https://img.shields.io/badge/License-MIT-blue) [![QQ群](https://img.shields.io/badge/QQ%E7%BE%A4-1128537765-12B7F5?logo=qq&logoColor=white)](https://qm.qq.com/q/wJMTPn2Qow)

基于 Qt 6 Widgets 的 [shadcn/ui](https://ui.shadcn.com) 风格组件库。组件全部自绘，不依赖 QSS，外观与交互尽量与 shadcn/ui v4 保持一致，内置 [Lucide](https://lucide.dev) 图标字体。

交流 QQ 群：1128537765，点击链接加入群聊 [ShadcnWidgetTools](https://qm.qq.com/q/wJMTPn2Qow)

## 特性

- **主题**：shadcn 颜色 token，亮色 / 暗色与跟随系统，五种基色，圆角可调，切换后所有组件立即重绘。
- **窗口特效**：壁纸模糊（所有平台）、背景模糊（Windows、macOS）、亚克力与云母（Windows 11）；底色与组件的不透明度、壁纸模糊程度可调，macOS 的背景模糊程度也可调，默认仅模糊桌面（实时捕获桌面层，需要录屏权限，可关闭）。
- **图标**：内置 Lucide 图标字体（2191 个），通过 `ShadIconType` 枚举使用。
- **Emoji**：Windows 与 Linux 上内置 Twemoji 彩色 emoji 字体。
- **交互**：hover 过渡动画，仅键盘导航时显示焦点环，完整的键盘操作。
- **动画**：标签与页面切换带过渡动画，开关、时长与风格由 `ShadTheme` 统一配置；侧栏指示条、手风琴等动画时长可调。
- **RTL**：跟随 Qt 的 `layoutDirection`，从右到左界面中绘制与交互全部镜像。
- **多语言**：内置文字原文为简体中文，附带英文、繁体中文、日语与韩语翻译；`ShadI18n` 切换语言后组件文字即时刷新，也可注册应用自己的翻译。
- **GPU 图表**：面积图、折线图、柱状图、散点图与热力图通过 QRhi 由 GPU 绘制，可选绘制 API，支持数十万个实时数据点，GPU 不可用时改用 CPU。
- **构建**：CMake，动态库 / 静态库，可通过 `FetchContent` 直接引入源码，也可 install 后通过 `find_package` 引用。

## 组件

共 100+ 个组件。除主题、语言与图标外，分类与示例程序的导航一致。

### 主题、语言与图标

| 类 | 名称 | 要点 |
|---|---|---|
| ShadTheme | 主题管理器 | 亮色 / 暗色 / 跟随系统，五种基色，圆角、页面切换动画与窗口特效配置 |
| ShadI18n | 语言管理器 | 切换界面语言，组件内置文字即时刷新；`addTranslations` 注册应用自己的翻译 |
| ShadIcon | 图标 | 2191 个 Lucide 图标，通过 `ShadIconType` 使用 |

### 按钮与操作

| 类 | 名称 | 要点 |
|---|---|---|
| ShadButton | 按钮 | Default / Destructive / Outline / Secondary / Ghost / Link 六种变体，多种尺寸，支持图标与加载状态 |
| ShadButtonGroup | 按钮组 | 相邻按钮拼接，可插入文字块 |
| ShadToggle | 切换按钮 | 双态，选中时为 accent 背景 |
| ShadToggleGroup | 切换按钮组 | 单选 / 多选，按钮横向拼接 |
| ShadSegmented | 分段控制器 | 选中块滑动动画，图标与文字，三种尺寸，可撑满宽度 |
| ShadDropdownMenu | 下拉菜单 | 图标、快捷键提示、复选项、单选项、子菜单、分组标题、危险操作样式 |
| ShadMenubar | 菜单栏 | 悬停切换菜单，方向键在菜单间切换，macOS 上可显示在系统菜单栏 |
| ShadToolbar | 工具栏 | 按钮、勾选、下拉菜单与任意组件，横向或纵向，放不下的收进更多菜单 |
| ShadStatusBar | 状态栏 | 开始侧状态与临时消息，结束侧常驻组件，自动分隔线 |
| ShadContextMenu | 右键菜单 | 右键或菜单键在鼠标位置弹出，菜单项与下拉菜单相同 |
| ShadCommand | 命令面板 | 模糊搜索、分组、快捷键提示 |
| ShadCommandDialog | 命令面板对话框 | 程序内快捷键打开，点击外部关闭 |
| ShadKbd | 按键 | 键盘按键标识 |
| ShadKeySequenceEdit | 快捷键录制 | 实时显示修饰键，Esc 取消，可清除 |

### 数据录入

| 类 | 名称 | 要点 |
|---|---|---|
| ShadInput | 输入框 | 前置图标，清除与密码显隐按钮，错误态 |
| ShadInputGroup | 输入组 | 输入框前后附加文字、图标、按钮 |
| ShadTextarea | 多行输入 | 随内容自动增高，可设上限 |
| ShadCodeEditor | 代码编辑器 | 行号、语法高亮、当前行高亮 |
| ShadInputOtp | 验证码输入 | 分格输入，粘贴自动分发，键盘导航 |
| ShadPasskey / ShadPasskeyButton | 通行密钥 | 系统通行密钥注册与登录，模拟模式 |
| ShadBiometric | 身份验证 | Touch ID / Windows Hello 确认本人，可改用密码或 PIN |
| ShadSpinBox | 整数输入 | 聚焦后滚轮调节 |
| ShadDoubleSpinBox | 小数输入 | 聚焦后滚轮调节 |
| ShadComboBox | 下拉选择 | 单选 / 多选，搜索过滤，保留 QComboBox 接口 |
| ShadTreeSelect | 树形选择 | 单选 / 多选，父子勾选联动，搜索过滤，完整路径显示 |
| ShadCheckbox | 复选框 | 三态 |
| ShadSwitch | 开关 | 可带文字 |
| ShadRadioGroup | 单选组 | 布局与互斥选择 |
| ShadRadioButton | 单选项 | 由单选组创建和管理 |
| ShadSlider | 滑块 | 水平 / 垂直，范围模式 |
| ShadRating | 评分 | 支持只读 |
| ShadColorPicker | 颜色选择器 | HSV 选取，基本与自定义颜色，屏幕取色 |
| ShadCalendar | 日历 | 键盘导航，跟随 locale |
| ShadDatePicker | 日期选择 | 单日与范围模式 |
| ShadTimePicker | 时间选择 | 时、分、秒滚动列 |
| ShadField | 表单项 | 标签、说明、错误态，选择卡片样式 |
| ShadFieldGroup | 表单项组 | 纵向排列表单项，可插入分隔线 |
| ShadFieldSet | 表单分组 | 标题、说明与一组表单项 |
| ShadFieldSeparator | 表单分隔线 | 可带居中文字 |
| ShadLabel | 标签 | 主题颜色角色，关联控件 |
| ShadFileDropZone | 文件上传区 | 拖放或点击选择，类型与大小校验 |
| ShadAttachment | 附件 | 图标或图片预览，操作按钮 |
| ShadImageCropper | 图片裁剪 | 自由或固定宽高比，圆形裁剪，旋转与翻转，按原图分辨率输出 |
| ShadSignaturePad | 签名板 | 平滑笔迹，线宽随速度或压感变化，撤销，导出透明背景图片 |
| ShadAttachmentGroup | 附件组 | 横向排列，超出宽度时横向滚动 |
| ShadQuestionnaire | 问卷 | 一次一题，单选 / 多选 / 自由输入 / 跳过，校验与快捷键 |

### 数据展示

| 类 | 名称 | 要点 |
|---|---|---|
| ShadCard | 卡片 | 标题、描述、操作、内容与页脚 |
| ShadAccordion | 手风琴 | 单项 / 多项展开，高度动画，带边框样式 |
| ShadCollapsible | 折叠面板 | 内容区展开收起，可放任意组件 |
| ShadExpander | 抽屉面板 | 标题行展开分隔排列的内容行 |
| ShadItem | 列表项 | 可点击，多种尺寸 |
| ShadItemGroup | 列表项组 | 纵向排列，可插入分隔线 |
| ShadListView | 虚拟列表 | 基于 QListView 只绘制可见行，三种尺寸，复选框，空状态，滚动加载更多 |
| ShadCarousel | 轮播 | 横向 / 纵向，拖动与键盘切换 |
| ShadImageViewer | 图片预览 | 缩放、旋转、翻转，多图切换，复制与保存 |
| ShadAvatar | 头像 | 图片失败时显示文字或图标 |
| ShadAvatarGroup | 头像组 | 头像重叠排列，末尾可显示计数 |
| ShadBadge | 徽章 | Default / Secondary / Destructive / Outline |
| ShadTable | 表格 | 自动高度，图片列 |
| ShadDataTable | 数据表格 | 筛选、排序、列显示、行选择、分页 |
| ShadDescriptions | 描述列表 | 键值对按列网格排列，跨列，带边框与纵向排列，内容可为任意组件 |
| ShadPropertyGrid | 属性面板 | 分组折叠，按类型自动选择编辑器，可自定义 |
| ShadTreeView | 树形视图 | 勾选联动，Lucide 图标 |
| ShadAreaChart | 面积图 | GPU 绘制，可堆叠 |
| ShadLineChart | 折线图 | GPU 绘制 |
| ShadBarChart | 柱状图 | GPU 绘制，可堆叠 |
| ShadScatterChart | 散点图 | GPU 绘制，增量追加数据点 |
| ShadHeatmap | 热力图 | GPU 绘制，支持瀑布图 |
| ShadStatistic | 统计数值 | 数值滚动动画，前后缀，涨跌趋势，倒计时 |
| ShadCodeBlock | 代码块 | 标题栏、复制按钮、行号与增删标记 |
| ShadMarkdownView | Markdown | GitHub 风格 Markdown，代码块高亮与复制，流式追加 |
| ShadTrace | 执行记录 | 图标、标题与耗时，可点击 |
| ShadMarker | 标记 | 对话中的状态与提示，行内、带边框与分隔线三种变体，扫光，可作为链接或按钮 |
| ShadQrCode | 二维码 | 纠错等级可选 |
| ShadWatermark | 水印 | 文字或图片，交错平铺，角度与间距可调 |

### 反馈

| 类 | 名称 | 要点 |
|---|---|---|
| ShadAlert | 提示 | Default / Destructive，图标与操作按钮 |
| ShadEmpty | 空状态 | 图标或自定义媒体、标题、描述与操作区，可选虚线边框 |
| ShadProgress | 进度条 | 数值过渡动画 |
| ShadRadialProgress | 环形进度 | 整圆或仪表盘，四种状态，数值过渡动画 |
| ShadSpinner | 加载指示 | 匀速旋转 |
| ShadToaster | 通知 | 六种类型、六个位置，悬停展开堆叠 |
| ShadNotification | 消息通知 | 四种类型、八个位置，倒计时自动关闭 |

### 浮层与窗口

| 类 | 名称 | 要点 |
|---|---|---|
| ShadPopover | 浮层 | 锚定显示，点击外部或 Esc 关闭 |
| ShadPopconfirm | 气泡确认 | 在按钮旁弹出确认与取消，可选危险样式与图标 |
| ShadTooltip | 悬停提示 | 接管全部 toolTip，带箭头 |
| ShadHoverCard | 悬停卡片 | 悬停延迟显示，可放任意组件，不抢占焦点 |
| ShadDialog | 对话框 | 模态遮罩，标题栏按钮可配置 |
| ShadAlertDialog | 警告对话框 | 确认 / 取消，可带媒体区 |
| ShadInputDialog | 输入对话框 | 文本 / 多行 / 整数 / 小数，提供静态函数 |
| ShadColorDialog | 颜色对话框 | 预览并恢复原始颜色 |
| ShadDrawer | 抽屉 | 四个方向，拖动关闭，嵌套叠放与吸附点 |
| ShadWindow | 窗口 | 无边框，系统贴靠与阴影，macOS 上窗口按钮可放在左侧 |
| ShadWidget | 独立窗口 | 与 ShadWindow 相同的标题栏，可直接 setLayout |
| ShadScreenshot | 截图 | 框选或单击窗口，标注，复制、保存或钉在屏幕上，全局快捷键 |

### 导航与布局

| 类 | 名称 | 要点 |
|---|---|---|
| ShadNavigationMenu | 导航菜单 | 悬停或点击显示内容，切换时内容滑入 |
| ShadNavigationMenuLink | 导航链接 | 标题与两行描述，点击后关闭菜单 |
| ShadBreadcrumb | 面包屑 | 可折叠中间项，宽度不够时换行 |
| ShadPagination | 分页 | 页码跳转 |
| ShadTabs | 标签页 | 横向 / 纵向，Line 变体 |
| ShadTabView | 标签视图 | 关闭、新建、拖动排序，拆分与合并窗口 |
| ShadStackedWidget | 页面栈 | 切换动画跟随 ShadTheme |
| ShadDockManager | 停靠布局 | 分组拆分、停靠指引、浮动窗口、保存与恢复布局 |
| ShadDockPanel | 停靠面板 | 按 id 保存与恢复布局 |
| ShadSidebar | 侧栏 | Sidebar / Floating / Inset 三种样式，收起为图标，可拖动宽度 |
| ShadSidebarLayout | 侧栏布局 | 侧栏与主内容区，Ctrl+B 切换 |
| ShadSidebarGroup | 侧栏分组 | 标题与菜单，可折叠 |
| ShadSidebarMenuButton | 侧栏菜单按钮 | 图标、文字、徽章与子菜单 |
| ShadSidebarTrigger | 侧栏切换按钮 | 展开或收起侧栏 |
| ShadSeparator | 分隔线 | 1px，Border 颜色 |
| ShadResizable | 可调整面板 | 拖动分隔线调整大小 |
| ShadScrollArea | 滚动区域 | 悬停显示的细滚动条 |
| ShadScrollBar | 滚动条 | 细滚动条，可替换原生滚动条 |
| ShadFlowLayout | 流式布局 | 自动换行 |

## 示例程序

`ShadcnWidgetToolsExample` 左侧按上表分类导航，每个组件一个演示页；用户菜单中的设置可切换主题、基色、圆角、窗口特效、动画与 RTL。

## 环境要求

| 项目 | 要求 |
|---|---|
| Qt | 建议 6.7 及以上；不使用 GPU 时最低 6.5；翻译需要 Qt Linguist Tools，没有时只有简体中文 |
| 编译器 | MSVC、AppleClang、GCC、Clang |
| CMake | 3.21 及以上 |
| C++ 标准 | C++17 |
| 平台 | Windows x64、macOS、WebAssembly；Linux 未测试 |

## 构建

工程自带 `CMakePresets.json`（Ninja 生成器）：Windows 为 `msvc-debug` / `msvc-release`（在 Visual Studio x64 开发者命令行中执行），macOS 为 `macos-*`，Linux 为 `linux-gcc-*` / `linux-clang-*`，WebAssembly 为 `wasm-release`（见下文）。

```bash
cmake --preset msvc-release
```

```bash
cmake --build --preset msvc-release
```

产物位于 `build/<preset>/bin` 与 `build/<preset>/lib`。工程不写死 Qt 路径，通过 `-DQT_SDK_DIR`、环境变量 `QT_SDK_DIR` 或 `CMAKE_PREFIX_PATH` 指定 Qt 安装目录（例如 `~/Qt/6.12.0/macos`）；都未指定时使用系统中的 Qt。

| 选项 | 默认值 | 说明 |
|---|---|---|
| `SHADCN_BUILD_STATIC` | `OFF` | 编译为静态库 |
| `SHADCN_CHART_GPU` | `ON` | 图表使用 GPU 绘制，依赖不满足时只编译 CPU 绘制 |
| `SHADCN_BUILD_EXAMPLE` | 顶层工程 `ON`，子工程 `OFF` | 编译示例程序 |
| `SHADCN_INSTALL` | 顶层工程 `ON`，子工程 `OFF` | 安装头文件、库与 CMake 包配置；关闭时只安装动态库 |
| `SHADCN_EXAMPLE_AI_CHAT` | `ON` | 示例程序编译 AI 对话页面，需要 Qt Network |

编译后不复制 Qt 运行库：Windows 上需要把 Qt 的 `bin` 目录加入 `PATH` 再运行示例；执行 `cmake --install build/<preset>` 时会自动部署 Qt 运行库。

## 在其他工程中使用

推荐用 `FetchContent` 直接引入源码，组件库随工程一起编译：

```cmake
include(FetchContent)
FetchContent_Declare(ShadcnWidgetTools
    GIT_REPOSITORY https://github.com/RainbowCandyX/ShadcnWidgetTools.git
    GIT_TAG v1.0.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ShadcnWidgetTools)

target_link_libraries(MyApp PRIVATE ShadcnWidgetTools::ShadcnWidgetTools)
```

- 正式版本由 main 分支按 `CMakeLists.txt` 中的版本号发布为 `vX.Y.Z`，`GIT_TAG` 填 [Releases](https://github.com/RainbowCandyX/ShadcnWidgetTools/releases) 中的版本号。
- 作为子工程时只编译组件库，不编译示例程序，也不修改上层工程的构建类型、输出目录与安装路径。
- Windows 上动态库需要与程序在同一目录才能直接运行，在 `FetchContent_MakeAvailable` 之前设置 `CMAKE_RUNTIME_OUTPUT_DIRECTORY`，组件库会沿用该目录。
- `cmake --install` 时只安装动态库本体（Windows 在 `bin`，Linux / macOS 在 `lib`），可配合 `qt_generate_deploy_app_script` 一起打包；需要完整开发包时设置 `SHADCN_INSTALL=ON`。

也可以先在本仓库执行 `cmake --install build/<preset>`，再在其他工程中通过 `CMAKE_PREFIX_PATH` 指向安装目录引用：

```cmake
find_package(ShadcnWidgetTools REQUIRED)
target_link_libraries(MyApp PRIVATE ShadcnWidgetTools::ShadcnWidgetTools)
```

## WebAssembly 构建

需要 Qt 的 WebAssembly（多线程）组件、同版本桌面 Qt，以及用 [emsdk](https://github.com/emscripten-core/emsdk) 安装的 Emscripten 5.0.5（`emsdk install 5.0.5 && emsdk activate 5.0.5`）。

设置环境变量 `QT_WASM_DIR`（wasm 版 Qt，例如 `~/Qt/6.12.0/wasm_multithread`）与 `QT_HOST_PATH`（桌面版 Qt），每个新终端先激活 Emscripten（macOS：`source ~/emsdk/emsdk_env.sh`；Windows：`emsdk_env.bat`，PATH 中需要 `ninja`），再编译并启动本地服务器：

```bash
cmake --preset wasm-release
cmake --build --preset wasm-release
python3 scripts/serve_wasm.py build/wasm-release/bin
```

- 多线程需要 COOP / COEP 响应头，不能直接打开 `.html`，需通过 `serve_wasm.py` 访问。
- 顶层窗口调用 `show()` 会全屏，需要原尺寸时用 `setVisible(true)`。
- 不可用：截图、屏幕取色、通行密钥、身份验证、全局快捷键、窗口特效。

## 图表绘制 API

所有图表默认用平台的 GPU API 绘制，不可用时改用 CPU。

| 平台 | 可选 API（另含 Auto 与 Software） |
|---|---|
| Windows | Direct3D 11（默认）、Direct3D 12、OpenGL、Vulkan |
| macOS | Metal（默认）、OpenGL |
| Linux | OpenGL（默认）、Vulkan |

- 同一窗口只能用一种 GPU API。
- 环境变量 `SHADCN_CHART_API` 优先，可选 `software`、`opengl`、`d3d11`、`d3d12`、`vulkan`、`metal`。

## 致谢

- 设计与交互参考 [shadcn/ui](https://ui.shadcn.com) 与 [Base UI](https://base-ui.com)。
- 图标来自 [Lucide](https://lucide.dev)。
- Emoji 来自 [Twemoji Mozilla](https://github.com/mozilla/twemoji-colr)，为适配 Qt 调整了字形结构，图形未改动。
- WebAssembly 版示例程序的中文字体为 [Noto Sans SC](https://github.com/notofonts/noto-cjk)。

## 许可证

本项目基于 [MIT 许可证](LICENSE) 发布。内置的 Lucide 图标字体遵循 ISC 许可证，见 `ShadcnWidgetTools/Resource/Font/LICENSE-lucide.txt`。Twemoji 字体遵循 CC-BY 4.0，见 `ShadcnWidgetTools/Resource/Font/LICENSE-twemoji.md`。Noto Sans SC 字体遵循 SIL OFL 1.1，见 `ShadcnWidgetToolsExample/Resource/Font/LICENSE-noto-sans-sc.txt`。
