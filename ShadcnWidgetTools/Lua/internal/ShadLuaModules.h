#ifndef SHADLUAMODULES_H
#define SHADLUAMODULES_H

struct lua_State;

// shad.json、shad.xml 与 shad.http 功能模块，不对外导出
namespace ShadLuaModules
{
    // 必须在 ShadLuaMeta::install 之后调用
    void install(lua_State* L, bool isHttpEnabled);
} // namespace ShadLuaModules

#endif // SHADLUAMODULES_H
