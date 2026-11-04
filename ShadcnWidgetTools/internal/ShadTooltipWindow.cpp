#include "ShadTooltipWindow.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include <utility>

#include "ShadPlacement.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // px-3 py-1.5 text-xs，过长时在 320px 处换行
    constexpr int kPaddingX = 12;
    constexpr int kPaddingY = 6;
    constexpr int kFontPixelSize = 12;
    constexpr int kMaxTextWidth = 320;
    // 文字与快捷键之间、快捷键之间的间距
    constexpr int kKeysGap = 8;
    constexpr int kKeyGap = 4;
    // 箭头 size-2.5 旋转 45 度，中心位于内容边缘内 2px，露出约 5px；内容与锚点之间留出箭头的长度
    constexpr qreal kArrowSize = 10.0;
    constexpr qreal kArrowInset = 2.0;
    constexpr int kArrowLength = 5;
    // 窗口四周留给箭头与位移动画的透明边距
    constexpr int kWindowMargin = 16;
    // slide-in-from-*-2 与 zoom-in-95，持续 150ms
    constexpr qreal kSlideDistance = 8.0;
    constexpr qreal kZoomFrom = 0.95;
    constexpr int kAnimationDurationMs = 150;

    QFont tooltipFont(const QFont& baseFont)
    {
        QFont font = baseFont;
        font.setPixelSize(kFontPixelSize);
        font.setWeight(QFont::Normal);
        return font;
    }
} // namespace

ShadTooltipWindow::ShadTooltipWindow()
    : QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    _animation = new QVariantAnimation(this);
    _animation->setDuration(kAnimationDurationMs);
    _animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _progress = value.toReal();
        setWindowOpacity(_progress);
        update();
    });
    connect(_animation, &QVariantAnimation::finished, this, [this]()
    {
        if (_isClosing)
        {
            hide();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTooltipWindow::~ShadTooltipWindow()
{
}

void ShadTooltipWindow::showFor(const QRect& anchorRect, const QString& text, const QStringList& keys, ShadPlacementType::Side side, Qt::LayoutDirection direction)
{
    const bool wasShowing = isVisible() && !_isClosing;
    _text = text;
    _keys = keys;
    setLayoutDirection(direction);
    const QSize content = contentSize();
    // 箭头居中对准锚点（align="center"），内容与锚点之间留出箭头的长度
    const ShadPlacement::Result placement = ShadPlacement::place(anchorRect, content, side, ShadPlacementType::Center, kArrowLength, direction);
    _side = placement.side;
    const bool isVerticalSide = _side == ShadPlacementType::Top || _side == ShadPlacementType::Bottom;
    // 箭头对准锚点中心，内容被屏幕边缘推开时限制在圆角之内
    const int radius = shadTheme->getRadius(ShadThemeType::Medium);
    const int arrowLimit = radius + int(kArrowSize);
    if (isVerticalSide)
    {
        _arrowCenter = qBound(arrowLimit, anchorRect.center().x() - placement.rect.left(), qMax(arrowLimit, content.width() - arrowLimit));
    }
    else
    {
        _arrowCenter = qBound(arrowLimit, anchorRect.center().y() - placement.rect.top(), qMax(arrowLimit, content.height() - arrowLimit));
    }
    setGeometry(placement.rect.adjusted(-kWindowMargin, -kWindowMargin, kWindowMargin, kWindowMargin));
    _isClosing = false;
    if (wasShowing)
    {
        // 已显示时直接切换内容与位置
        _animation->stop();
        _progress = 1.0;
        setWindowOpacity(1.0);
        update();
        return;
    }
    if (!isVisible())
    {
        _progress = 0.0;
        setWindowOpacity(0.0);
        show();
    }
    startAnimation(1.0);
}

void ShadTooltipWindow::hideAnimated()
{
    if (!isVisible() || _isClosing)
    {
        return;
    }
    _isClosing = true;
    startAnimation(0.0);
}

bool ShadTooltipWindow::isClosing() const
{
    return _isClosing;
}

void ShadTooltipWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF content(contentRect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor background = shadTheme->getColor(ShadThemeType::Foreground);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // 动画：从锚点方向位移 8px，并以箭头所在处为原点从 95% 放大
    const qreal remaining = 1.0 - _progress;
    QPointF origin;
    QPointF slide;
    switch (_side)
    {
        case ShadPlacementType::Top:
            {
                origin = QPointF(content.left() + _arrowCenter, content.bottom());
                slide = QPointF(0, kSlideDistance * remaining);
                break;
            }
        case ShadPlacementType::Bottom:
            {
                origin = QPointF(content.left() + _arrowCenter, content.top());
                slide = QPointF(0, -kSlideDistance * remaining);
                break;
            }
        case ShadPlacementType::Left:
            {
                origin = QPointF(content.right(), content.top() + _arrowCenter);
                slide = QPointF(kSlideDistance * remaining, 0);
                break;
            }
        case ShadPlacementType::Right:
            {
                origin = QPointF(content.left(), content.top() + _arrowCenter);
                slide = QPointF(-kSlideDistance * remaining, 0);
                break;
            }
    }
    const qreal scale = kZoomFrom + (1.0 - kZoomFrom) * _progress;
    painter.translate(slide);
    painter.translate(origin);
    painter.scale(scale, scale);
    painter.translate(-origin);

    fillRoundedRect(&painter, content, radius, background);
    // 箭头：旋转 45 度、rounded-[2px] 的方块，中心位于内容边缘内 2px
    QPointF arrowCenter = origin;
    switch (_side)
    {
        case ShadPlacementType::Top:
            {
                arrowCenter.ry() -= kArrowInset;
                break;
            }
        case ShadPlacementType::Bottom:
            {
                arrowCenter.ry() += kArrowInset;
                break;
            }
        case ShadPlacementType::Left:
            {
                arrowCenter.rx() -= kArrowInset;
                break;
            }
        case ShadPlacementType::Right:
            {
                arrowCenter.rx() += kArrowInset;
                break;
            }
    }
    painter.save();
    painter.translate(arrowCenter);
    painter.rotate(45.0);
    fillRoundedRect(&painter, QRectF(-kArrowSize / 2.0, -kArrowSize / 2.0, kArrowSize, kArrowSize), 2.0, background);
    painter.restore();

    // 文字在起始侧，快捷键跟在后面；按 LTR 计算后在内容区内镜像
    const Qt::LayoutDirection direction = layoutDirection();
    const QFont font = tooltipFont(this->font());
    const QFontMetrics metrics(font);
    const QRectF inner = content.adjusted(kPaddingX, kPaddingY, -kPaddingX, -kPaddingY);
    int keysWidth = 0;
    for (const QString& key : std::as_const(_keys))
    {
        keysWidth += kbdSize(this->font(), key).width();
    }
    keysWidth += qMax(0, int(_keys.size()) - 1) * kKeyGap;
    const qreal textWidth = qMax<qreal>(0.0, inner.width() - (_keys.isEmpty() ? 0 : keysWidth + kKeysGap));
    if (!_text.isEmpty())
    {
        painter.setFont(font);
        // 不随窗口特效变透明
        painter.setPen(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background));
        const QRectF logicalText(inner.left(), inner.top(), textWidth, inner.height());
        painter.drawText(visualRect(direction, inner, logicalText), visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter) | Qt::TextWordWrap, _text);
    }
    qreal keyLeft = inner.left() + (_text.isEmpty() ? 0.0 : textWidth + kKeysGap);
    for (const QString& key : std::as_const(_keys))
    {
        const QSize keySize = kbdSize(this->font(), key);
        const QRectF keyRect(keyLeft, inner.center().y() - keySize.height() / 2.0, keySize.width(), keySize.height());
        drawKbd(&painter, visualRect(direction, inner, keyRect), this->font(), key, true);
        keyLeft += keySize.width() + kKeyGap;
    }
}

QSize ShadTooltipWindow::contentSize() const
{
    const QFont font = tooltipFont(this->font());
    QSize textSize;
    if (!_text.isEmpty())
    {
        const QRect bounds = QFontMetrics(font).boundingRect(QRect(0, 0, kMaxTextWidth, 100000), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, _text);
        // 单行文字用浮点宽度向上取整，避免被换行
        const int singleLineWidth = ShadStyleHelper::textWidth(font, _text);
        textSize = QSize(singleLineWidth <= kMaxTextWidth && !_text.contains(QLatin1Char('\n')) ? singleLineWidth : bounds.width(), bounds.height());
    }
    int keysWidth = 0;
    int keysHeight = 0;
    for (const QString& key : _keys)
    {
        const QSize keySize = ShadStyleHelper::kbdSize(this->font(), key);
        keysWidth += keySize.width();
        keysHeight = qMax(keysHeight, keySize.height());
    }
    keysWidth += qMax(0, int(_keys.size()) - 1) * kKeyGap;
    int width = textSize.width() + keysWidth;
    if (!_text.isEmpty() && !_keys.isEmpty())
    {
        width += kKeysGap;
    }
    const int height = qMax(textSize.height(), keysHeight);
    return QSize(width + 2 * kPaddingX, height + 2 * kPaddingY);
}

QRect ShadTooltipWindow::contentRect() const
{
    return rect().adjusted(kWindowMargin, kWindowMargin, -kWindowMargin, -kWindowMargin);
}

void ShadTooltipWindow::startAnimation(qreal endValue)
{
    _animation->stop();
    _animation->setStartValue(_progress);
    _animation->setEndValue(endValue);
    _animation->setDuration(qMax(1, qRound(kAnimationDurationMs * qAbs(endValue - _progress))));
    _animation->start();
}
