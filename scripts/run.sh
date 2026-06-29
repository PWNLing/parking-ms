#!/usr/bin/env bash
# 启动脚本 (run.sh)
# 自动 source 用户的 Qt6 环境脚本 + 设置 HyperLPR3 库路径 + 启动 parking_ms
#
# 用法:
#   1. 确保已按《远程Z服务器运行qt.md》准备好 Qt6/OpenCV/HyperLPR3 环境
#   2. 编译: cd parking-ms && mkdir build && cd build && cmake .. && cmake --build .
#   3. 运行: ./scripts/run.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"

# ---------- 1. 自动 source 用户的 Qt6 环境脚本 ----------
# 按常见位置查找 env.sh（设置 PATH/LD_LIBRARY_PATH/CMAKE_PREFIX_PATH/QT_PLUGIN_PATH）
if [ -z "${QT_PLUGIN_PATH:-}" ] || [ -z "${CMAKE_PREFIX_PATH:-}" ]; then
    for envsh in "$HOME/qt6pkgs/env.sh" "$HOME/env.sh" "$APP_DIR/env.sh" \
                 "$HOME/qt6pkgs/root/env.sh"; do
        if [ -f "$envsh" ]; then
            echo "✅ source Qt6 环境: $envsh"
            # shellcheck disable=SC1090
            source "$envsh"
            break
        fi
    done
fi

# ---------- 2. 查找 HyperLPR3 库目录 ----------
HYPERLPR3_LIB=""
for candidate in \
    "${APP_DIR}/third_party/hyperlpr3/lib" \
    "${APP_DIR}/lib" \
    "${HOME}/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3/lib"; do
    if [ -f "${candidate}/libhyperlpr3.so" ]; then
        HYPERLPR3_LIB="${candidate}"
        break
    fi
done

if [ -z "${HYPERLPR3_LIB}" ]; then
    echo "❌ 未找到 libhyperlpr3.so"
    echo "   请将 HyperLPR3 SDK 放到: ${APP_DIR}/third_party/hyperlpr3/"
    echo "   详见 third_party/README.md"
    exit 1
fi
echo "✅ HyperLPR3 库: ${HYPERLPR3_LIB}"

# 合并 LD_LIBRARY_PATH（HyperLPR3 优先）
export LD_LIBRARY_PATH="${HYPERLPR3_LIB}:${LD_LIBRARY_PATH:-}"

# ---------- 3. 确保 Qt 插件路径可用 ----------
if [ -z "${QT_PLUGIN_PATH:-}" ]; then
    echo "⚠  未设置 QT_PLUGIN_PATH，若启动报 'Failed to load platform plugin' 或 'Driver not loaded'，"
    echo "    请先 source 你的 Qt6 环境脚本（env.sh），或在 run.sh 中手动设置："
    echo "    export QT_PLUGIN_PATH=/home/z/qt6pkgs/root/usr/lib/x86_64-linux-gnu/qt6/plugins"
fi

# 离屏渲染（无显示器时）；如有显示器可改为 xcb
if [ -z "${QT_QPA_PLATFORM:-}" ] && [ -z "${DISPLAY:-}" ]; then
    export QT_QPA_PLATFORM=offscreen
    echo "ℹ️  无 DISPLAY，使用 offscreen 渲染"
fi

# ---------- 4. 定位可执行文件 ----------
BIN="${APP_DIR}/parking_ms"
if [ ! -x "${BIN}" ]; then
    BIN="${APP_DIR}/build/parking_ms"
fi
if [ ! -x "${BIN}" ]; then
    echo "❌ 未找到可执行文件 parking_ms"
    echo "   请先编译: cd ${APP_DIR} && mkdir build && cd build && cmake .. -G Ninja && cmake --build . -j\$(nproc)"
    exit 1
fi

# ---------- 5. 切换工作目录并启动 ----------
cd "${APP_DIR}"
mkdir -p data logs
echo "🚀 启动 parking_ms ..."
echo "   工作目录: ${APP_DIR}"
echo "   LD_LIBRARY_PATH 首项: $(echo "${LD_LIBRARY_PATH}" | cut -d: -f1)"
echo "   QT_PLUGIN_PATH: ${QT_PLUGIN_PATH:-<未设置>}"
echo "   QT_QPA_PLATFORM: ${QT_QPA_PLATFORM:-<默认>}"
exec "${BIN}" "$@"
