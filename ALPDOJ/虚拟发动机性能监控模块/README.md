# Virtual Engine Performance Monitor

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![Visual Studio](https://img.shields.io/badge/Visual%20Studio-2022-5C2D91?logo=visualstudio&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-blue)

基于 **Qt Widgets + C++17 + VS2022** 开发的虚拟发动机性能监控模块。项目参考飞机 EICAS（Engine Indicating and Crew Alerting System）显示逻辑，实现双发发动机转速、排气温度、燃油余量、燃油流速的实时仿真、图形化显示、异常注入、告警检测和数据记录。

## 项目亮点

- 使用 Qt Widgets 实现 EICAS 风格黑底监控界面。
- 使用自绘 `GaugeWidget` 绘制 N1 / EGT 扇形仪表。
- 以 5 ms 周期刷新仿真数据，符合题目要求。
- 实现启动、稳态、停车三个发动机阶段。
- 仅在点击 `START` 后开始累计运行时间与 CSV 记录。
- 支持推力增加和推力减小操作。
- 支持多类异常注入与检测，包括传感器故障、燃油异常、超转、超温。
- 支持 `AUTO FAULT MODE`，可在稳态高推力下自然触发少量异常。
- 使用白色、琥珀色、红色和 `--` 表示正常、警戒、警告和无效值。
- 自动生成 CSV 数据文件，并在首次有效告警时创建 LOG 告警日志。
- 同类告警 5 秒内只记录一次，避免日志重复刷屏。

## 功能预览

程序主界面包含以下区域：

- 左侧发动机仪表区：`N1 L`、`N1 R`、`EGT L`、`EGT R`
- 中下部状态区：`START` 状态灯、`RUN` 状态灯、燃油余量、燃油流速
- 底部控制区：`START`、`STOP`、`THRUST +`、`THRUST -`、`RESET`
- 右侧告警区：CAS / ALERTS 实时告警文本
- 右侧测试区：FAULT TEST 异常注入按钮
- 右侧辅助控制：`AUTO FAULT MODE`

## 技术栈

| 模块 | 技术 |
| --- | --- |
| 语言 | C++17 |
| GUI | Qt Widgets |
| IDE | Visual Studio 2022 |
| 构建 | Visual Studio `.sln` / `.vcxproj` |
| 平台 | Windows x64 |
| 数据记录 | CSV + LOG |

## 环境要求

运行或编译本项目需要：

- Windows 10 / Windows 11
- Visual Studio 2022
- Qt VS Tools
- Qt 6.11.0 msvc2022_64
- MSVC v143 工具集

当前项目 Qt 配置名为：

```text
6.11.0_msvc2022_64
```

如果本机 Qt 配置名称不同，需要在 Visual Studio 的 Qt VS Tools 中重新选择对应版本。

## 快速开始

1. 克隆或下载本项目。
2. 使用 Visual Studio 2022 打开解决方案文件：

```text
大作业题组（二）-虚拟发动机性能监控模块开发.sln
```

3. 选择构建配置：

```text
Debug | x64
```

4. 确认 Qt VS Tools 中 Qt 版本可用。
5. 点击“本地 Windows 调试器”运行程序。

## 操作说明

| 按钮 | 功能 |
| --- | --- |
| `START` | 启动发动机，进入启动阶段 |
| `STOP` | 进入停车阶段，优先级最高 |
| `THRUST +` | 增加推力，燃油流速、转速和温度随之上升 |
| `THRUST -` | 减小推力，燃油流速、转速和温度随之下降 |
| `RESET` | 重置仿真状态，并新建数据记录文件 |

补充说明：

- 运行时间与 CSV 记录从第一次点击 `START` 开始。
- `AUTO FAULT MODE` 开启后，稳态高推力运行中会自然累积少量异常风险。

推荐演示流程：

1. 点击 `START`，观察 N1、EGT 和燃油流速逐步上升。
2. 等待进入 `RUN` 状态，确认 `RUN` 状态灯亮起。
3. 点击 `THRUST +` / `THRUST -`，观察推力调节效果。
4. 打开 `LOW FUEL`、`OVER FF`、`OVER SPD 1` 等异常按钮，观察琥珀色告警。
5. 打开 `OVER SPD 2` 或 `OVER TEMP 4`，观察红色告警和自动停车。
6. 查看 `output` 目录中的 CSV 和 LOG 文件。

## 发动机仿真逻辑

发动机状态机分为四类显示状态：

| 状态 | 含义 |
| --- | --- |
| `OFF` | 未启动或停车完成 |
| `START` | 启动阶段，包括线性增长和对数增长 |
| `RUN` | 稳态运行 |
| `STOP` | 停车阶段 |

内部阶段更细分为：

- `Off`
- `StartingLinear`
- `StartingLog`
- `Steady`
- `Stopping`

启动阶段：

- 前 2 秒转速线性增加。
- 前 2 秒燃油流速线性增加。
- 随后转速、燃油流速和 EGT 按对数趋势增长。
- 当左右发动机转速达到额定转速的 95% 后进入稳态。

稳态阶段：

- 转速、温度和燃油流速围绕目标值动态稳定。
- 推力调节会改变目标燃油流速、转速和温度。
- 在开启 `AUTO FAULT MODE` 时，高推力稳态会逐步累积超流量、一级超转和一级超温风险。

停车阶段：

- 燃油流速立即归零。
- 转速和温度按衰减函数下降。
- 约 10 秒内完成停车。

## 异常测试

右侧 `FAULT TEST` 区域提供异常注入按钮。

| 按钮 | 异常类型 |
| --- | --- |
| `N1 S1 FAIL` | 单个转速传感器故障 |
| `N1 S2 FAIL` | 单个转速传感器故障 |
| `EGT S1 FAIL` | 单个 EGT 传感器故障 |
| `EGT S2 FAIL` | 单个 EGT 传感器故障 |
| `N1 ENG FAIL` | 单发转速传感器故障 |
| `EGT ENG FAIL` | 单发 EGT 传感器故障 |
| `LOW FUEL` | 燃油余量低 |
| `FUEL FAIL` | 燃油余量传感器故障 |
| `OVER FF` | 燃油流速超限 |
| `OVER SPD 1` | 一级超转 |
| `OVER SPD 2` | 二级超转，自动停车 |
| `OVER TEMP 1` | 启动阶段一级超温 |
| `OVER TEMP 2` | 启动阶段二级超温，自动停车 |
| `OVER TEMP 3` | 稳态阶段一级超温 |
| `OVER TEMP 4` | 稳态阶段二级超温，自动停车 |

说明：

- 手动故障注入始终可用，用于稳定复现指定异常。
- 开启 `AUTO FAULT MODE` 后，系统会在正常运行中自然触发少量与工况相关的异常。

## 告警颜色规则

| 状态 | 显示方式 |
| --- | --- |
| 正常值 | 白色 |
| 警戒值 | 琥珀色 |
| 警告值 | 红色 |
| 无效值 | `--` |

告警文本使用英文，符合题目要求。

## 数据输出

程序第一次点击 `START` 或点击 `RESET` 后重新开始运行时，会新建一组记录文件。

数据文件：

```text
output/engine_data_YYYYMMDD_HHMMSS.csv
```

告警日志：

```text
output/engine_alert_YYYYMMDD_HHMMSS.log
```

CSV 字段包括：

- `time_sec`
- `phase`
- `rpm_l`
- `rpm_r`
- `n1_pct_l`
- `n1_pct_r`
- `egt_l`
- `egt_r`
- `fuel_qty`
- `fuel_flow`
- `n1_l_state`
- `n1_r_state`
- `egt_l_state`
- `egt_r_state`
- `fuel_qty_state`
- `fuel_flow_state`
- `start_light`
- `run_light`

LOG 字段包括：

- `time_sec`
- `level`
- `code`
- `text`

日志行为：

- 只有在首次产生有效告警时才创建 `.log` 文件。
- 每次写入后立即刷新到磁盘，便于程序运行中直接查看。

## 项目结构

```text
.
├── README.md
├── 大作业题组（二）-虚拟发动机性能监控模块开发.sln
└── 大作业题组（二）-虚拟发动机性能监控模块开发
    ├── main.cpp
    ├── QtWidgetsApplication.h
    ├── QtWidgetsApplication.cpp
    ├── GaugeWidget.h
    ├── GaugeWidget.cpp
    ├── EngineCore.h
    ├── EngineCore.cpp
    ├── Recorder.h
    ├── Recorder.cpp
    ├── QtWidgetsApplication.ui
    ├── QtWidgetsApplication.qrc
    ├── 大作业题组（二）-虚拟发动机性能监控模块开发.vcxproj
    └── 大作业题组（二）-虚拟发动机性能监控模块开发.vcxproj.filters
```

## 核心文件说明

| 文件 | 说明 |
| --- | --- |
| `EngineCore.h/.cpp` | 发动机仿真、状态机、异常注入、告警判定 |
| `Recorder.h/.cpp` | CSV 数据记录和 LOG 告警记录 |
| `GaugeWidget.h/.cpp` | 自绘扇形仪表控件 |
| `QtWidgetsApplication.h/.cpp` | Qt 主窗口、按钮交互、实时刷新和界面更新 |
| `main.cpp` | Qt 应用程序入口 |

## 构建验证

项目已在本机使用 Visual Studio 2022 / MSBuild 验证通过：

```text
Configuration: Debug | x64
Result: 0 warning, 0 error
```

生成程序路径示例：

```text
x64/Debug/大作业题组（二）-虚拟发动机性能监控模块开发.exe
```

## 注意事项

- 首次运行前请确认 Qt 版本配置正确。
- `output` 目录中的 CSV 和 LOG 文件为运行时生成文件，不需要手动创建。
- 如果上传 GitHub，建议忽略 `.vs/`、`x64/`、`Debug/`、`Release/`、`*.user` 等本地构建文件。

## License

本项目用于课程学习与作业展示。
