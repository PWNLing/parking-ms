# 构建说明 (BUILD.md)

## 环境要求

| 依赖 | 最低版本 | 备注 |
|------|----------|------|
| C++ 编译器 | GCC 9+ / Clang 10+ | 需 C++17 支持 |
| CMake | 3.21+ | |
| Qt6 | 6.2+ | **必需**: Core Gui Widgets Sql；**可选**: Charts（饼图）、Multimedia（摄像头） |
| OpenCV | 4.5+ | core imgproc imgcodecs |
| HyperLPR3 | 用户已编译 | 见下文路径 |
| SQLite | 内置于 Qt6 Sql 模块 | 无需单独安装 |

> **可选模块说明**：若未安装 `qt6-charts-dev`，饼图自动降级为文本显示；若未安装 `qt6-multimedia-dev`，摄像头自动降级为"选择本地图片识别"。核心功能（出入库、查询、账户、预约）不依赖这两个模块。

## 安装依赖（Ubuntu/Debian）

```bash
# Qt6 核心（必需）
sudo apt install qt6-base-dev qt6-base-dev-tools

# Qt6 可选模块（推荐安装以获得完整体验）
sudo apt install qt6-charts-dev qt6-multimedia-dev

# OpenCV
sudo apt install libopencv-dev

# 编译工具
sudo apt install build-essential cmake ninja-build
```

## HyperLPR3 编译产物

按用户提供的路径，本机 HyperLPR3 安装在：

```
~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/
├── include/hyper_lpr_sdk.h
├── include/MNN/...
├── lib/libhyperlpr3.so
├── lib/libMNN.a
└── resource/models/r2_mobile/*.mnn
```

如路径不同，构建时通过 `-DHYPERLPR3_ROOT=...` 指定。

## ★ 推荐做法：把 HyperLPR3 放到工程内 third_party/

最省心的方式：把 SDK 直接复制（或软链接）到工程内的 `third_party/hyperlpr3/`，CMake 会**自动检测**，无需任何参数：

```bash
# 假设您的 SDK 在 ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3
cp -r ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3 \
      ~/Desktop/prj/parking-ms/third_party/hyperlpr3

# 验证目录结构
ls ~/Desktop/prj/parking-ms/third_party/hyperlpr3/
# 应看到: include  lib  resource
```

放置后目录应为：
```
parking-ms/third_party/hyperlpr3/
├── include/
│   ├── hyper_lpr_sdk.h
│   └── MNN/
├── lib/
│   ├── libhyperlpr3.so
│   └── libMNN.a
└── resource/
    ├── models/r2_mobile/*.mnn   (6 个模型文件)
    ├── font/platech.ttf
    └── images/                  (测试图，可选)
```

详见 `third_party/README.md`。

## 构建步骤

### 方式 A：SDK 放在 third_party/（推荐，零参数）

```bash
cd ~/Desktop/prj/parking-ms
mkdir build && cd build

# Qt6 路径按实际安装位置调整；HyperLPR3 自动检测
cmake .. -DCMAKE_PREFIX_PATH=/usr/lib/qt6/cmake -G Ninja

cmake --build . -j$(nproc)

# 运行（用启动脚本自动设置 LD_LIBRARY_PATH）
cd ..
./scripts/run.sh
# 或手动：
# export LD_LIBRARY_PATH=$(pwd)/third_party/hyperlpr3/lib:$LD_LIBRARY_PATH
# ./build/parking_ms
```

### 方式 B：SDK 在用户本机原位置（无需复制）

```bash
cd ~/Desktop/prj/parking-ms
mkdir build && cd build

cmake .. \
    -DCMAKE_PREFIX_PATH=/usr/lib/qt6/cmake \
    -DHYPERLPR3_ROOT=$HOME/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3 \
    -G Ninja

cmake --build . -j$(nproc)

# 运行
export LD_LIBRARY_PATH=$HOME/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/lib:$LD_LIBRARY_PATH
./parking_ms
```

### 方式 C：SDK 在任意自定义位置

```bash
cmake .. -DHYPERLPR3_ROOT=/your/custom/path/to/hyperlpr3 -G Ninja
```

## 部署

构建完成后，建议将以下文件复制到部署目录：

```
parking-ms-deploy/
├── parking_ms                  # 可执行文件
├── libhyperlpr3.so             # 复制自 HyperLPR3/lib
├── config.json                 # 首次运行生成
├── data/parking.db             # 首次运行生成
└── logs/                       # 运行日志
```

运行时需让 `libhyperlpr3.so` 可被找到：

```bash
# 方式 1：设置 LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/parking-ms-deploy:$LD_LIBRARY_PATH
./parking_ms

# 方式 2：安装到系统目录
sudo cp libhyperlpr3.so /usr/local/lib/
sudo ldconfig
```

CMake 已配置 RPATH `$ORIGIN/../lib`，若按上述部署目录结构放置则无需手动设置环境变量。

## 常见问题

### Q1: Qt6 找不到？
确认 `CMAKE_PREFIX_PATH` 指向 Qt6 的 cmake 目录，常见路径：
- `/usr/lib/qt6/cmake`（系统安装）
- `/opt/Qt/6.x.x/gcc_64/lib/cmake`（官方安装包）

### Q2: OpenCV 找不到？
`apt install libopencv-dev` 后通常自动找到。如自定义路径：
```bash
cmake .. -DOpenCV_DIR=/path/to/opencv/build
```

### Q3: HyperLPR3 初始化失败？
检查启动时的初始化向导中「模型目录」是否包含 `*.mnn` 文件。可手动验证：
```bash
ls ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/resource/models/r2_mobile/*.mnn
```
应看到 6 个 .mnn 文件。

### Q4: 摄像头不可用？
系统会自动回退到「选择本地图片识别」模式。可用 HyperLPR3 自带测试图验证识别功能：
```
~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/resource/images/test_img.jpg
```

### Q5: 数据库损坏如何重置？
删除 `data/parking.db` 与 `config.json`，重启应用会触发初始化向导。

## 调试

```bash
# Debug 构建
cmake .. -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build .

# 运行（日志输出到 logs/parking_ms_YYYYMMDD.log）
./parking_ms

# 实时查看日志
tail -f logs/parking_ms_$(date +%Y%m%d).log
```
