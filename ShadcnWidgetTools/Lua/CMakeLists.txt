set(SHADCN_LUA_TARGET ShadcnWidgetToolsLua)

# LuaBridge3 只需要仓库内的单个头文件，不需要联网
set(SHADCN_LUABRIDGE_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/LuaBridge3")
if (NOT EXISTS "${SHADCN_LUABRIDGE_DIR}/LuaBridge/LuaBridge.h")
    message(FATAL_ERROR "未找到 LuaBridge3 头文件 ${SHADCN_LUABRIDGE_DIR}/LuaBridge/LuaBridge.h，请确认仓库完整")
endif ()

set(SHADCN_LUA_RUNTIME_OBJECTS "")
set(SHADCN_LUA_INCLUDE_DIRS "")
set(SHADCN_LUA_LINK_LIBRARIES "")
if (SHADCN_LUA_ENGINE STREQUAL "LuaJIT")
    if (EMSCRIPTEN)
        message(FATAL_ERROR "WebAssembly 不支持 LuaJIT，请把 SHADCN_LUA_ENGINE 设为 Lua")
    endif ()
    # 用仓库内 LuaJIT 源码自带的构建脚本编译成静态库，不需要预装也不需要联网
    set(_shadcn_luajit_source_dir "${PROJECT_SOURCE_DIR}/ThirdParty/LuaJIT")
    if (NOT EXISTS "${_shadcn_luajit_source_dir}/src/Makefile")
        message(FATAL_ERROR "未找到 LuaJIT 源码 ${_shadcn_luajit_source_dir}，请确认仓库完整，或用 -DSHADCN_LUA_ENGINE=Lua 改用 PUC-Lua 5.4")
    endif ()
    # 构建脚本只能在源码目录内编译，先复制到构建目录以保持仓库源码不变
    set(_shadcn_luajit_build_dir "${CMAKE_CURRENT_BINARY_DIR}/LuaJIT")
    file(COPY "${_shadcn_luajit_source_dir}/src" "${_shadcn_luajit_source_dir}/dynasm" "${_shadcn_luajit_source_dir}/.relver" DESTINATION "${_shadcn_luajit_build_dir}")
    set(_shadcn_luajit_dir "${_shadcn_luajit_build_dir}/src")
    if (MSVC)
        set(_shadcn_luajit_library "${_shadcn_luajit_dir}/lua51.lib")
        set(_shadcn_luajit_arguments static)
        # C 运行库与本工程保持一致
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            list(PREPEND _shadcn_luajit_arguments debug)
        endif ()
        set(_shadcn_luajit_command cmd /c msvcbuild.bat ${_shadcn_luajit_arguments})
        unset(_shadcn_luajit_arguments)
    elseif (UNIX)
        find_program(SHADCN_MAKE_PROGRAM NAMES gmake make)
        if (NOT SHADCN_MAKE_PROGRAM)
            message(FATAL_ERROR "编译 LuaJIT 需要 GNU make，请安装后重试，或用 -DSHADCN_LUA_ENGINE=Lua 改用 PUC-Lua 5.4")
        endif ()
        include(ProcessorCount)
        ProcessorCount(_shadcn_luajit_jobs)
        if (_shadcn_luajit_jobs EQUAL 0)
            set(_shadcn_luajit_jobs 1)
        endif ()
        set(_shadcn_luajit_library "${_shadcn_luajit_dir}/libluajit.a")
        set(_shadcn_luajit_compiler "${CMAKE_C_COMPILER}")
        set(_shadcn_luajit_environment "")
        if (APPLE)
            # LuaJIT 一次只能编译一种架构，并要求指定最低系统版本
            list(LENGTH CMAKE_OSX_ARCHITECTURES _shadcn_luajit_architecture_count)
            if (_shadcn_luajit_architecture_count GREATER 1)
                message(FATAL_ERROR "LuaJIT 不支持通用二进制，请把 CMAKE_OSX_ARCHITECTURES 设为单一架构，或用 -DSHADCN_LUA_ENGINE=Lua 改用 PUC-Lua 5.4")
            elseif (_shadcn_luajit_architecture_count EQUAL 1)
                string(APPEND _shadcn_luajit_compiler " -arch ${CMAKE_OSX_ARCHITECTURES}")
            endif ()
            unset(_shadcn_luajit_architecture_count)
            if (CMAKE_OSX_DEPLOYMENT_TARGET)
                set(_shadcn_luajit_deployment_target "${CMAKE_OSX_DEPLOYMENT_TARGET}")
            elseif (DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
                set(_shadcn_luajit_deployment_target "$ENV{MACOSX_DEPLOYMENT_TARGET}")
            else ()
                set(_shadcn_luajit_deployment_target "11.0")
            endif ()
            list(APPEND _shadcn_luajit_environment "MACOSX_DEPLOYMENT_TARGET=${_shadcn_luajit_deployment_target}")
            unset(_shadcn_luajit_deployment_target)
            if (CMAKE_OSX_SYSROOT)
                list(APPEND _shadcn_luajit_environment "SDKROOT=${CMAKE_OSX_SYSROOT}")
            endif ()
        endif ()
        # 静态库要链接进动态库，目标文件需要位置无关
        set(_shadcn_luajit_command ${CMAKE_COMMAND} -E env ${_shadcn_luajit_environment} ${SHADCN_MAKE_PROGRAM} -j${_shadcn_luajit_jobs} libluajit.a BUILDMODE=static "CC=${_shadcn_luajit_compiler}" CFLAGS=-fPIC)
        unset(_shadcn_luajit_jobs)
        unset(_shadcn_luajit_compiler)
        unset(_shadcn_luajit_environment)
    else ()
        message(FATAL_ERROR "当前工具链不支持从源码编译 LuaJIT（Windows 上需要 MSVC），请用 -DSHADCN_LUA_ENGINE=Lua 改用 PUC-Lua 5.4")
    endif ()
    # luajit.h 由构建脚本生成，声明为副产物
    add_custom_command(OUTPUT "${_shadcn_luajit_library}" BYPRODUCTS "${_shadcn_luajit_dir}/luajit.h" COMMAND ${_shadcn_luajit_command} DEPENDS "${_shadcn_luajit_source_dir}/.relver" WORKING_DIRECTORY "${_shadcn_luajit_dir}" COMMENT "从源码编译 LuaJIT" VERBATIM)
    add_custom_target(ShadcnLuaJitBuild DEPENDS "${_shadcn_luajit_library}")
    add_library(ShadcnLuaJit STATIC IMPORTED GLOBAL)
    set_target_properties(ShadcnLuaJit PROPERTIES IMPORTED_LOCATION "${_shadcn_luajit_library}")
    add_dependencies(ShadcnLuaJit ShadcnLuaJitBuild)
    list(APPEND SHADCN_LUA_INCLUDE_DIRS "${_shadcn_luajit_dir}")
    list(APPEND SHADCN_LUA_LINK_LIBRARIES ShadcnLuaJit)
    # package 库依赖 dlopen，数学函数依赖 libm
    if (UNIX AND NOT APPLE)
        list(APPEND SHADCN_LUA_LINK_LIBRARIES ${CMAKE_DL_LIBS} m)
    endif ()
    unset(_shadcn_luajit_source_dir)
    unset(_shadcn_luajit_build_dir)
    unset(_shadcn_luajit_dir)
    unset(_shadcn_luajit_library)
    unset(_shadcn_luajit_command)
elseif (SHADCN_LUA_ENGINE STREQUAL "Lua")
    # PUC-Lua 源码在配置时下载并编译进本库，官方仓库没有 CMakeLists.txt
    include(FetchContent)
    FetchContent_Declare(lua
        GIT_REPOSITORY https://github.com/lua/lua.git
        GIT_TAG v5.4.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(lua)
    set(_shadcn_lua_names lapi lauxlib lbaselib lcode lcorolib lctype ldblib ldebug ldo ldump lfunc lgc linit liolib llex lmathlib lmem loadlib lobject lopcodes loslib lparser lstate lstring lstrlib ltable ltablib ltm lundump lutf8lib lvm lzio)
    set(_shadcn_lua_sources "")
    foreach (_shadcn_lua_name IN LISTS _shadcn_lua_names)
        list(APPEND _shadcn_lua_sources "${lua_SOURCE_DIR}/${_shadcn_lua_name}.c")
    endforeach ()
    add_library(ShadcnLuaRuntime OBJECT ${_shadcn_lua_sources})
    unset(_shadcn_lua_names)
    unset(_shadcn_lua_sources)
    unset(_shadcn_lua_name)
    set_target_properties(ShadcnLuaRuntime PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        AUTOMOC OFF
    )
    if (APPLE)
        target_compile_definitions(ShadcnLuaRuntime PRIVATE LUA_USE_MACOSX)
    elseif (UNIX AND NOT EMSCRIPTEN)
        target_compile_definitions(ShadcnLuaRuntime PRIVATE LUA_USE_POSIX)
    endif ()
    set(SHADCN_LUA_RUNTIME_OBJECTS $<TARGET_OBJECTS:ShadcnLuaRuntime>)
    list(APPEND SHADCN_LUA_INCLUDE_DIRS "${lua_SOURCE_DIR}")
else ()
    message(FATAL_ERROR "SHADCN_LUA_ENGINE 只能是 Lua 或 LuaJIT，当前为 ${SHADCN_LUA_ENGINE}")
endif ()
message(STATUS "Lua 运行时: ${SHADCN_LUA_ENGINE}")

file(GLOB SHADCN_LUA_PUBLIC_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SHADCN_LUA_PUBLIC_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB SHADCN_LUA_INTERNAL_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/internal/*.cpp")
# 地图组件的绑定只在同时编译地图库时加入
if (NOT SHADCN_BUILD_MAP)
    list(FILTER SHADCN_LUA_INTERNAL_FILES EXCLUDE REGEX "/ShadLuaMapBindings\\.cpp$")
endif ()
source_group(include FILES ${SHADCN_LUA_PUBLIC_HEADERS})
source_group(internal FILES ${SHADCN_LUA_INTERNAL_FILES})

# 始终编译为静态库，链接进使用它的程序，不单独生成动态库
qt_add_library(${SHADCN_LUA_TARGET} STATIC
    ${SHADCN_LUA_PUBLIC_HEADERS}
    ${SHADCN_LUA_PUBLIC_SOURCES}
    ${SHADCN_LUA_INTERNAL_FILES}
    ${SHADCN_LUA_RUNTIME_OBJECTS}
)
add_library(ShadcnWidgetTools::ShadcnWidgetToolsLua ALIAS ${SHADCN_LUA_TARGET})

# 新增源文件时触发 AUTOMOC，原因见 ShadcnWidgetTools/CMakeLists.txt
set(_shadcn_lua_autogen_sources ${SHADCN_LUA_PUBLIC_HEADERS} ${SHADCN_LUA_PUBLIC_SOURCES} ${SHADCN_LUA_INTERNAL_FILES})
list(SORT _shadcn_lua_autogen_sources)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsLuaSourceList.cmake" CONTENT "# 自动生成，用于触发 AUTOMOC：@_shadcn_lua_autogen_sources@\n" @ONLY)
include("${CMAKE_CURRENT_BINARY_DIR}/ShadcnWidgetToolsLuaSourceList.cmake")
unset(_shadcn_lua_autogen_sources)

target_compile_options(${SHADCN_LUA_TARGET} PRIVATE ${SHADCN_UTF8_COMPILE_OPTIONS})
# LuaBridge 的模板实例较多，MSVC 需要放宽单个目标文件的节数上限
if (MSVC)
    target_compile_options(${SHADCN_LUA_TARGET} PRIVATE /bigobj)
endif ()
if (SHADCN_LUA_ENGINE STREQUAL "LuaJIT")
    target_compile_definitions(${SHADCN_LUA_TARGET} PUBLIC SHADCN_LUA_JIT)
    # 先编译 LuaJIT 以生成 luajit.h
    add_dependencies(${SHADCN_LUA_TARGET} ShadcnLuaJitBuild)
endif ()

target_include_directories(${SHADCN_LUA_TARGET}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${SHADCN_LUABRIDGE_DIR}>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/internal
)
foreach (_shadcn_lua_include_dir IN LISTS SHADCN_LUA_INCLUDE_DIRS)
    target_include_directories(${SHADCN_LUA_TARGET} PUBLIC $<BUILD_INTERFACE:${_shadcn_lua_include_dir}>)
endforeach ()
unset(_shadcn_lua_include_dir)

target_link_libraries(${SHADCN_LUA_TARGET} PUBLIC ShadcnWidgetTools::ShadcnWidgetTools Qt6::Widgets)
find_package(Qt6 REQUIRED COMPONENTS Network)
target_link_libraries(${SHADCN_LUA_TARGET} PRIVATE Qt6::Network)
if (SHADCN_BUILD_MAP)
    target_link_libraries(${SHADCN_LUA_TARGET} PRIVATE ShadcnWidgetTools::ShadcnWidgetToolsMap)
    target_compile_definitions(${SHADCN_LUA_TARGET} PRIVATE SHADCN_LUA_MAP)
endif ()
if (SHADCN_LUA_LINK_LIBRARIES)
    target_link_libraries(${SHADCN_LUA_TARGET} PRIVATE ${SHADCN_LUA_LINK_LIBRARIES})
endif ()

set_target_properties(${SHADCN_LUA_TARGET} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    DEBUG_POSTFIX "d"
)
