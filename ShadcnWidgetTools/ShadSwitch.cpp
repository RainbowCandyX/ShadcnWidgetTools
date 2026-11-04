#include "ShadSwitch.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtMath>

#include "ShadStyleHelper.h"
#include "ShadSwitchPrivate.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 轨道 w-8 h-[1.15rem]，滑块 size-4，轨道带 1px 透明边框
    constexpr int kTrackWidth = 32;
    constexpr int kTrackHeight = 18;
    constexpr int kThumbSize = 16;
    constexpr qreal kTrackBorder = 1.0;
    constexpr int kTextGap = 8;
} // namespace

ShadSwitch::ShadSwitch(QWidget* parent)
    : QAbstractButton(parent), d_ptr(new ShadSwitchPrivate())
{
    Q_D(ShadSwitch);
    d->q_ptr = this;
    d->_checkTransition = new ShadTransition(this);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(this, &QAbstractButton::toggled, this, [d](bool checked) { d->_checkTransition->animateTo(checked ? 1.0 : 0.0); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSwitch::ShadSwitch(const QString& text, QWidget* parent)
    : ShadSwitch(parent)
{
    setText(text);
}

ShadSwitch::~ShadSwitch()
{
}

QSize ShadSwitch::sizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const QFontMetricsF fontMetrics(font());
    const QString content = text();
    const int textWidth = content.isEmpty() ? 0 : kTextGap + qCeil(fontMetrics.horizontalAdvance(content));
    const int height = qMax(kTrackHeight, qCeil(fontMetrics.height()));
    return QSize(kTrackWidth + textWidth + margin, height + margin);
}

QSize ShadSwitch::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadSwitch::hitButton(const QPoint& pos) const
{
    return rect().contains(pos);
}

void ShadSwitch::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSwitch);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadSwitch::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadSwitch);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadSwitch::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadSwitch::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSwitch);
    using namespace ShadStyleHelper;
    const int margin = kFocusRingWidth;
    // 以下按 LTR 计算逻辑位置，绘制前用 visualRect 镜像：RTL 时轨道在右侧，滑块打开时向左移动
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF bounds(rect());
    const QRectF logicalTrackRect(margin, (height() - kTrackHeight) / 2, kTrackWidth, kTrackHeight);
    const QRectF trackRect = visualRect(direction, bounds, logicalTrackRect);
    const qreal trackRadius = kTrackHeight / 2.0;
    const qreal progress = d->_checkTransition->value();
    const bool isDark = shadTheme->isDark();
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 轨道：checked bg-primary，unchecked bg-input（暗色 bg-input/80）
    const QColor input = shadTheme->getColor(ShadThemeType::Input);
    const QColor uncheckedTrack = isDark ? withAlpha(input, 0.8) : input;
    drawShadowXs(&painter, trackRect, trackRadius);
    fillRoundedRect(&painter, trackRect, trackRadius, mix(uncheckedTrack, shadTheme->getColor(ShadThemeType::Primary), progress));
    if (showFocusRing)
    {
        drawFocusRing(&painter, trackRect, trackRadius, focusRingColor(false));
    }

    // 滑块：bg-background，暗色下 unchecked bg-foreground、checked bg-primary-foreground
    const QColor background = shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background);
    const QColor uncheckedThumb = isDark ? shadTheme->getColor(ShadThemeType::Foreground) : background;
    const QColor checkedThumb = isDark ? shadTheme->getColor(ShadThemeType::PrimaryForeground) : background;
    const qreal travel = kTrackWidth - 2 * kTrackBorder - kThumbSize;
    const QRectF logicalThumbRect(logicalTrackRect.left() + kTrackBorder + travel * progress, logicalTrackRect.top() + kTrackBorder, kThumbSize, kThumbSize);
    fillRoundedRect(&painter, visualRect(direction, bounds, logicalThumbRect), kThumbSize / 2.0, mix(uncheckedThumb, checkedThumb, progress));

    const QString content = text();
    if (!content.isEmpty())
    {
        const qreal textLeft = logicalTrackRect.right() + kTextGap;
        const QRectF textRect = visualRect(direction, bounds, QRectF(textLeft, 0, width() - textLeft - margin, height()));
        const QString elidedText = QFontMetricsF(font()).elidedText(content, Qt::ElideRight, textRect.width());
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elidedText);
    }
}
