#ifndef SHADNAVIGATIONMENUTRIGGER_H
#define SHADNAVIGATIONMENUTRIGGER_H

#include <QAbstractButton>
#include <QSize>
#include <QString>
#include <QtGlobal>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;
class QVariantAnimation;

// ShadNavigationMenu 的菜单项
class ShadNavigationMenuTrigger : public QAbstractButton
{
    Q_OBJECT

    public:
        ShadNavigationMenuTrigger(const QString& text, bool hasMenu, QWidget* parent = nullptr);
        ~ShadNavigationMenuTrigger() override;

        bool hasMenu() const;
        void setOpen(bool open);
        bool isOpen() const;
        // 按光标位置刷新悬停状态
        void syncHover();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void hoverChanged(bool hovered);

    protected:
        // 导航键交给 ShadNavigationMenu
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        int chevronGap() const;

        QVariantAnimation* _chevronAnimation{nullptr};
        qreal _chevronProgress{0.0};
        bool _hasMenu{false};
        bool _isOpen{false};
        bool _isHovered{false};
        bool _isFocusVisible{false};
};

#endif // SHADNAVIGATIONMENUTRIGGER_H
