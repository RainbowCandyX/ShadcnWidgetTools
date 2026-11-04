#ifndef RTLPAGE_H
#define RTLPAGE_H

#include <QPointer>

#include "ExampleBasePage.h"

class ShadWidget;

// RTL 布局演示：演示区域固定为 RightToLeft，不受标题栏 RTL 开关影响
class RtlPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit RtlPage(QWidget* parent = nullptr);
        ~RtlPage() override;

    private:
        // 当前打开的独立窗口，不设父组件，页面析构时删除
        QPointer<ShadWidget> _rtlWindow;
};

#endif // RTLPAGE_H
