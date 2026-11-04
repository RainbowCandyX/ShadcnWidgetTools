#ifndef SHADPOPUPEFFECT_H
#define SHADPOPUPEFFECT_H

#include <QObject>
#include <QRect>
#include <QRectF>
#include <QtGlobal>

#include <functional>

class QEvent;
class QPainter;
class QWidget;

// 浮层窗口的窗口特效；只过滤事件，没有 Q_OBJECT
class ShadPopupEffect : public QObject
{
    public:
        ShadPopupEffect(QWidget* popup, std::function<QRect()> cardRect);
        ~ShadPopupEffect() override;

        // 在绘制卡片底色之前调用
        static void paintBackdrop(QPainter* painter, const QWidget* popup, const QRectF& card, qreal radius);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        void apply();

        QWidget* _popup{nullptr};
        std::function<QRect()> _cardRect;
};

#endif // SHADPOPUPEFFECT_H
