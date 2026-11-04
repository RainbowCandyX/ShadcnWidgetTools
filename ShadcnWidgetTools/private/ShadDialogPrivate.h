#ifndef SHADDIALOGPRIVATE_H
#define SHADDIALOGPRIVATE_H

#include <QPointer>
#include <QSize>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadProperty.h"

class QHBoxLayout;
class QScreen;
class QVBoxLayout;
class QVariantAnimation;
class QWidget;
class ShadDialog;
class ShadDialogOverlay;
class ShadLabel;
class ShadTitleBar;
class ShadWindowFrame;

class ShadDialogPrivate
{
    SHAD_D_CREATE(ShadDialog)

    public:
        ShadDialogPrivate();
        ~ShadDialogPrivate();

        static constexpr int kDefaultTitleBarHeight = 40;
        static constexpr int kDefaultWidth = 448;
        static constexpr int kPadding = 24;
        static constexpr int kSectionGap = 24;
        static constexpr int kHeaderGap = 8;
        static constexpr int kFooterGap = 8;
        static constexpr int kScreenMargin = 16;
        static constexpr int kAnimationDurationMs = 100;

        QWidget* hostWindow() const;
        // 有父窗口时取其所在屏幕，否则取鼠标所在屏幕
        QScreen* targetScreen() const;
        // 只在窗口未显示时调用
        void updateWindowHints();
        void updateHeader();
        void autoResize();
        // 焦点交给正文中第一个可通过 Tab 聚焦的组件，没有时交给对话框自身
        void focusFirstChild();
        void showOverlay();
        void releaseOverlay();
        void setProgress(qreal progress);

        QVBoxLayout* _rootLayout{nullptr};
        ShadTitleBar* _titleBar{nullptr};
        ShadWindowFrame* _frame{nullptr};
        QWidget* _body{nullptr};
        QWidget* _header{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QWidget* _footer{nullptr};
        QHBoxLayout* _footerLayout{nullptr};
        QVariantAnimation* _animation{nullptr};
        QPointer<ShadDialogOverlay> _overlay;
        int _titleBarHeight{kDefaultTitleBarHeight};
        bool _isOverlayVisible{true};
        // 非模态时也显示遮罩
        bool _isNonModalOverlay{false};
        // 关闭动画进行中，结束后以 _pendingResult 调用 QDialog::done
        bool _isClosing{false};
        int _pendingResult{0};
        qreal _progress{1.0};
        // 上一次自动计算的尺寸；打开时窗口尺寸与之不同说明用户改过尺寸，保持不变
        bool _hasAutoSize{false};
        QSize _autoSize;
        bool _isOpening{false};
        Qt::WindowFlags _flagsBeforeOpen;
        // macOS 上显示期间把窗口模态改为应用模态，隐藏时恢复
        bool _isModalityOverridden{false};
        bool _wasModalitySet{false};
};

#endif // SHADDIALOGPRIVATE_H
