#ifndef WIDGETPAGE_H
#define WIDGETPAGE_H

#include <QPointer>

#include "ExampleBasePage.h"

class ShadWidget;

// 独立窗口示例：基础用法、标题栏按钮与自定义组件、固定尺寸、置顶与拦截关闭按钮
class WidgetPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit WidgetPage(QWidget* parent = nullptr);
        ~WidgetPage() override;

    private:
        // 当前打开的示例窗口，不设父组件，页面析构时删除
        QPointer<ShadWidget> _basicWindow;
        QPointer<ShadWidget> _titleBarWindow;
        QPointer<ShadWidget> _buttonPositionWindow;
        QPointer<ShadWidget> _fixedWindow;
        QPointer<ShadWidget> _stayOnTopWindow;
        QPointer<ShadWidget> _editorWindow;
};

#endif // WIDGETPAGE_H
