# cmake/FindHyperLPR3.cmake
# 查找 HyperLPR3 SDK（用户本机编译产物）
#
# 查找顺序：
#   1. HYPERLPR3_ROOT（命令行 -D 或 CMakeLists 自动设置）
#   2. 工程内 third_party/hyperlpr3/
#   3. 用户本机 ~/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3
#
# 用法:
#   find_package(HyperLPR3 REQUIRED)
#   target_link_libraries(app PRIVATE HyperLPR3::HyperLPR3)
#
# 提供变量:
#   HYPERLPR3_INCLUDE_DIR  - 头文件目录
#   HYPERLPR3_LIBRARY      - libhyperlpr3.so 路径
#   MNN_LIBRARY            - libMNN.a 路径

# 候选路径列表
set(_HYPERLPR3_CANDIDATE_PATHS
    "${HYPERLPR3_ROOT}"
    "${CMAKE_SOURCE_DIR}/third_party/hyperlpr3"
    "$ENV{HOME}/Desktop/prj/HyperLPR/build/linux/install/hyperlpr3"
    "/opt/hyperlpr3"
)

find_path(HYPERLPR3_INCLUDE_DIR
    NAMES hyper_lpr_sdk.h
    PATHS ${_HYPERLPR3_CANDIDATE_PATHS}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
    REQUIRED)

# 找到头文件后，反推 SDK 根目录
get_filename_component(_HYPERLPR3_INC_DIR "${HYPERLPR3_INCLUDE_DIR}" ABSOLUTE)
get_filename_component(_HYPERLPR3_ROOT "${_HYPERLPR3_INC_DIR}/.." ABSOLUTE)

find_library(HYPERLPR3_LIBRARY
    NAMES hyperlpr3
    PATHS "${_HYPERLPR3_ROOT}/lib"
    NO_DEFAULT_PATH
    REQUIRED)

find_library(MNN_LIBRARY
    NAMES MNN
    PATHS "${_HYPERLPR3_ROOT}/lib"
    NO_DEFAULT_PATH
    REQUIRED)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HyperLPR3
    REQUIRED_VARS HYPERLPR3_INCLUDE_DIR HYPERLPR3_LIBRARY MNN_LIBRARY)

if(HyperLPR3_FOUND AND NOT TARGET HyperLPR3::HyperLPR3)
    add_library(HyperLPR3::HyperLPR3 SHARED IMPORTED)
    set_target_properties(HyperLPR3::HyperLPR3 PROPERTIES
        IMPORTED_LOCATION "${HYPERLPR3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HYPERLPR3_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${MNN_LIBRARY}"
    )
    message(STATUS "HyperLPR3 SDK root: ${_HYPERLPR3_ROOT}")
    message(STATUS "  include: ${HYPERLPR3_INCLUDE_DIR}")
    message(STATUS "  lib:     ${HYPERLPR3_LIBRARY}")
    message(STATUS "  MNN:     ${MNN_LIBRARY}")

    # 顺便把 SDK 中的模型目录也记录下来，供应用读取
    set(HYPERLPR3_MODELS_DIR "${_HYPERLPR3_ROOT}/resource/models/r2_mobile"
        CACHE PATH "HyperLPR3 models directory")
    if(EXISTS "${HYPERLPR3_MODELS_DIR}")
        message(STATUS "  models:  ${HYPERLPR3_MODELS_DIR}")
    else()
        message(WARNING "  models directory not found: ${HYPERLPR3_MODELS_DIR}")
    endif()
endif()
