#ifndef LUAPAGE_H
#define LUAPAGE_H

#include "ExampleBasePage.h"

// Lua 脚本示例，仅在 SHADCN_BUILD_LUA 开启时编译
class LuaPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit LuaPage(QWidget* parent = nullptr);
        ~LuaPage() override;
};

#endif // LUAPAGE_H
