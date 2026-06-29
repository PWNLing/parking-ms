#!/usr/bin/env bash
# USB 摄像头恢复脚本（无需拔插）
set -e

echo "=== USB 摄像头恢复 ==="

if ls /dev/video* >/dev/null 2>&1; then
    echo "当前 video 设备:"
    ls -l /dev/video*
else
    echo "⚠ 未发现 /dev/video* 设备（摄像头可能已掉线）"
fi
echo ""

# 查找所有 USB 摄像头的 authorized 文件路径
# 关键：busnum/devnum 在 USB 设备目录（如 1-10），不在接口目录（如 1-10:1.0）
find_camera_authorized() {
    local found=()
    for dev in /sys/class/video4linux/video*; do
        [ -d "$dev" ] || continue
        local name=$(cat "$dev/name" 2>/dev/null || echo "")
        local usbpath=$(readlink -f "$dev/device" 2>/dev/null || echo "")
        local authpath=""
        while [ -n "$usbpath" ] && [ "$usbpath" != "/" ]; do
            if [ -f "$usbpath/authorized" ]; then
                authpath="$usbpath/authorized"
                break
            fi
            usbpath=$(dirname "$usbpath")
        done
        if [ -n "$authpath" ]; then
            found+=("$dev|$name|$authpath")
        else
            echo "  ⚠ $dev ($name): 未找到 authorized 文件"
        fi
    done
    printf '%s\n' "${found[@]}"
}

echo "查找 USB 摄像头..."
mapfile -t CAMERAS < <(find_camera_authorized)

if [ ${#CAMERAS[@]} -eq 0 ]; then
    echo "✗ 未找到任何 USB 摄像头的 authorized 文件"
    echo "  请尝试物理拔插 USB 摄像头"
    exit 1
fi

echo ""
echo "找到 ${#CAMERAS[@]} 个摄像头，开始 reset..."
echo ""

for line in "${CAMERAS[@]}"; do
    IFS='|' read -r devname name authpath <<< "$line"
    echo "处理: $devname → $name"
    echo "  authorized: $authpath"

    if [ -w "$authpath" ]; then
        echo 0 > "$authpath" 2>/dev/null && sleep 1 && echo 1 > "$authpath" 2>/dev/null \
            && echo "  ✓ 已通过 authorized 重置" \
            || echo "  ⚠ 直接写入失败，尝试 sudo"
    else
        echo "  无写权限，尝试 sudo..."
    fi

    if [ ! -w "$authpath" ] || [ "$(cat "$authpath" 2>/dev/null)" != "1" ]; then
        sudo bash -c "
            echo 0 > '$authpath' 2>/dev/null
            sleep 1
            echo 1 > '$authpath' 2>/dev/null
        " && echo "  ✓ 已通过 sudo authorized 重置" || echo "  ✗ sudo 重置失败"
    fi
    echo ""
done

echo "等待设备重新枚举（3 秒）..."
sleep 3

echo ""
echo "=== 恢复后状态 ==="
if ls /dev/video* >/dev/null 2>&1; then
    echo "✓ video 设备已恢复:"
    ls -l /dev/video*
    for dev in /sys/class/video4linux/video*; do
        [ -d "$dev" ] || continue
        echo "  $(basename $dev): $(cat "$dev/name" 2>/dev/null)"
    done
    echo ""
    echo "✓ 摄像头已恢复，可重新运行 ./scripts/run.sh"
else
    echo "✗ 设备仍未恢复，请尝试："
    echo "  1. 物理拔插 USB 摄像头"
    echo "  2. sudo udevadm trigger"
    echo "  3. sudo systemctl restart systemd-udevd"
    echo "  4. 重新加载 USB 驱动: sudo modprobe -r uvcvideo && sudo modprobe uvcvideo"
fi