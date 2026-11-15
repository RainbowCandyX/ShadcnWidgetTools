#ifndef SHADLUAINCLUDE_H
#define SHADLUAINCLUDE_H

// Lua 头文件必须先于 LuaBridge 包含
extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#if defined(SHADCN_LUA_JIT)
#include <luajit.h>
#endif
}

#include <LuaBridge/LuaBridge.h>

#endif // SHADLUAINCLUDE_H
