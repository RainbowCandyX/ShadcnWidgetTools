#ifndef SHADLUABINDINGS_H
#define SHADLUABINDINGS_H

// Qt 基础类型与组件库组件的内置绑定，不对外导出
namespace ShadLuaBindings
{
    // 只在第一次调用时执行
    void registerAll();
    void registerDataComponents();
    void registerFormComponents();
    void registerLayoutComponents();
    void registerOverlayComponents();
    void registerDisplayComponents();
    // 只在同时编译地图库时提供
    void registerMapComponents();
} // namespace ShadLuaBindings

#endif // SHADLUABINDINGS_H
