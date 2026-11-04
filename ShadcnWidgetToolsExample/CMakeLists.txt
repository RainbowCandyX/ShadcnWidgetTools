set(EXAMPLE_TARGET ShadcnWidgetToolsExample)

file(GLOB EXAMPLE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB EXAMPLE_PAGE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ExamplePage/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/ExamplePage/*.cpp")
# AI 对话页面（AiChatPage、AiChatClient）由 SHADCN_EXAMPLE_AI_CHAT 控制，关闭时不编译也不依赖 Qt Network
if (SHADCN_EXAMPLE_AI_CHAT)
    find_package(Qt6 REQUIRED COMPONENTS Network)
else ()
    list(FILTER EXAMPLE_PAGE_FILES EXCLUDE REGEX "/AiChat[^/]*$")
endif ()
message(STATUS "示例 AI 对话页面: ${SHADCN_EXAMPLE_AI_CHAT}")
source_group(ExamplePage FILES ${EXAMPLE_PAGE_FILES})

# macOS 上生成 .app bundle，install 时 Qt 部署脚本只支持 bundle
qt_add_executable(${EXAMPLE_TARGET} WIN32 MACOSX_BUNDLE
    ${EXAMPLE_FILES}
    ${EXAMPLE_PAGE_FILES}
)

# 新增页面文件时触发 AUTOMOC，原因见 ShadcnWidgetTools/CMakeLists.txt
set(_example_autogen_sources ${EXAMPLE_FILES} ${EXAMPLE_PAGE_FILES})
list(SORT _example_autogen_sources)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsExampleSourceList.cmake" CONTENT "# 自动生成，用于触发 AUTOMOC：@_example_autogen_sources@\n" @ONLY)
include("${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsExampleSourceList.cmake")
unset(_example_autogen_sources)

# 原文为简体中文，ts 只保存英文译文；.qm 嵌入资源 :/i18n，main.cpp 通过 ShadI18n 注册；修改示例文字后构建 ShadcnWidgetToolsExample_lupdate 目标更新 ts
# Qt 6.7 起需要 SOURCE_TARGETS 限定源码范围，否则会收集项目中的全部目标（包括组件库），原因同 ShadcnWidgetTools/CMakeLists.txt
if (SHADCN_TRANSLATIONS_ENABLED)
    set(_example_translation_sources "")
    if (Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
        set(_example_translation_sources SOURCE_TARGETS ${EXAMPLE_TARGET})
    endif ()
    qt_add_translations(${EXAMPLE_TARGET}
        ${_example_translation_sources}
        TS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsExample_en.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsExample_zh_TW.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsExample_ja.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsExample_ko.ts"
        RESOURCE_PREFIX "/i18n"
    )
    unset(_example_translation_sources)
endif ()

target_include_directories(${EXAMPLE_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ExamplePage
)

target_link_libraries(${EXAMPLE_TARGET} PRIVATE
    Qt6::Widgets
    ShadcnWidgetTools::ShadcnWidgetTools
)

if (SHADCN_EXAMPLE_AI_CHAT)
    target_link_libraries(${EXAMPLE_TARGET} PRIVATE Qt6::Network)
    target_compile_definitions(${EXAMPLE_TARGET} PRIVATE SHADCN_EXAMPLE_AI_CHAT)
endif ()

set_target_properties(${EXAMPLE_TARGET} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "PATH=${SHADCN_QT_DIR}/bin;%PATH%"
)

if (EMSCRIPTEN)
    target_link_options(${EXAMPLE_TARGET} PUBLIC -sASYNCIFY -Os)
    qt_add_resources(${EXAMPLE_TARGET} "ShadcnWidgetToolsExampleFonts"
        PREFIX "/fonts"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Font"
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Font/NotoSansSC-Regular.otf"
    )
endif ()

# Linux 的 install RPATH 由 qt_standard_project_setup 设置为 $ORIGIN/../lib；
# macOS 需要手动指定，安装后的 bundle 位于 <prefix>/ShadcnWidgetToolsExample.app，可执行文件向上三级即 <prefix>，
# macdeployqt 依赖该 RPATH 找到 libShadcnWidgetTools 并复制进 bundle
if (APPLE)
    set_target_properties(${EXAMPLE_TARGET} PROPERTIES
        INSTALL_RPATH "@executable_path/../../../${CMAKE_INSTALL_LIBDIR}"
    )
endif ()

# 编译后不复制 Qt 运行库，Windows 上运行时依赖 PATH 中的 ${SHADCN_QT_DIR}/bin，macOS / Linux 依赖构建 RPATH；只有 install 时才部署 Qt 运行库，用于打包发布
install(TARGETS ${EXAMPLE_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)
qt_generate_deploy_app_script(
    TARGET ${EXAMPLE_TARGET}
    OUTPUT_SCRIPT SHADCN_EXAMPLE_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
# Windows 上 Qt 插件与 exe 一起放在 bin，与 windeployqt 默认布局一致，发布时只需 bin 目录；部署脚本在同一作用域读取该变量
if (WIN32)
    install(CODE "set(QT_DEPLOY_PLUGINS_DIR \"${CMAKE_INSTALL_BINDIR}\")")
endif ()
install(SCRIPT ${SHADCN_EXAMPLE_DEPLOY_SCRIPT})
