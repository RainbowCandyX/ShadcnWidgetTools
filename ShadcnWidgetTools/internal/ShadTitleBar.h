#ifndef SHADTITLEBAR_H
#define SHADTITLEBAR_H

#include <QPoint>
#include <QPointer>
#include <QWidget>

#include "ShadDef.h"

class QEvent;
class QHBoxLayout;
class QLabel;
class QMouseEvent;
class QShowEvent;
class ShadLabel;
class ShadWindowButton;

// ShadWindow、ShadDialog 与 ShadWidget 的自绘标题栏，不对外导出
class ShadTitleBar : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadTitleBar(QWidget* window);
        ~ShadTitleBar() override;

        void setBarHeight(int height);

        void setButtonFlags(ShadWindowType::ButtonFlags buttonFlags);
        ShadWindowType::ButtonFlags getButtonFlags() const;

        void setButtonPosition(ShadWindowType::ButtonPosition position);
        ShadWindowType::ButtonPosition getButtonPosition() const;

        // 旧组件会被隐藏并延迟删除；传 nullptr 只移除
        void setAreaWidget(ShadWindowType::TitleBarArea area, QWidget* widget);
        QWidget* getAreaWidget(ShadWindowType::TitleBarArea area) const;

        // 总会发出 closeButtonClicked；为 true 时随后调用窗口的 close()
        void setDefaultCloseEnabled(bool enabled);
        bool isDefaultCloseEnabled() const;
        // 与点击关闭按钮相同
        void requestClose();

        // 显示期间修改直接作用于原生窗口，不会隐藏或重建窗口
        void setStayOnTop(bool stayOnTop);
        bool isStayOnTop() const;

        // localPos 为标题栏坐标
        bool isCaptionAt(const QPoint& localPos) const;
        bool canMaximize() const;
        void toggleMaximize();

        ShadWindowButton* getMaximizeButton() const;
        void updateMaximizeState();
        void setWindowActive(bool isActive);

    Q_SIGNALS:
        void closeButtonClicked();
        void stayOnTopChanged(bool stayOnTop);

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    private:
        void updateWindowIcon();
        void updateWindowTitle();
        void updateStayOnTopButton();
        void updateButtonLayout();

        QWidget* _window{nullptr};
        QHBoxLayout* _mainLayout{nullptr};
        QHBoxLayout* _buttonLayout{nullptr};
        QHBoxLayout* _leftAreaLayout{nullptr};
        QHBoxLayout* _rightAreaLayout{nullptr};
        QLabel* _iconLabel{nullptr};
        ShadLabel* _titleLabel{nullptr};
        QPointer<QWidget> _leftAreaWidget;
        QPointer<QWidget> _rightAreaWidget;
        ShadWindowButton* _stayOnTopButton{nullptr};
        ShadWindowButton* _minimizeButton{nullptr};
        ShadWindowButton* _maximizeButton{nullptr};
        ShadWindowButton* _closeButton{nullptr};
        ShadWindowType::ButtonFlags _buttonFlags{ShadWindowType::AllButtons};
        ShadWindowType::ButtonPosition _buttonPosition{ShadWindowType::ButtonsOnRight};
        bool _isDefaultCloseEnabled{true};
        // macOS / Linux：移动超过拖动阈值才交给系统移动窗口，不影响单击与双击
        bool _isCaptionPressed{false};
        bool _isManualMoving{false};
        QPoint _pressGlobalPos;
        QPoint _moveStartWindowPos;
};

#endif // SHADTITLEBAR_H
