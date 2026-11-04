#ifndef SHADWINDOWPRIVATE_H
#define SHADWINDOWPRIVATE_H

#include <QPointer>

#include "ShadProperty.h"

class QVBoxLayout;
class QWidget;
class ShadTitleBar;
class ShadWindow;
class ShadWindowFrame;

class ShadWindowPrivate
{
    SHAD_D_CREATE(ShadWindow)

    public:
        ShadWindowPrivate();
        ~ShadWindowPrivate();

        static constexpr int kDefaultTitleBarHeight = 40;

        QVBoxLayout* _rootLayout{nullptr};
        ShadTitleBar* _titleBar{nullptr};
        // 边框、原生消息与边缘缩放，ShadDialog 共用同一实现
        ShadWindowFrame* _frame{nullptr};
        QPointer<QWidget> _centralWidget;
        int _titleBarHeight{kDefaultTitleBarHeight};
};

#endif // SHADWINDOWPRIVATE_H
