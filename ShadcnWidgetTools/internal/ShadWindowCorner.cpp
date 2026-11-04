#include "ShadWindowCorner.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadWindowCorner::ShadWindowCorner(Qt::Corner corner, QWidget* window)
    : QWidget(window), _corner(corner)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
}

ShadWindowCorner::~ShadWindowCorner()
{
}

void ShadWindowCorner::place(int radius)
{
    _radius = radius;
    const QWidget* window = parentWidget();
    // 覆盖圆弧与边框所在的正方形区域
    const int size = qMin(radius + 1, qMin(window->width(), window->height()));
    const int left = (_corner == Qt::TopLeftCorner || _corner == Qt::BottomLeftCorner) ? 0 : window->width() - size;
    const int top = (_corner == Qt::TopLeftCorner || _corner == Qt::TopRightCorner) ? 0 : window->height() - size;
    setGeometry(left, top, size, size);
    update();
}

void ShadWindowCorner::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (_radius <= 0)
    {
        return;
    }
    const QWidget* window = parentWidget();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 使用窗口坐标，圆弧与窗口 paint 中画的圆角矩形完全重合；超出本组件的部分被裁掉
    painter.translate(-pos());
    const QRectF windowRect(window->rect());
    QPainterPath outside;
    outside.addRect(windowRect);
    QPainterPath rounded;
    rounded.addRoundedRect(windowRect, _radius, _radius);
    // 清除模式按覆盖率混合，圆弧边缘的像素只清除一部分，得到抗锯齿的透明圆角
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillPath(outside.subtracted(rounded), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    ShadStyleHelper::strokeRoundedRect(&painter, windowRect, _radius, shadTheme->getColor(ShadThemeType::Border), 1.0);
}
