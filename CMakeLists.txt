cmake_minimum_required(VERSION 3.21)
project(ParkingMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# HyperLPR3 路径查找顺序：
#   1. 命令行 -DHYPERLPR3_ROOT=/path/to/hyperlpr3
#   2. 工程内 third_party/hyperlpr3/        ← 推荐：直接放这里，零配置
#   3. 用户本机 ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3
if(NOT DEFINED HYPERLPR3_ROOT)
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/hyperlpr3/include/hyper_lpr_sdk.h")
        set(HYPERLPR3_DEFAULT "${CMAKE_SOURCE_DIR}/third_party/hyperlpr3")
    else()
        set(HYPERLPR3_DEFAULT "$ENV{HOME}/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3")
    endif()
else()
    set(HYPERLPR3_DEFAULT "${HYPERLPR3_ROOT}")
endif()
set(HYPERLPR3_ROOT "${HYPERLPR3_DEFAULT}"
    CACHE PATH "HyperLPR3 install root (推荐放到 third_party/hyperlpr3/)")
message(STATUS "HyperLPR3 root (auto-detected): ${HYPERLPR3_ROOT}")

# 查找 Qt6（核心模块必需；Charts/Multimedia 可选，缺失时自动降级）
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Sql
                       OPTIONAL_COMPONENTS Charts Multimedia)

# 查找 OpenCV（core/imgproc/imgcodecs/videoio：videoio 用于 cv::VideoCapture 摄像头取流）
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)

# 查找 HyperLPR3（自定义 find 模块）
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(HyperLPR3 REQUIRED)

# ---------- 可选模块的编译宏 ----------
if(TARGET Qt6::Charts)
    set(PARKING_MS_HAS_CHARTS TRUE)
    message(STATUS "Qt6 Charts: found (启用饼图)")
else()
    message(STATUS "Qt6 Charts: NOT found (饼图降级为文本显示，安装 qt6-charts-dev 可启用)")
endif()
if(TARGET Qt6::Multimedia)
    set(PARKING_MS_HAS_MULTIMEDIA TRUE)
    message(STATUS "Qt6 Multimedia: found (启用摄像头)")
else()
    message(STATUS "Qt6 Multimedia: NOT found (摄像头降级为图片识别，安装 qt6-multimedia-dev 可启用)")
endif()

set(PARKING_MS_HEADERS
    src/core/Session.h
    src/core/ConfigManager.h
    src/core/DatabaseManager.h
    src/core/Logger.h
    src/core/Md5.h

    src/models/ParkingInfo.h
    src/models/CarRecord.h
    src/models/UserRow.h

    src/services/AuthService.h
    src/services/VehicleService.h
    src/services/QueryService.h
    src/services/AccountService.h
    src/services/ReservationService.h

    src/recognition/PlateRecognizer.h
    src/recognition/CameraCapture.h
    src/recognition/PlateTypes.h

    src/ui/InitWizard.h
    src/ui/LoginWindow.h
    src/ui/RegisterDialog.h
    src/ui/MainWindow.h
    src/ui/ManualPlateDialog.h
    src/ui/QueryDialog.h
    src/ui/AccountDialog.h
    src/ui/ResetPwdDialog.h
    src/ui/RoleSelectDialog.h
    src/ui/UserModeWindow.h
)

# ---------- 源文件列表 ----------
set(PARKING_MS_SOURCES
    src/main.cpp

    # core
    src/core/ConfigManager.cpp
    src/core/DatabaseManager.cpp
    src/core/Session.cpp
    src/core/Md5.cpp
    src/core/Logger.cpp

    # models
    src/models/ParkingInfo.cpp
    src/models/CarRecord.cpp
    src/models/UserRow.cpp

    # services
    src/services/AuthService.cpp
    src/services/VehicleService.cpp
    src/services/QueryService.cpp
    src/services/AccountService.cpp
    src/services/ReservationService.cpp

    # recognition
    src/recognition/PlateTypes.cpp
    src/recognition/PlateRecognizer.cpp
    src/recognition/CameraCapture.cpp

    # ui
    src/ui/InitWizard.cpp
    src/ui/RoleSelectDialog.cpp
    src/ui/LoginWindow.cpp
    src/ui/RegisterDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/ManualPlateDialog.cpp
    src/ui/QueryDialog.cpp
    src/ui/AccountDialog.cpp
    src/ui/ResetPwdDialog.cpp
    src/ui/UserModeWindow.cpp
)

set(PARKING_MS_RESOURCES resources/parking_ms.qrc)

# ---------- 可执行文件 ----------
add_executable(parking_ms ${PARKING_MS_SOURCES} ${PARKING_MS_HEADERS} ${PARKING_MS_RESOURCES})

target_include_directories(parking_ms PRIVATE
    src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(parking_ms PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    ${OpenCV_LIBS}
    HyperLPR3::HyperLPR3
)

# ---------- 可选模块链接 + 编译宏 ----------
if(PARKING_MS_HAS_CHARTS)
    target_link_libraries(parking_ms PRIVATE Qt6::Charts)
    target_compile_definitions(parking_ms PRIVATE PARKING_MS_HAS_CHARTS)
endif()
if(PARKING_MS_HAS_MULTIMEDIA)
    target_link_libraries(parking_ms PRIVATE Qt6::Multimedia)
    target_compile_definitions(parking_ms PRIVATE PARKING_MS_HAS_MULTIMEDIA)
endif()

# ---------- 把 HyperLPR3 模型路径编译进二进制（作为默认值） ----------
if(DEFINED HYPERLPR3_MODELS_DIR)
    target_compile_definitions(parking_ms PRIVATE
        HYPERLPR3_DEFAULT_MODELS_PATH="${HYPERLPR3_MODELS_DIR}")
endif()

# ---------- 编译选项 ----------
target_compile_options(parking_ms PRIVATE
    -Wall -Wextra -Wno-unused-parameter
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O2>
)

# ---------- RPATH: 运行时从同级 lib/ 找 .so ----------
set_target_properties(parking_ms PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH FALSE
)

# ---------- 安装目标 ----------
install(TARGETS parking_ms DESTINATION bin)
install(FILES ${HYPERLPR3_ROOT}/lib/libhyperlpr3.so DESTINATION lib OPTIONAL)

# ---------- 提示信息 ----------
message(STATUS "=== ParkingMS Configuration ===")
message(STATUS "Qt6 version: ${Qt6Core_VERSION}")
message(STATUS "OpenCV: ${OpenCV_DIR}")
message(STATUS "HyperLPR3 root: ${HYPERLPR3_ROOT}")
message(STATUS "HyperLPR3 include: ${HYPERLPR3_INCLUDE_DIR}")
message(STATUS "HyperLPR3 lib: ${HYPERLPR3_LIBRARY}")
message(STATUS "MNN lib: ${MNN_LIBRARY}")
message(STATUS "===============================")
