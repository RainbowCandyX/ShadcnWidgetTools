#ifndef SHADHOVERCARD_H
#define SHADHOVERCARD_H

#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QHideEvent;
class QObject;
class QPaintEvent;
class QVBoxLayout;

// 悬停卡片，不抢占焦点
class ShadHoverCardPrivate;

class SHADCN_EXPORT ShadHoverCard : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadHoverCard)
    Q_PROPERTY(int openDelay READ getOpenDelay WRITE setOpenDelay)
    Q_PROPERTY(int closeDelay READ getCloseDelay WRITE setCloseDelay)
    Q_PROPERTY(int contentWidth READ getContentWidth WRITE setContentWidth)
    Q_PROPERTY(int contentPadding READ getContentPadding WRITE setContentPadding)

    public:
        explicit ShadHoverCard(QWidget* parent = nullptr);
        ~ShadHoverCard() override;

        // 触发组件不归卡片所有
        void setTrigger(QWidget* trigger);
        QWidget* getTrigger() const;

        QVBoxLayout* contentLayout() const;

        void setSide(ShadPlacementType::Side side);
        ShadPlacementType::Side getSide() const;
        void setAlign(ShadPlacementType::Align align);
        ShadPlacementType::Align getAlign() const;

        // 毫秒，默认 700
        void setOpenDelay(int msecs);
        int getOpenDelay() const;
        // 毫秒，默认 300
        void setCloseDelay(int msecs);
        int getCloseDelay() const;

        // 默认 256
        void setContentWidth(int width);
        int getContentWidth() const;
        // 默认 16
        void setContentPadding(int padding);
        int getContentPadding() const;

        // 立即显示
        void popup();

        QSize sizeHint() const override;

    Q_SIGNALS:
        void aboutToShow();
        void aboutToHide();

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADHOVERCARD_H
