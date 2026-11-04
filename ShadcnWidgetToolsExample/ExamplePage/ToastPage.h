#ifndef TOASTPAGE_H
#define TOASTPAGE_H

#include <QPointer>

#include "ExampleBasePage.h"
#include "ShadWidget.h"

class ShadToaster;

// Toast 示例：toaster 由主窗口传入，另含子窗口示例
class ToastPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        ToastPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~ToastPage() override;

    private:
        QPointer<ShadWidget> _childWindow;
};

#endif // TOASTPAGE_H
