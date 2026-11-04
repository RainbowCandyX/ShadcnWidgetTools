#include "ShadScreenPickerOverlay.h"

#include <QCursor>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QScreen>
#include <QString>
#include <QWindow>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <utility>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 放大镜显示目标像素周围 11x11 个物理像素，每个放大为 12x12
    constexpr int kLoupePixels = 11;
    constexpr int kLoupeCellSize = 12;
    constexpr int kLoupeSide = kLoupePixels * kLoupeCellSize;
    constexpr int kLoupeBorder = 1;
    // 放大镜与光标的距离、放大镜与颜色标签的间距
    constexpr int kCursorOffset = 20;
    constexpr int kLabelGap = 4;
    // 颜色标签两行：第一行 R / G / B 三个标签，第二行色块与 #RRGGBB
    constexpr int kLabelPaddingX = 8;
    constexpr int kLabelPaddingY = 6;
    constexpr int kLabelRowHeight = 20;
    constexpr int kLabelRowGap = 4;
    constexpr int kLabelHeight = 2 * kLabelPaddingY + 2 * kLabelRowHeight + kLabelRowGap;
    constexpr int kChipSize = 14;
    constexpr int kLabelTextPixelSize = 12;
    // R / G / B 标签：bg-muted rounded-sm，字母为 muted-foreground，数值为 foreground，标签之间 gap-1
    constexpr int kTagGap = 4;
    constexpr int kTagTextPixelSize = 11;
    constexpr int kTagLetterGap = 3;
    // 重绘时向外扩出的范围，覆盖 shadow-md
    constexpr int kShadowMargin = 12;
    // 截图上的遮罩 bg-black/70
    constexpr qreal kMaskAlpha = 0.7;
    constexpr qreal kLoupeCrosshairWidth = 2.0;
    // 边框画在窗口内侧，贴着屏幕边缘的窗口也能完整显示
    constexpr int kHighlightBorderWidth = 2;
} // namespace

ShadScreenPickerOverlay::ShadScreenPickerOverlay(QScreen* screen, const QPixmap& screenshot, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint)
{
    const QRect screenGeometry = screen->geometry();
    // 统一按物理像素处理：截图的设备像素比设为 1，比例由截图与屏幕逻辑尺寸计算
    _screenshot = screenshot;
    _screenshot.setDevicePixelRatio(1.0);
    _image = _screenshot.toImage().convertToFormat(QImage::Format_RGB32);
    _ratio = screenGeometry.width() > 0 ? static_cast<qreal>(_screenshot.width()) / screenGeometry.width() : 1.0;

    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    // 覆盖层与截图一一对应，不随布局方向镜像
    setLayoutDirection(Qt::LeftToRight);
    // 先创建原生窗口并指定屏幕，混合 DPI 时按该屏幕的缩放比例显示
    winId();
    if (QWindow* window = windowHandle())
    {
        window->setScreen(screen);
    }
    setGeometry(screenGeometry);
}

ShadScreenPickerOverlay::~ShadScreenPickerOverlay()
{
}

void ShadScreenPickerOverlay::syncWithCursor()
{
    const QPoint position = mapFromGlobal(QCursor::pos());
    if (!rect().contains(position))
    {
        if (_hasTarget)
        {
            const QRect oldRect = loupeRect().adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin);
            _hasTarget = false;
            update(oldRect);
            updateHighlight();
        }
        return;
    }
    setTargetPixel(pixelAt(QPointF(position)));
}

void ShadScreenPickerOverlay::setWindowRects(const QList<QRect>& rects)
{
    _windowRects.clear();
    const QPoint origin = geometry().topLeft();
    for (const QRect& rect : rects)
    {
        const QRect localRect = rect.translated(-origin).intersected(this->rect());
        if (!localRect.isEmpty())
        {
            _windowRects.append(localRect);
        }
    }
    updateHighlight();
}

void ShadScreenPickerOverlay::enterEvent(QEnterEvent* event)
{
    // 光标进入的屏幕接收键盘输入
    activateWindow();
    setFocus(Qt::OtherFocusReason);
    setTargetPixel(pixelAt(event->position()));
    QWidget::enterEvent(event);
}

void ShadScreenPickerOverlay::leaveEvent(QEvent* event)
{
    syncWithCursor();
    QWidget::leaveEvent(event);
}

void ShadScreenPickerOverlay::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Escape:
            event->accept();
            Q_EMIT canceled();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            event->accept();
            if (_hasTarget)
            {
                Q_EMIT colorPicked(colorAt(_targetPixel));
            }
            return;
        case Qt::Key_Left:
            setTargetPixel(_targetPixel + QPoint(-1, 0));
            break;
        case Qt::Key_Right:
            setTargetPixel(_targetPixel + QPoint(1, 0));
            break;
        case Qt::Key_Up:
            setTargetPixel(_targetPixel + QPoint(0, -1));
            break;
        case Qt::Key_Down:
            setTargetPixel(_targetPixel + QPoint(0, 1));
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

void ShadScreenPickerOverlay::mouseMoveEvent(QMouseEvent* event)
{
    setTargetPixel(pixelAt(event->position()));
    QWidget::mouseMoveEvent(event);
}

void ShadScreenPickerOverlay::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    if (event->button() == Qt::RightButton)
    {
        Q_EMIT canceled();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        const QColor color = colorAt(pixelAt(event->position()));
        if (color.isValid())
        {
            Q_EMIT colorPicked(color);
        }
    }
}

void ShadScreenPickerOverlay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    // 只绘制需要更新的区域，源区域按物理像素换算
    const QRect dirtyRect = event->rect();
    const QRectF sourceRect(dirtyRect.x() * _ratio, dirtyRect.y() * _ratio, dirtyRect.width() * _ratio, dirtyRect.height() * _ratio);
    painter.drawPixmap(QRectF(dirtyRect), _screenshot, sourceRect);
    // 遮罩表示正在取色，高亮窗口内不画遮罩；放大镜与取色使用原始截图，不受遮罩影响
    const QColor maskColor(0, 0, 0, qRound(255 * kMaskAlpha));
    for (const QRect& maskRect : QRegion(dirtyRect).subtracted(QRegion(_highlightRect)))
    {
        painter.fillRect(maskRect, maskColor);
    }
    if (!_highlightRect.isEmpty() && _highlightRect.intersects(dirtyRect))
    {
        const QColor borderColor = shadTheme->getColor(ShadThemeType::Destructive);
        const QRect& area = _highlightRect;
        const int border = qMin(kHighlightBorderWidth, qMin(area.width(), area.height()) / 2);
        painter.fillRect(QRect(area.left(), area.top(), area.width(), border), borderColor);
        painter.fillRect(QRect(area.left(), area.bottom() - border + 1, area.width(), border), borderColor);
        painter.fillRect(QRect(area.left(), area.top() + border, border, area.height() - 2 * border), borderColor);
        painter.fillRect(QRect(area.right() - border + 1, area.top() + border, border, area.height() - 2 * border), borderColor);
    }
    if (_hasTarget)
    {
        paintLoupe(&painter);
    }
}

QPoint ShadScreenPickerOverlay::pixelAt(const QPointF& position) const
{
    return QPoint(qFloor(position.x() * _ratio), qFloor(position.y() * _ratio));
}

QColor ShadScreenPickerOverlay::colorAt(const QPoint& pixel) const
{
    if (!_image.valid(pixel))
    {
        return QColor();
    }
    return QColor::fromRgb(_image.pixel(pixel));
}

void ShadScreenPickerOverlay::setTargetPixel(const QPoint& pixel)
{
    if (_image.isNull())
    {
        return;
    }
    const QPoint boundedPixel(qBound(0, pixel.x(), _image.width() - 1), qBound(0, pixel.y(), _image.height() - 1));
    if (_hasTarget && boundedPixel == _targetPixel)
    {
        return;
    }
    const QRect oldRect = _hasTarget ? loupeRect().adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin) : QRect();
    _targetPixel = boundedPixel;
    _hasTarget = true;
    const QRect newRect = loupeRect().adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin);
    update(oldRect.united(newRect));
    updateHighlight();
}

void ShadScreenPickerOverlay::updateHighlight()
{
    // 列表从前到后，第一个包含目标像素中心的窗口即为最上层
    QRect highlightRect;
    if (_hasTarget)
    {
        const QPointF center = (QPointF(_targetPixel) + QPointF(0.5, 0.5)) / _ratio;
        for (const QRect& rect : std::as_const(_windowRects))
        {
            if (QRectF(rect).contains(center))
            {
                highlightRect = rect;
                break;
            }
        }
    }
    if (highlightRect == _highlightRect)
    {
        return;
    }
    update(_highlightRect);
    update(highlightRect);
    _highlightRect = highlightRect;
}

QRect ShadScreenPickerOverlay::loupeRect() const
{
    const int frameSide = kLoupeSide + 2 * kLoupeBorder;
    const int totalHeight = frameSide + kLabelGap + kLabelHeight;
    const QPointF anchor = QPointF(_targetPixel) / _ratio;
    // 默认位于光标右下方，超出屏幕时翻到另一侧
    int x = qRound(anchor.x()) + kCursorOffset;
    if (x + frameSide > width())
    {
        x = qRound(anchor.x()) - kCursorOffset - frameSide;
    }
    int y = qRound(anchor.y()) + kCursorOffset;
    if (y + totalHeight > height())
    {
        y = qRound(anchor.y()) - kCursorOffset - totalHeight;
    }
    return QRect(x, y, frameSide, totalHeight);
}

void ShadScreenPickerOverlay::paintLoupe(QPainter* painter) const
{
    using namespace ShadStyleHelper;
    const QRect area = loupeRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QRectF frameRect(area.left(), area.top(), kLoupeSide + 2 * kLoupeBorder, kLoupeSide + 2 * kLoupeBorder);
    const QRectF pixelsRect = frameRect.adjusted(kLoupeBorder, kLoupeBorder, -kLoupeBorder, -kLoupeBorder);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    drawShadowMd(painter, frameRect, radius);
    // 不随窗口特效变透明
    fillRoundedRect(painter, frameRect, radius, shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background));

    // 目标像素周围的区域按最近邻放大；超出截图的部分为黑色
    const int half = kLoupePixels / 2;
    const QImage region = _image.copy(_targetPixel.x() - half, _targetPixel.y() - half, kLoupePixels, kLoupePixels);
    painter->save();
    painter->setClipPath(roundedRectPath(pixelsRect, qMax(0.0, radius - kLoupeBorder)));
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawImage(pixelsRect, region);
    // 像素网格
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(QColor(0, 0, 0, 40), 1.0));
    for (int index = 1; index < kLoupePixels; ++index)
    {
        const qreal offset = index * kLoupeCellSize;
        painter->drawLine(QPointF(pixelsRect.left() + offset, pixelsRect.top()), QPointF(pixelsRect.left() + offset, pixelsRect.bottom()));
        painter->drawLine(QPointF(pixelsRect.left(), pixelsRect.top() + offset), QPointF(pixelsRect.right(), pixelsRect.top() + offset));
    }
    const QPointF center(pixelsRect.left() + half * kLoupeCellSize + kLoupeCellSize / 2.0, pixelsRect.top() + half * kLoupeCellSize + kLoupeCellSize / 2.0);
    const QColor crosshairColor = shadTheme->getColor(ShadThemeType::Destructive);
    painter->fillRect(QRectF(center.x() - kLoupeCrosshairWidth / 2.0, pixelsRect.top(), kLoupeCrosshairWidth, pixelsRect.height()), crosshairColor);
    painter->fillRect(QRectF(pixelsRect.left(), center.y() - kLoupeCrosshairWidth / 2.0, pixelsRect.width(), kLoupeCrosshairWidth), crosshairColor);
    painter->restore();
    strokeRoundedRect(painter, frameRect, radius, shadTheme->getColor(ShadThemeType::Border));

    // 颜色标签：第一行 R / G / B 三个标签，第二行色块与 #RRGGBB
    const QColor color = colorAt(_targetPixel).isValid() ? colorAt(_targetPixel) : QColor(Qt::black);
    const QRectF labelRect(frameRect.left(), frameRect.bottom() + kLabelGap, frameRect.width(), kLabelHeight);
    drawShadowMd(painter, labelRect, radius);
    fillRoundedRect(painter, labelRect, radius, shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Popover));
    strokeRoundedRect(painter, labelRect, radius, shadTheme->getColor(ShadThemeType::Border));
    const qreal smallRadius = shadTheme->getRadius(ShadThemeType::Small);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);

    // 第一行：三个等宽标签，字母与数值作为整体居中
    QFont tagFont = font();
    tagFont.setPixelSize(kTagTextPixelSize);
    tagFont.setWeight(QFont::Medium);
    painter->setFont(tagFont);
    const qreal contentWidth = labelRect.width() - 2 * kLabelPaddingX;
    const qreal tagWidth = (contentWidth - 2 * kTagGap) / 3.0;
    const qreal firstRowTop = labelRect.top() + kLabelPaddingY;
    const QString letters[3]{QStringLiteral("R"), QStringLiteral("G"), QStringLiteral("B")};
    const int values[3]{color.red(), color.green(), color.blue()};
    for (int index = 0; index < 3; ++index)
    {
        const QRectF tagRect(labelRect.left() + kLabelPaddingX + index * (tagWidth + kTagGap), firstRowTop, tagWidth, kLabelRowHeight);
        fillRoundedRect(painter, tagRect, smallRadius, shadTheme->getColor(ShadThemeType::Muted));
        const QString valueText = QString::number(values[index]);
        const qreal letterWidth = textWidth(tagFont, letters[index]);
        const qreal totalWidth = letterWidth + kTagLetterGap + textWidth(tagFont, valueText);
        const qreal textLeft = tagRect.left() + (tagRect.width() - totalWidth) / 2.0;
        painter->setPen(mutedForeground);
        painter->drawText(QRectF(textLeft, tagRect.top(), letterWidth, tagRect.height()), Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, letters[index]);
        painter->setPen(foreground);
        painter->drawText(QRectF(textLeft + letterWidth + kTagLetterGap, tagRect.top(), tagRect.right() - textLeft - letterWidth - kTagLetterGap, tagRect.height()), Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, valueText);
    }

    // 第二行：色块与 #RRGGBB
    const qreal secondRowTop = firstRowTop + kLabelRowHeight + kLabelRowGap;
    const QRectF chipRect(labelRect.left() + kLabelPaddingX, secondRowTop + (kLabelRowHeight - kChipSize) / 2.0, kChipSize, kChipSize);
    fillRoundedRect(painter, chipRect, smallRadius, color);
    strokeRoundedRect(painter, chipRect, smallRadius, shadTheme->getColor(ShadThemeType::Border));
    QFont hexFont = font();
    hexFont.setPixelSize(kLabelTextPixelSize);
    hexFont.setWeight(QFont::Medium);
    painter->setFont(hexFont);
    painter->setPen(foreground);
    const QRectF hexRect(chipRect.right() + kLabelPaddingX, secondRowTop, labelRect.right() - chipRect.right() - 2 * kLabelPaddingX, kLabelRowHeight);
    painter->drawText(hexRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, color.name(QColor::HexRgb).toUpper());
    painter->restore();
}
