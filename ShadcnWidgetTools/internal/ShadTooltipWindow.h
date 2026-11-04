#ifndef SHADTOOLTIPWINDOW_H
#define SHADTOOLTIPWINDOW_H

#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"

class QPaintEvent;
class QVariantAnimation;

// ShadTooltip 使用的提示窗口，不对外导出
// 对应 TooltipContent：bg-foreground text-background text-xs rounded-md px-3 py-1.5，带 size-2.5 的箭头
// 打开与关闭时淡入淡出、缩放 95% 并向锚点方向位移 8px（slide-in-from-*-2）
class ShadTooltipWindow : public QWidget
{
    Q_OBJECT

    public:
        ShadTooltipWindow();
        ~ShadTooltipWindow() override;

        // 在屏幕坐标的锚点矩形旁显示；已经显示时直接移动到新位置，不重新播放动画（对应 skipDelayDuration）
        void showFor(const QRect& anchorRect, const QString& text, const QStringList& keys, ShadPlacementType::Side side, Qt::LayoutDirection direction);
        // 播放关闭动画后隐藏
        void hideAnimated();
        bool isClosing() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        // 内容区（不含箭头与窗口边距）的尺寸
        QSize contentSize() const;
        QRect contentRect() const;
        void startAnimation(qreal endValue);

        QString _text;
        QStringList _keys;
        ShadPlacementType::Side _side{ShadPlacementType::Top};
        // 箭头在内容区交叉方向上的中心位置
        int _arrowCenter{0};
        qreal _progress{0.0};
        bool _isClosing{false};
        QVariantAnimation* _animation{nullptr};
};

#endif // SHADTOOLTIPWINDOW_H
