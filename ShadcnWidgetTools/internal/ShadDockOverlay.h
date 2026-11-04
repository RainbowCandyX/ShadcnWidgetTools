#ifndef SHADDOCKOVERLAY_H
#define SHADDOCKOVERLAY_H

#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"

class QPaintEvent;
class QPainter;

class ShadDockOverlay : public QWidget
{
    Q_OBJECT

    public:
        struct Hit
        {
            bool isValid{false};
            bool isEdge{false};
            ShadDockType::DockArea area{ShadDockType::CenterArea};
        };

        explicit ShadDockOverlay(QWidget* parent = nullptr);
        ~ShadDockOverlay() override;

        // 空矩形时隐藏
        void setGroupRect(const QRect& rect, bool isCenterAllowed);
        Hit hitTest(const QPoint& pos) const;
        void setHover(const Hit& hit);
        // 空矩形时隐藏
        void setPreview(const QRect& rect);

        // RTL 下左右对调
        static ShadDockType::DockArea physicalArea(ShadDockType::DockArea area, Qt::LayoutDirection direction);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QRectF guideRect(bool isEdge, ShadDockType::DockArea area) const;
        bool isGuideVisible(bool isEdge, ShadDockType::DockArea area) const;
        void paintGuide(QPainter* painter, bool isEdge, ShadDockType::DockArea area) const;

        QRect _groupRect;
        bool _isCenterAllowed{false};
        Hit _hover;
        QRect _preview;
};

#endif // SHADDOCKOVERLAY_H
