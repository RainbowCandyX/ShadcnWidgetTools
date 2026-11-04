#include "ShadRadioButton.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtMath>

#include "ShadRadioButtonPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // size-4 圆形，选中指示点 size-2
    constexpr int kCircleSize = 16;
    constexpr qreal kDotSize = 8.0;
    constexpr int kTextGap = 8;
} // namespace

ShadRadioButton::ShadRadioButton(QWidget* parent)
    : QRadioButton(parent), d_ptr(new ShadRadioButtonPrivate())
{
    Q_D(ShadRadioButton);
    d->q_ptr = this;
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    // QRadioButton 默认开启该属性，macOS 样式会在获得焦点时额外绘制系统焦点框；焦点环由组件自绘
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    // QRadioButton 会从样式读取布局项边距（macOS 上不为 0），组件自绘且 sizeHint 已包含焦点环边距，直接使用控件矩形
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadRadioButton::ShadRadioButton(const QString& text, QWidget* parent)
    : ShadRadioButton(parent)
{
    setText(text);
}

ShadRadioButton::~ShadRadioButton()
{
}

QSize ShadRadioButton::sizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const QFontMetricsF fontMetrics(font());
    const QString content = text();
    const int textWidth = content.isEmpty() ? 0 : kTextGap + qCeil(fontMetrics.horizontalAdvance(content));
    const int height = qMax(kCircleSize, qCeil(fontMetrics.height()));
    return QSize(kCircleSize + textWidth + margin, height + margin);
}

QSize ShadRadioButton::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadRadioButton::hitButton(const QPoint& pos) const
{
    return rect().contains(pos);
}

void ShadRadioButton::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadRadioButton);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QRadioButton::focusInEvent(event);
    update();
}

void ShadRadioButton::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadRadioButton);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QRadioButton::focusOutEvent(event);
    update();
}

void ShadRadioButton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QRadioButton::changeEvent(event);
}

void ShadRadioButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadRadioButton);
    using namespace ShadStyleHelper;
    const int margin = kFocusRingWidth;
    // RTL 时圆框位于右侧
    const QRectF circleRect = visualRect(layoutDirection(), QRectF(rect()), QRectF(margin, (height() - kCircleSize) / 2, kCircleSize, kCircleSize));
    const qreal radius = kCircleSize / 2.0;
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // border-input dark:bg-input/30 shadow-xs，选中时中心为 fill-primary 圆点
    drawShadowXs(&painter, circleRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, circleRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    strokeRoundedRect(&painter, circleRect, radius, shadTheme->getColor(showFocusRing ? ShadThemeType::Ring : ShadThemeType::Input));
    if (showFocusRing)
    {
        drawFocusRing(&painter, circleRect, radius, focusRingColor(false));
    }
    if (isChecked())
    {
        const QPointF center = circleRect.center();
        const QRectF dotRect(center.x() - kDotSize / 2.0, center.y() - kDotSize / 2.0, kDotSize, kDotSize);
        fillRoundedRect(&painter, dotRect, kDotSize / 2.0, shadTheme->getColor(ShadThemeType::Primary));
    }

    const QString content = text();
    if (!content.isEmpty())
    {
        // 按 LTR 计算圆框右侧的文字区域，RTL 时镜像到圆框左侧并靠右对齐
        const qreal textLeft = margin + kCircleSize + kTextGap;
        const QRectF textRect = visualRect(layoutDirection(), QRectF(rect()), QRectF(textLeft, 0, width() - textLeft - margin, height()));
        const QString elidedText = QFontMetricsF(font()).elidedText(content, Qt::ElideRight, textRect.width());
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(textRect, visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), elidedText);
    }
}
