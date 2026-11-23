set(SHADCN_MAP_TARGET ShadcnWidgetToolsMap)

find_package(Qt6 REQUIRED COMPONENTS Network)

file(GLOB SHADCN_MAP_PUBLIC_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SHADCN_MAP_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB SHADCN_MAP_PRIVATE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/private/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/private/*.cpp")
file(GLOB SHADCN_MAP_INTERNAL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp")
source_group(include FILES ${SHADCN_MAP_PUBLIC_HEADERS})
source_group(private FILES ${SHADCN_MAP_PRIVATE_FILES})
source_group(internal FILES ${SHADCN_MAP_INTERNAL_FILES})

# 始终编译为静态库，链接进使用它的程序
qt_add_library(${SHADCN_MAP_TARGET} STATIC
    ${SHADCN_MAP_PUBLIC_HEADERS}
    ${SHADCN_MAP_PUBLIC_SOURCES}
    ${SHADCN_MAP_PRIVATE_FILES}
    ${SHADCN_MAP_INTERNAL_FILES}
)
add_library(ShadcnWidgetTools::ShadcnWidgetToolsMap ALIAS ${SHADCN_MAP_TARGET})

# 新增源文件时触发 AUTOMOC，原因见 ShadcnWidgetTools/CMakeLists.txt
set(_shadcn_map_autogen_sources ${SHADCN_MAP_PUBLIC_HEADERS} ${SHADCN_MAP_PUBLIC_SOURCES} ${SHADCN_MAP_PRIVATE_FILES} ${SHADCN_MAP_INTERNAL_FILES})
list(SORT _shadcn_map_autogen_sources)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsMapSourceList.cmake" CONTENT "# 自动生成，用于触发 AUTOMOC：@_shadcn_map_autogen_sources@\n" @ONLY)
include("${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsMapSourceList.cmake")
unset(_shadcn_map_autogen_sources)

if (SHADCN_TRANSLATIONS_ENABLED)
    # .qm 嵌入资源 :/ShadcnWidgetToolsMap/i18n，由 ShadMap 注册到 ShadI18n
    set(_shadcn_map_translation_sources "")
    if (Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
        set(_shadcn_map_translation_sources SOURCE_TARGETS ${SHADCN_MAP_TARGET})
    endif ()
    qt_add_translations(${SHADCN_MAP_TARGET}
        ${_shadcn_map_translation_sources}
        TS_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsMap_en.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsMap_zh_TW.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsMap_ja.ts"
        "${CMAKE_CURRENT_SOURCE_DIR}/i18n/ShadcnWidgetToolsMap_ko.ts"
        RESOURCE_PREFIX "/ShadcnWidgetToolsMap/i18n"
    )
    unset(_shadcn_map_translation_sources)
    target_compile_definitions(${SHADCN_MAP_TARGET} PRIVATE SHADCN_MAP_TRANSLATIONS)
endif ()

target_compile_options(${SHADCN_MAP_TARGET} PRIVATE ${SHADCN_UTF8_COMPILE_OPTIONS})
# 瓦片请求的 User-Agent 使用组件库版本号
target_compile_definitions(${SHADCN_MAP_TARGET} PRIVATE SHADCN_MAP_VERSION="${PROJECT_VERSION}")

target_include_directories(${SHADCN_MAP_TARGET}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/private
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

target_link_libraries(${SHADCN_MAP_TARGET} PUBLIC ShadcnWidgetTools::ShadcnWidgetTools Qt6::Widgets)
target_link_libraries(${SHADCN_MAP_TARGET} PRIVATE Qt6::Network)

set_target_properties(${SHADCN_MAP_TARGET} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    DEBUG_POSTFIX "d"
)
