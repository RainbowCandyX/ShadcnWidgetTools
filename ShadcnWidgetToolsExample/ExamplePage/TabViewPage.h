#ifndef TABVIEWPAGE_H
#define TABVIEWPAGE_H

#include <QPointer>

#include "ExampleBasePage.h"

class ShadWidget;

class TabViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TabViewPage(QWidget* parent = nullptr);
        ~TabViewPage() override;

    private:
        // 无父组件，页面析构时删除
        QPointer<ShadWidget> _titleBarWindow;
};

#endif // TABVIEWPAGE_H
