# third_party/hyperlpr3/ — HyperLPR3 SDK 放置目录

把您本机编译好的 HyperLPR3 SDK 完整目录复制（或软链接）到此处：

```bash
# 方式 1：复制（推荐，独立于源码树）
cp -r ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3 \
      ~/Desktop/prj/parking-ms/third_party/hyperlpr3

# 方式 2：软链接（节省空间，但要求源路径稳定）
ln -s ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3 \
      ~/Desktop/prj/parking-ms/third_party/hyperlpr3
```

放置后目录结构应为：

```
parking-ms/third_party/hyperlpr3/
├── include/
│   ├── hyper_lpr_sdk.h          ← C API 头文件
│   └── MNN/                     ← MNN 推理引擎头文件
├── lib/
│   ├── libhyperlpr3.so          ← 共享库（运行时需要）
│   └── libMNN.a                 ← 静态库（链接时需要）
└── resource/
    ├── models/
    │   └── r2_mobile/           ← 6 个 .mnn 模型文件
    │       ├── b320_backbone_h.mnn
    │       ├── b320_header_h.mnn
    │       ├── b640x_backbone_h.mnn
    │       ├── b640x_head_h.mnn
    │       ├── litemodel_cls_96xh.mnn
    │       └── rpv3_mdict_160h.mnn
    ├── font/
    │   └── platech.ttf
    └── images/                  ← 测试图片（可选）
```

## 验证

放置后执行：

```bash
ls third_party/hyperlpr3/include/hyper_lpr_sdk.h   # 应存在
ls third_party/hyperlpr3/lib/libhyperlpr3.so       # 应存在
ls third_party/hyperlpr3/lib/libMNN.a              # 应存在
ls third_party/hyperlpr3/resource/models/r2_mobile/*.mnn  # 应有 6 个文件
```

## 编译

放置完成后，CMake 会**自动检测**到 `third_party/hyperlpr3/`，无需任何 `-DHYPERLPR3_ROOT` 参数：

```bash
cd parking-ms
mkdir build && cd build
cmake .. -G Ninja
cmake --build . -j$(nproc)
```

CMake 配置阶段会输出类似：
```
-- HyperLPR3 root (auto-detected): /path/to/parking-ms/third_party/hyperlpr3
-- HyperLPR3 SDK root: /path/to/parking-ms/third_party/hyperlpr3
--   include: .../include
--   lib:     .../lib/libhyperlpr3.so
--   MNN:     .../lib/libMNN.a
--   models:  .../resource/models/r2_mobile
```

## 运行时

CMake 已配置 RPATH `$ORIGIN/../lib`，但那是相对可执行文件的。如果 SDK 放在 `third_party/`，运行时需确保 `libhyperlpr3.so` 可被找到，有两种方式：

**方式 A（推荐）**：运行前设置环境变量
```bash
export LD_LIBRARY_PATH=$(pwd)/third_party/hyperlpr3/lib:$LD_LIBRARY_PATH
./build/parking_ms
```

**方式 B**：使用启动脚本
```bash
./scripts/run.sh
```
`run.sh` 已自动把 `third_party/hyperlpr3/lib` 加入 `LD_LIBRARY_PATH`。

**方式 C**：复制 .so 到构建目录
```bash
cp third_party/hyperlpr3/lib/libhyperlpr3.so build/
cd build && ./parking_ms
```

## 其他放置方式（不放到 third_party/）

如果不想放到工程内，也可：

1. **保持原位置**：CMake 会回退到 `~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3`
2. **命令行指定任意路径**：
   ```bash
   cmake .. -DHYPERLPR3_ROOT=/any/path/to/hyperlpr3
   ```
3. **系统目录**：`sudo cp -r hyperlpr3 /opt/` （CMake 会兜底查找 `/opt/hyperlpr3`）
