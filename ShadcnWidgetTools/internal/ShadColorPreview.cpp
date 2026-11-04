#include "ShadColorPreview.h"

#include <QEvent>
#include <QMargins>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 可视高度与最小可视宽度
    constexpr int kPreviewHeight = 48;
    constexpr int kMinimumWidth = 48;
} // namespace

ShadColorPreview::ShadColorPreview(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 与输入框一样四周预留焦点环边距，可视边缘与同一列的输入框对齐
    const int ring = ShadStyleHelper::kFocusRingWidth;
    setContentsMargins(ring, ring, ring, ring);
    setMouseTracking(true);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadColorPreview::~ShadColorPreview()
{
}

void ShadColorPreview::setCurrentColor(const QColor& color)
{
    _currentColor = color;
    update();
}

void ShadColorPreview::setOriginalColor(const QColor& color)
{
    _originalColor = color;
    update();
}

QSize ShadColorPreview::sizeHint() const
{
    const QMargins margins = contentsMargins();
    return QSize(kMinimumWidth + margins.left() + margins.right(), kPreviewHeight + margins.top() + margins.bottom());
}

QSize ShadColorPreview::minimumSizeHint() const
{
    return sizeHint();
}

void ShadColorPreview::leaveEvent(QEvent* event)
{
    unsetCursor();
    QWidget::leaveEvent(event);
}

void ShadColorPreview::mouseMoveEvent(QMouseEvent* event)
{
    if (isOriginalArea(event->position().toPoint().y()))
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        unsetCursor();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadColorPreview::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isOriginalArea(event->position().toPoint().y()))
    {
        event->accept();
        Q_EMIT originalClicked();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadColorPreview::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF previewRect(contentsRect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const qreal halfHeight = previewRect.height() / 2.0;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.save();
    painter.setClipPath(roundedRectPath(previewRect, radius));
    painter.fillRect(QRectF(previewRect.left(), previewRect.top(), previewRect.width(), halfHeight), _currentColor);
    const QColor originalColor = _originalColor.isValid() ? _originalColor : _currentColor;
    painter.fillRect(QRectF(previewRect.left(), previewRect.top() + halfHeight, previewRect.width(), previewRect.height() - halfHeight), originalColor);
    painter.restore();
    strokeRoundedRect(&painter, previewRect, radius, shadTheme->getColor(ShadThemeType::Border));
}

bool ShadColorPreview::isOriginalArea(int y) const
{
    const QRect previewRect = contentsRect();
    return _originalColor.isValid() && y >= previewRect.center().y() && y <= previewRect.bottom();
}
