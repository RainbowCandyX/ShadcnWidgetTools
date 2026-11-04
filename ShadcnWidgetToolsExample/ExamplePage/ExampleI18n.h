#ifndef EXAMPLEI18N_H
#define EXAMPLEI18N_H

#include <QObject>
#include <QString>

#include <functional>

#include "ShadI18n.h"

// 示例程序的界面文字绑定：切换语言时原地刷新文字，组件不重建，选中、展开、滚动与输入等状态都保留
namespace ExampleI18n
{
    // 返回当前语言文字的函数，内部调用 tr()；用于页面标题、区块标题等由基类负责刷新的文字
    using TextFunction = std::function<QString()>;

    // 立即执行一次 apply，之后每次切换语言（ShadI18n::languageChanged）再执行一次；owner 销毁后自动断开
    // apply 中用 tr() 设置文字，依赖当前状态的文字（例如计数）按当前状态重新计算；不要在 apply 中改动用户可编辑的内容
    template <typename Function>
    void bind(QObject* owner, Function apply)
    {
        apply();
        QObject::connect(shadI18n, &ShadI18n::languageChanged, owner, [apply]() mutable { apply(); });
    }
} // namespace ExampleI18n

#endif // EXAMPLEI18N_H
