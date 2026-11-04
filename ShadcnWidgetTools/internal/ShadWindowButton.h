#ifndef SHADWINDOWBUTTON_H
#define SHADWINDOWBUTTON_H

#include <QAbstractButton>
#include <QSize>

#include "ShadIconType.h"

class QEnterEvent;
class QEvent;
class QPaintEvent;
class ShadTransition;

// 标题栏右侧的置顶 / 最小化 / 最大化 / 关闭按钮，不对外导出
// 悬停为 accent 背景，关闭按钮悬停为 destructive 背景；置顶按钮为可勾选按钮，置顶时常驻 accent 背景并显示 PinOff 图标
class ShadWindowButton : public QAbstractButton
{
    Q_OBJECT

    public:
        enum class Kind
        {
            StayOnTop,
            Minimize,
            Maximize,
            Restore,
            Close,
        };

        // 按钮宽度与 Windows 系统标题栏按钮一致，高度由标题栏决定
        static constexpr int kButtonWidth = 46;

        explicit ShadWindowButton(Kind kind, QWidget* parent = nullptr);
        ~ShadWindowButton() override;

        void setKind(Kind kind);
        Kind getKind() const;

        // Windows 上最大化按钮区域交给系统命中测试（用于 Windows 11 贴靠布局），Qt 收不到该区域的鼠标事件，由窗口转发悬停与按下状态
        void setNativeHovered(bool isHovered);
        void setNativePressed(bool isPressed);
        bool isNativePressed() const;

        // 窗口失去激活时图标变为 muted-foreground
        void setWindowActive(bool isActive);

        QSize sizeHint() const override;

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadIconType::IconName iconName() const;
        void updateHoverTarget();

        Kind _kind{Kind::Minimize};
        ShadTransition* _hoverTransition{nullptr};
        bool _isMouseOver{false};
        bool _isNativeHovered{false};
        bool _isNativePressed{false};
        bool _isWindowActive{true};
};

#endif // SHADWINDOWBUTTON_H
