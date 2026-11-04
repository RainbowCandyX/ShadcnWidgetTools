#include "ShadCheckbox.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtMath>

#include "ShadCheckboxPrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // size-4 方框，rounded-[4px]
    constexpr int kBoxSize = 16;
    constexpr qreal kBoxRadius = 4.0;
    // 勾选图标 size-3.5
    constexpr int kCheckIconSize = 14;
    // 方框与文字间距 gap-2
    constexpr int kTextGap = 8;
} // namespace

ShadCheckbox::ShadCheckbox(QWidget* parent)
    : QCheckBox(parent), d_ptr(new ShadCheckboxPrivate())
{
    Q_D(ShadCheckbox);
    d->q_ptr = this;
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    // QCheckBox 默认开启该属性，macOS 样式会在获得焦点时额外绘制系统焦点框；焦点环由组件自绘
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    // QCheckBox 会从样式读取布局项边距（macOS 上不为 0），组件自绘且 sizeHint 已包含焦点环边距，直接使用控件矩形
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadCheckbox::ShadCheckbox(const QString& text, QWidget* parent)
    : ShadCheckbox(parent)
{
    setText(text);
}

ShadCheckbox::~ShadCheckbox()
{
}

void ShadCheckbox::setInvalid(bool invalid)
{
    Q_D(ShadCheckbox);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadCheckbox::isInvalid() const
{
    Q_D(const ShadCheckbox);
    return d->_isInvalid;
}

QSize ShadCheckbox::sizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const QFontMetricsF fontMetrics(font());
    const QString content = text();
    const int textWidth = content.isEmpty() ? 0 : kTextGap + qCeil(fontMetrics.horizontalAdvance(content));
    const int height = qMax(kBoxSize, qCeil(fontMetrics.height()));
    return QSize(kBoxSize + textWidth + margin, height + margin);
}

QSize ShadCheckbox::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadCheckbox::hitButton(const QPoint& pos) const
{
    // 整个组件（含文字）都可点击，与 label 包裹 checkbox 的行为一致
    return rect().contains(pos);
}

void ShadCheckbox::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadCheckbox);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QCheckBox::focusInEvent(event);
    update();
}

void ShadCheckbox::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadCheckbox);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QCheckBox::focusOutEvent(event);
    update();
}

void ShadCheckbox::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QCheckBox::changeEvent(event);
}

void ShadCheckbox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCheckbox);
    using namespace ShadStyleHelper;
    const int margin = kFocusRingWidth;
    // 使用整数坐标避免方框边缘发虚；RTL 时方框位于右侧
    const QRectF boxRect = visualRect(layoutDirection(), QRectF(rect()), QRectF(margin, (height() - kBoxSize) / 2, kBoxSize, kBoxSize));
    const Qt::CheckState state = checkState();
    const bool isMarked = state != Qt::Unchecked;
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // border-input dark:bg-input/30 shadow-xs；选中后 bg-primary border-primary
    drawShadowXs(&painter, boxRect, kBoxRadius);
    QColor background(0, 0, 0, 0);
    QColor borderColor = shadTheme->getColor(ShadThemeType::Input);
    if (isMarked)
    {
        background = shadTheme->getColor(ShadThemeType::Primary);
        borderColor = background;
    }
    else if (shadTheme->isDark())
    {
        background = withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3);
    }
    if (showFocusRing)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Ring);
    }
    if (d->_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    fillRoundedRect(&painter, boxRect, kBoxRadius, background);
    strokeRoundedRect(&painter, boxRect, kBoxRadius, borderColor);
    if (showFocusRing || d->_isInvalid)
    {
        drawFocusRing(&painter, boxRect, kBoxRadius, focusRingColor(d->_isInvalid));
    }
    if (isMarked)
    {
        const ShadIconType::IconName markIcon = state == Qt::PartiallyChecked ? ShadIconType::Minus : ShadIconType::Check;
        ShadIcon::paint(&painter, boxRect, markIcon, shadTheme->getColor(ShadThemeType::PrimaryForeground), kCheckIconSize);
    }

    const QString content = text();
    if (!content.isEmpty())
    {
        // 按 LTR 计算方框右侧的文字区域，RTL 时镜像到方框左侧并靠右对齐
        const qreal textLeft = margin + kBoxSize + kTextGap;
        const QRectF textRect = visualRect(layoutDirection(), QRectF(rect()), QRectF(textLeft, 0, width() - textLeft - margin, height()));
        const QString elidedText = QFontMetricsF(font()).elidedText(content, Qt::ElideRight, textRect.width());
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(textRect, visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), elidedText);
    }
}
