# BBR-based Bandwidth Estimation Demo

BWE (Bandwidth Estimation) 是一个跨平台 C++ 模块，用于移动设备（iOS 和 Android）的带宽和网络质量估测。它借鉴了 GCC 和 BBRv2 的设计理念，能够处理每秒的播放器统计数据（如抖动、延迟、接收字节数、缓冲级别等），为自适应比特率 (ABR) 和用户体验质量 (QoE) 提供带宽及质量估计。

## 目录结构

- **`src/network`**: 数据模型和测量收集。
- **`src/bwe`**: 信号预处理、BWE 核心算法和质量评分。
- **`test_common`**: **测试专用**的合成数据生成工具，包括 [`network_simulator.cpp`](test_common/network_simulator.cpp)，生成 1 Hz 跟踪数据供验证单元测试。
- **`src/api`**: 集成到播放器的公共 API。
- **`tests`**: 单元测试。
- **`tools`**: 包括 **拟合运行器**，基于模拟（或记录）数据运行 BWE 并评估带宽估算效果。
- **`scripts`**: 各类脚本工具（含验证和绘图脚本）。

## 快速开始

### 获取数据集
运行以下命令下载所需数据集：
```bash
./scripts/download.sh [source]
```

### 生成模拟数据
生成指定时长（单位：秒）的测试数据：
```bash
./scripts/generate_dummy.sh 300
```

### 运行测试与绘图
使用以下命令运行 BWE 模拟器并生成结果图表（`bbr_results_plot.png`）：
```bash
./scripts/test.sh [source] [additional_params...]
```
提示：`scripts/plot.py` 会被自动调用，无需单独运行。

## 编译说明
使用 CMake 和兼容 C++17 的编译器进行构建：
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest
```
注意：需要 GoogleTest 库支持，用户可以选择让 CMake 自动下载或使用本地的现有安装。

## 示例输出

运行 `test.sh` 后，将生成以下示例输出：
- **估算结果**：带宽估计曲线（PNG 文件）。
- **日志文件**：运行时的详细日志。

## 贡献指南

欢迎任何形式的贡献！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 了解相关贡献流程。