#ifndef SHADLUADEF_H
#define SHADLUADEF_H

#include <QFlags>
#include <QtCore/qglobal.h>

// Lua 绑定库始终为静态库，不需要导出或导入标记
#define SHADCN_LUA_EXPORT

namespace ShadLuaType
{
    // Isolated 新建独立状态，Shared 使用进程内共享状态
    enum Scope
    {
        Isolated = 0,
        Shared,
    };

    // 向脚本开放的标准库，当前运行时没有的库会被忽略
    enum Library
    {
        BaseLibrary      = 0x001,
        TableLibrary     = 0x002,
        StringLibrary    = 0x004,
        MathLibrary      = 0x008,
        CoroutineLibrary = 0x010,
        Utf8Library      = 0x020,
        BitLibrary       = 0x040,
        IoLibrary        = 0x080,
        OsLibrary        = 0x100,
        PackageLibrary   = 0x200,
        DebugLibrary     = 0x400,
        JitLibrary       = 0x800,
        FfiLibrary       = 0x1000,
        HttpLibrary      = 0x2000,
        // 默认开放的库，不能访问文件、进程与本机代码，但包含联网的 HttpLibrary
        SafeLibraries    = BaseLibrary | TableLibrary | StringLibrary | MathLibrary | CoroutineLibrary | Utf8Library | BitLibrary | JitLibrary | HttpLibrary,
        AllLibraries     = SafeLibraries | IoLibrary | OsLibrary | PackageLibrary | DebugLibrary | FfiLibrary,
    };

    Q_DECLARE_FLAGS(Libraries, Library)
} // namespace ShadLuaType

Q_DECLARE_OPERATORS_FOR_FLAGS(ShadLuaType::Libraries)

#endif // SHADLUADEF_H
