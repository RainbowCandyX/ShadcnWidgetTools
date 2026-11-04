#ifndef DOCKPAGE_H
#define DOCKPAGE_H

#include <QByteArray>
#include <QPointer>

#include "ExampleBasePage.h"

class ShadWidget;

class DockPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DockPage(QWidget* parent = nullptr);
        ~DockPage() override;

    private:
        ShadWidget* createDockWindow();

        // 无父组件，页面析构时删除
        QPointer<ShadWidget> _window;
        QByteArray _savedLayout;
};

#endif // DOCKPAGE_H
