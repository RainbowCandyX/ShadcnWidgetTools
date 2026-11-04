set(SHADCN_TARGET ShadcnWidgetTools)

# macOS 窗口特效使用 Objective-C++
if (APPLE)
    enable_language(OBJCXX)
endif ()

file(GLOB SHADCN_PUBLIC_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SHADCN_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB SHADCN_PRIVATE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/private/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/private/*.cpp")
file(GLOB SHADCN_INTERNAL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp")

# 不用 GPU 绘制时排除 QRhi 绘制层
if (NOT SHADCN_CHART_GPU_ENABLED)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadChartView\\.(h|cpp)$")
endif ()

if (NOT WIN32)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadPasskeyBackendWin\\.cpp$")
endif ()
if (WIN32 OR APPLE)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadPasskeyBackendUnsupported\\.cpp$")
endif ()
if (NOT (WIN32 AND MSVC))
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadBiometricBackendWin\\.cpp$")
endif ()
if ((WIN32 AND MSVC) OR APPLE)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadBiometricBackendUnsupported\\.cpp$")
endif ()
if (NOT WIN32)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadGlobalShortcutWin\\.cpp$")
endif ()
if (WIN32 OR APPLE)
    list(FILTER SHADCN_INTERNAL_FILES EXCLUDE REGEX "/ShadGlobalShortcutUnsupported\\.cpp$")
endif ()

source_group(include FILES ${SHADCN_PUBLIC_HEADERS})
source_group(private FILES ${SHADCN_PRIVATE_FILES})
source_group(internal FILES ${SHADCN_INTERNAL_FILES})

if (SHADCN_BUILD_STATIC)
    set(SHADCN_LIB_TYPE STATIC)
else ()
    set(SHADCN_LIB_TYPE SHARED)
endif ()

qt_add_library(${SHADCN_TARGET} ${SHADCN_LIB_TYPE}
    ${SHADCN_PUBLIC_HEADERS}
    ${SHADCN_PUBLIC_SOURCES}
    ${SHADCN_PRIVATE_FILES}
    ${SHADCN_INTERNAL_FILES}
)
add_library(ShadcnWidgetTools::ShadcnWidgetTools ALIAS ${SHADCN_TARGET})

# Ninja 下新增源文件不会触发 AUTOMOC 重新运行；把文件列表写入 include 的 .cmake 文件，列表变化时让 AUTOMOC 重新运行
set(_shadcn_autogen_sources ${SHADCN_PUBLIC_HEADERS} ${SHADCN_PUBLIC_SOURCES} ${SHADCN_PRIVATE_FILES} ${SHADCN_INTERNAL_FILES})
list(SORT _shadcn_autogen_sources)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsSourceList.cmake" CONTENT "# 自动生成，用于触发 AUTOMOC：@_shadcn_autogen_sources@\n" @ONLY)
include("${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsSourceList.cmake")
unset(_shadcn_autogen_sources)

set(SHADCN_RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Font/lucide.ttf")

# macOS 自带的 Apple Color Emoji 已包含国旗，不打包
if (NOT APPLE)
    list(APPEND SHADCN_RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Font/TwemojiMozilla.ttf")
endif ()
qt_add_resources(${SHADCN_TARGET} "ShadcnWidgetToolsResource"
    PREFIX "/ShadcnWidgetTools"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Resource"
    FILES
    ${SHADCN_RESOURCE_FILES}
    OUTPUT_TARGETS SHADCN_RESOURCE_TARGETS
)

set(SHADCN_SHADER_TARGETS "")
if (SHADCN_CHART_GPU_ENABLED)
    # 图表着色器，编译为 .qsb（含 SPIR-V、GLSL、HLSL 与 MSL）：chart 为三角形带，chartpoint 为点，chartimage 为热力图
    qt_add_shaders(${SHADCN_TARGET} "ShadcnWidgetToolsShaders"
        PREFIX "/ShadcnWidgetTools"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/Resource"
        FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chart.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chart.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chartpoint.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chartpoint.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chartimage.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shader/chartimage.frag"
        OUTPUT_TARGETS SHADCN_SHADER_TARGETS
    )
endif ()

set(SHADCN_TRANSLATION_TARGETS "")
if (SHADCN_TRANSLATIONS_ENABLED)
    # 原文为简体中文，ts 只保存英文译文；lrelease 生成的 .qm 嵌入资源 :/ShadcnWidgetTools/i18n，由 ShadI18n 加载
    # 修改内置文字后构建 ShadcnWidgetTools_lupdate 目标更新 ts
    # Qt 6.7 起不再从第一个参数的目标提取源码，未指定 SOURCE_TARGETS 时会收集项目中的全部目标（包括示例程序）；6.7 之前没有该参数
    set(_shadcn_translation_sources "")
    if (Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
        set(_shadcn_translation_sources SOURCE_TARGETS ${SHADCN_TARGET})
    endif ()
    qt_add_translations(${SHADCN_TARGET}
        ${_shadcn_translation_sources}
        TS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetTools_en.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetTools_zh_TW.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetTools_ja.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetTools_ko.ts"
        RESOURCE_PREFIX "/ShadcnWidgetTools/i18n"
        OUTPUT_TARGETS SHADCN_TRANSLATION_TARGETS
    )
    unset(_shadcn_translation_sources)
    # ShadI18n 只在有翻译时注册内置翻译文件
    target_compile_definitions(${SHADCN_TARGET} PRIVATE SHADCN_TRANSLATIONS)
endif ()

target_compile_definitions(${SHADCN_TARGET} PRIVATE SHADCN_WIDGETTOOLS_LIBRARY)
if (SHADCN_BUILD_STATIC)
    target_compile_definitions(${SHADCN_TARGET} PUBLIC SHADCN_STATIC)
endif ()
# 库内部用 SHADCN_CHART_GPU 判断是否有 GPU 绘制层
if (SHADCN_CHART_GPU_ENABLED)
    target_compile_definitions(${SHADCN_TARGET} PRIVATE SHADCN_CHART_GPU)
endif ()

target_include_directories(${SHADCN_TARGET}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/ShadcnWidgetTools>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

target_link_libraries(${SHADCN_TARGET} PUBLIC Qt6::Widgets)
# QRhi 头文件只随 GuiPrivate 提供，与 Qt 小版本绑定
if (SHADCN_CHART_GPU_ENABLED)
    target_link_libraries(${SHADCN_TARGET} PRIVATE Qt6::GuiPrivate)
endif ()
# ShadWindow 通过 DwmSetWindowAttribute 让系统边框跟随主题明暗；屏幕取色通过 EnumWindows 读取窗口位置；
# Windows 10 的窗口阴影通过 CreateDIBSection 与 UpdateLayeredWindow 绘制；窗口特效通过 DWM 实现
if (WIN32)
    target_link_libraries(${SHADCN_TARGET} PRIVATE dwmapi user32 gdi32)
endif ()
if (WIN32 AND MSVC)
    target_link_libraries(${SHADCN_TARGET} PRIVATE windowsapp winbio)
endif ()
# 颜色选择器的屏幕取色通过 CGPreflightScreenCaptureAccess 检查屏幕录制权限，通过 CGWindowListCopyWindowInfo 读取窗口位置
# 窗口特效通过 NSVisualEffectView 实现
if (APPLE)
    target_sources(${SHADCN_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadWindowEffectMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadPasskeyBackendMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadBiometricBackendMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadGlobalShortcutMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadDesktopCaptureMac.mm")
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadPasskeyBackendMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadBiometricBackendMac.mm" "${CMAKE_CURRENT_SOURCE_DIR}/internal/ShadDesktopCaptureMac.mm" PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(${SHADCN_TARGET} PRIVATE "-framework CoreGraphics" "-framework CoreFoundation" "-framework AppKit" "-framework AuthenticationServices" "-framework LocalAuthentication" "-framework Carbon" "-framework QuartzCore" "-framework CoreImage" "-framework CoreMedia" "-framework CoreVideo" "-Wl,-weak_framework,ScreenCaptureKit")
endif ()

# GCC / Clang 默认导出全部符号，设为 hidden 后只导出 SHADCN_EXPORT 标记的符号，与 MSVC 的 DLL 导出行为一致
set_target_properties(${SHADCN_TARGET} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    DEBUG_POSTFIX "d"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# 安装与 find_package 导出
include(CMakePackageConfigHelpers)
set(SHADCN_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/ShadcnWidgetTools")

install(TARGETS ${SHADCN_TARGET} ${SHADCN_RESOURCE_TARGETS} ${SHADCN_SHADER_TARGETS} ${SHADCN_TRANSLATION_TARGETS}
    EXPORT ShadcnWidgetToolsTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION include/ShadcnWidgetTools
)
install(FILES ${SHADCN_PUBLIC_HEADERS} DESTINATION include/ShadcnWidgetTools)
# 只有 MSVC 系链接器生成 PDB，其他编译器使用 TARGET_PDB_FILE 会在生成阶段报错
if (MSVC AND NOT SHADCN_BUILD_STATIC)
    install(FILES $<TARGET_PDB_FILE:${SHADCN_TARGET}> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif ()

install(EXPORT ShadcnWidgetToolsTargets
    NAMESPACE ShadcnWidgetTools::
    FILE ShadcnWidgetToolsTargets.cmake
    DESTINATION ${SHADCN_CMAKE_INSTALL_DIR}
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/ShadcnWidgetToolsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsConfig.cmake"
    INSTALL_DESTINATION ${SHADCN_CMAKE_INSTALL_DIR}
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsConfigVersion.cmake"
    DESTINATION ${SHADCN_CMAKE_INSTALL_DIR}
)
