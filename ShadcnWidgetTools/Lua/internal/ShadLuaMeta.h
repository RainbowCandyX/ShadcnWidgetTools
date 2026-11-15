#ifndef SHADLUAMETA_H
#define SHADLUAMETA_H

struct lua_State;

// 通过 Qt 元对象系统向 Lua 提供属性、信号、槽与枚举，不对外导出
namespace ShadLuaMeta
{
    void install(lua_State* L);

    // 只能在没有待析构的 C++ 局部对象时调用
    int raise(lua_State* L);
} // namespace ShadLuaMeta

#endif // SHADLUAMETA_H
