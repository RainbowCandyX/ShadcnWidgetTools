#include "ShadTrace.h"

#include <QColor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>
#include <QtMath>

#include "ShadIcon.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTracePrivate.h"
#include "ShadTransition.h"

namespace
{
    // 绘制图标：rotation 为绕中心顺时针旋转的角度，mirrored 为水平翻转（先翻转再旋转）
    void paintIcon(QPainter* painter, const QRectF& rect, ShadIconType::IconName icon, const QColor& color, qreal rotation, bool mirrored)
    {
        painter->save();
        const QPointF center = rect.center();
        painter->translate(center);
        painter->rotate(rotation);
        if (mirrored)
        {
            painter->scale(-1.0, 1.0);
        }
        painter->translate(-center);
        ShadIcon::paint(painter, rect, icon, color, ShadTraceLinePrivate::kIconSize);
        painter->restore();
    }
} // namespace

ShadTraceLine::ShadTraceLine(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTraceLinePrivate())
{
    Q_D(ShadTraceLine);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    d->_hoverTransition = new ShadTransition(this);

    d->_spinAnimation = new QVariantAnimation(this);
    d->_spinAnimation->setStartValue(0.0);
    d->_spinAnimation->setEndValue(360.0);
    d->_spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
    d->_spinAnimation->setLoopCount(-1);
    d->_spinAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_spinAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_spinAngle = value.toReal();
        update();
    });

    d->_shimmerAnimation = new QVariantAnimation(this);
    d->_shimmerAnimation->setStartValue(0.0);
    d->_shimmerAnimation->setEndValue(1.0);
    d->_shimmerAnimation->setDuration(ShadStyleHelper::kShimmerDurationMs);
    d->_shimmerAnimation->setLoopCount(-1);
    d->_shimmerAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_shimmerAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_shimmerProgress = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTraceLine::ShadTraceLine(ShadIconType::IconName icon, const QString& title, QWidget* parent)
    : ShadTraceLine(parent)
{
    Q_D(ShadTraceLine);
    d->_icon = icon;
    d->_title = title;
}

ShadTraceLine::~ShadTraceLine()
{
}

void ShadTraceLine::setIcon(ShadIconType::IconName icon)
{
    Q_D(ShadTraceLine);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateAnimations();
    updateGeometry();
    update();
}

ShadIconType::IconName ShadTraceLine::getIcon() const
{
    Q_D(const ShadTraceLine);
    return d->_icon;
}

void ShadTraceLine::setTitle(const QString& title)
{
    Q_D(ShadTraceLine);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    d->updateAnimations();
    updateGeometry();
    update();
}

QString ShadTraceLine::getTitle() const
{
    Q_D(const ShadTraceLine);
    return d->_title;
}

void ShadTraceLine::setDescription(const QString& description)
{
    Q_D(ShadTraceLine);
    if (d->_description == description)
    {
        return;
    }
    d->_description = description;
    updateGeometry();
    update();
}

QString ShadTraceLine::getDescription() const
{
    Q_D(const ShadTraceLine);
    return d->_description;
}

void ShadTraceLine::setDescriptionMonospace(bool monospace)
{
    Q_D(ShadTraceLine);
    if (d->_isDescriptionMonospace == monospace)
    {
        return;
    }
    d->_isDescriptionMonospace = monospace;
    updateGeometry();
    update();
}

bool ShadTraceLine::isDescriptionMonospace() const
{
    Q_D(const ShadTraceLine);
    return d->_isDescriptionMonospace;
}

void ShadTraceLine::setIndicatorIcon(ShadIconType::IconName icon)
{
    Q_D(ShadTraceLine);
    if (d->_indicatorIcon == icon)
    {
        return;
    }
    d->_indicatorIcon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadTraceLine::getIndicatorIcon() const
{
    Q_D(const ShadTraceLine);
    return d->_indicatorIcon;
}

void ShadTraceLine::setStatus(ShadTraceType::Status status)
{
    Q_D(ShadTraceLine);
    if (d->_status == status)
    {
        return;
    }
    d->_status = status;
    d->updateAnimations();
    update();
}

ShadTraceType::Status ShadTraceLine::getStatus() const
{
    Q_D(const ShadTraceLine);
    return d->_status;
}

void ShadTraceLine::setClickable(bool clickable)
{
    Q_D(ShadTraceLine);
    if (d->_isClickable == clickable)
    {
        return;
    }
    d->_isClickable = clickable;
    // 可点击时对应 button / a：可通过 Tab 聚焦，鼠标为手形
    setFocusPolicy(clickable ? Qt::StrongFocus : Qt::NoFocus);
    if (clickable)
    {
        setCursor(Qt::PointingHandCursor);
        d->_hoverTransition->jumpTo(underMouse() ? 1.0 : 0.0);
    }
    else
    {
        unsetCursor();
        d->_isPressed = false;
        d->_hoverTransition->jumpTo(0.0);
    }
    update();
}

bool ShadTraceLine::isClickable() const
{
    Q_D(const ShadTraceLine);
    return d->_isClickable;
}

void ShadTraceLine::setUrl(const QUrl& url)
{
    Q_D(ShadTraceLine);
    d->_url = url;
    if (!url.isEmpty())
    {
        setClickable(true);
    }
}

QUrl ShadTraceLine::getUrl() const
{
    Q_D(const ShadTraceLine);
    return d->_url;
}

QSize ShadTraceLine::sizeHint() const
{
    Q_D(const ShadTraceLine);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const qreal width = d->contentWidth(ShadStyleHelper::textWidth(d->titleFont(), d->_title));
    return QSize(qCeil(width) + margin, ShadTraceLinePrivate::kLineHeight + margin);
}

QSize ShadTraceLine::minimumSizeHint() const
{
    Q_D(const ShadTraceLine);
    // 标题可以压缩到 0，图标与描述保持完整
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(qCeil(d->contentWidth(0.0)) + margin, ShadTraceLinePrivate::kLineHeight + margin);
}

void ShadTraceLine::enterEvent(QEnterEvent* event)
{
    Q_D(ShadTraceLine);
    if (d->_isClickable)
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadTraceLine::leaveEvent(QEvent* event)
{
    Q_D(ShadTraceLine);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadTraceLine::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadTraceLine);
    if (!d->_isClickable || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_isPressed = true;
    event->accept();
}

void ShadTraceLine::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadTraceLine);
    if (!d->_isPressed || event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    d->_isPressed = false;
    event->accept();
    if (rect().contains(event->position().toPoint()))
    {
        d->activate();
    }
}

void ShadTraceLine::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadTraceLine);
    const int key = event->key();
    if (d->_isClickable && event->modifiers() == Qt::NoModifier && (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter))
    {
        event->accept();
        d->activate();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadTraceLine::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadTraceLine);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadTraceLine::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadTraceLine);
    // 弹出浮层等原因失去焦点时保持状态，焦点回来后继续显示焦点环
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        d->_isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadTraceLine::showEvent(QShowEvent* event)
{
    Q_D(ShadTraceLine);
    QWidget::showEvent(event);
    d->updateAnimations();
}

void ShadTraceLine::hideEvent(QHideEvent* event)
{
    Q_D(ShadTraceLine);
    QWidget::hideEvent(event);
    d->updateAnimations();
}

void ShadTraceLine::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadTraceLine::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadTraceLine);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isRightToLeft = direction == Qt::RightToLeft;
    const QRectF bounds(rect());
    const ShadTraceLinePrivate::Layout layout = d->computeLayout();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // 焦点环：rounded-md ring-3 ring-ring/50，不受 pending 的透明度影响
    if (hasFocus() && d->_isFocusVisible)
    {
        drawFocusRing(&painter, insetRect(rect(), kFocusRingWidth), shadTheme->getRadius(ShadThemeType::Medium), focusRingColor(false));
    }
    qreal opacity = isEnabled() ? 1.0 : kDisabledOpacity;
    if (d->_status == ShadTraceType::Pending)
    {
        opacity *= ShadTraceLinePrivate::kPendingOpacity;
    }
    painter.setOpacity(opacity);
    const QColor color = d->lineColor();
    if (!layout.icon.isEmpty())
    {
        paintIcon(&painter, visualRect(direction, bounds, layout.icon), d->_icon, color, d->_spinAngle, false);
    }
    if (!layout.title.isEmpty() && !layout.elidedTitle.isEmpty())
    {
        const QRectF titleRect = visualRect(direction, bounds, layout.title);
        const QFont font = d->titleFont();
        if (d->_status == ShadTraceType::Running)
        {
            drawShimmerText(&painter, titleRect, layout.elidedTitle, font, color, d->_shimmerProgress, Qt::AlignLeft | Qt::AlignVCenter, direction);
        }
        else
        {
            painter.setFont(font);
            painter.setPen(color);
            painter.drawText(titleRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), layout.elidedTitle);
        }
    }
    if (!layout.description.isEmpty())
    {
        painter.setFont(d->descriptionFont());
        painter.setPen(withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), ShadTraceLinePrivate::kDescriptionAlpha));
        painter.drawText(visualRect(direction, bounds, layout.description), visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), d->_description);
    }
    // 标题后的指示图标与箭头都有方向性，RTL 下水平翻转
    if (!layout.indicator.isEmpty())
    {
        paintIcon(&painter, visualRect(direction, bounds, layout.indicator), d->_indicatorIcon, color, 0.0, isRightToLeft);
    }
    if (!layout.chevron.isEmpty())
    {
        // 展开时 ChevronRight 顺时针旋转 90 度朝下；RTL 下翻转为朝左，逆时针旋转 90 度朝下
        const qreal rotation = (isRightToLeft ? -90.0 : 90.0) * d->_chevronProgress;
        paintIcon(&painter, visualRect(direction, bounds, layout.chevron), ShadIconType::ChevronRight, color, rotation, isRightToLeft);
    }
}

ShadTraceCollapsible::ShadTraceCollapsible(QWidget* parent)
    : ShadTraceCollapsible(ShadIconType::None, QString(), parent)
{
}

ShadTraceCollapsible::ShadTraceCollapsible(ShadIconType::IconName icon, const QString& title, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTraceCollapsiblePrivate())
{
    Q_D(ShadTraceCollapsible);
    d->q_ptr = this;
    // TraceCollapsible 占满宽度，高度由触发行与展开进度决定
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    d->createTrigger(icon, title);
    connect(d->_trigger, &ShadTraceLine::clicked, this, [this]() { setExpanded(!isExpanded()); });

    d->_content = new QWidget(this);
    d->_contentLayout = new QVBoxLayout(d->_content);
    // gap-2，扣除相邻组件上下的焦点环边距
    d->_contentLayout->setSpacing(ShadTraceCollapsiblePrivate::kContentSpacing - 2 * ShadStyleHelper::kFocusRingWidth);
    d->applyContentMargins();
    d->_content->hide();
    // 详情区添加、移除组件或组件尺寸变化时收到 LayoutRequest
    d->_content->installEventFilter(this);

    d->_animation = new QVariantAnimation(this);
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
    d->_animation->setEasingCurve(easingCurve);
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->setProgress(value.toReal()); });
    connect(d->_animation, &QVariantAnimation::finished, this, [d]()
    {
        // 折叠完成后隐藏详情区，键盘焦点不会进入折叠的内容
        if (!d->_isExpanded)
        {
            ShadStyleHelper::clearFocusWithin(d->_content);
            d->_content->hide();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->updateLayout();
}

ShadTraceCollapsible::~ShadTraceCollapsible()
{
}

ShadTraceLine* ShadTraceCollapsible::getTriggerLine() const
{
    Q_D(const ShadTraceCollapsible);
    return d->_trigger;
}

void ShadTraceCollapsible::setExpanded(bool expanded)
{
    Q_D(ShadTraceCollapsible);
    if (d->_isExpanded == expanded)
    {
        return;
    }
    d->_isExpanded = expanded;
    const qreal target = expanded ? 1.0 : 0.0;
    if (expanded)
    {
        d->_content->show();
    }
    d->_animation->stop();
    const qreal distance = qAbs(target - d->_progress);
    if (isVisible() && distance > 0.0)
    {
        // 中途反向时按剩余距离缩短时长
        d->_animation->setDuration(qMax(1, qRound(ShadTraceCollapsiblePrivate::kDurationMs * distance)));
        d->_animation->setStartValue(d->_progress);
        d->_animation->setEndValue(target);
        d->_animation->start();
    }
    else
    {
        d->setProgress(target);
        if (!expanded)
        {
            ShadStyleHelper::clearFocusWithin(d->_content);
            d->_content->hide();
        }
    }
    Q_EMIT expandedChanged(expanded);
}

bool ShadTraceCollapsible::isExpanded() const
{
    Q_D(const ShadTraceCollapsible);
    return d->_isExpanded;
}

QVBoxLayout* ShadTraceCollapsible::contentLayout() const
{
    Q_D(const ShadTraceCollapsible);
    return d->_contentLayout;
}

ShadLabel* ShadTraceCollapsible::addText(const QString& text)
{
    Q_D(ShadTraceCollapsible);
    ShadLabel* label = new ShadLabel(text, d->_content);
    label->setColorRole(ShadThemeType::MutedForeground);
    label->setWordWrap(true);
    // 与其他组件一致地四周预留焦点环边距，可视边缘对齐
    const int margin = ShadStyleHelper::kFocusRingWidth;
    label->setContentsMargins(margin, margin, margin, margin);
    d->_contentLayout->addWidget(label);
    return label;
}

QSize ShadTraceCollapsible::sizeHint() const
{
    Q_D(const ShadTraceCollapsible);
    int width = d->_trigger->sizeHint().width();
    if (d->_progress > 0.0)
    {
        width = qMax(width, d->_contentLayout->sizeHint().width());
    }
    return QSize(width, d->_preferredHeight);
}

QSize ShadTraceCollapsible::minimumSizeHint() const
{
    Q_D(const ShadTraceCollapsible);
    return QSize(d->_trigger->minimumSizeHint().width(), d->_preferredHeight);
}

bool ShadTraceCollapsible::event(QEvent* event)
{
    Q_D(ShadTraceCollapsible);
    if (event->type() == QEvent::LayoutRequest)
    {
        // 触发行的尺寸变化
        d->updateLayout();
    }
    return QWidget::event(event);
}

bool ShadTraceCollapsible::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadTraceCollapsible);
    if (watched == d->_content && event->type() == QEvent::LayoutRequest)
    {
        d->updateLayout();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadTraceCollapsible::changeEvent(QEvent* event)
{
    Q_D(ShadTraceCollapsible);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        // 详情区的缩进与竖线移到起始侧
        d->applyContentMargins();
        d->updateLayout();
    }
    QWidget::changeEvent(event);
}

void ShadTraceCollapsible::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadTraceCollapsible);
    QWidget::resizeEvent(event);
    d->updateLayout();
}

void ShadTraceCollapsible::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadTraceCollapsible);
    if (d->_progress <= 0.0)
    {
        return;
    }
    using ShadStyleHelper::kFocusRingWidth;
    // 详情区起始侧的 border-l：从 mt-2 之下到最后一个组件的可视底边
    const int triggerHeight = d->_trigger->height();
    const int top = triggerHeight - kFocusRingWidth + ShadTraceCollapsiblePrivate::kContentTop;
    const int bottom = d->_content->geometry().bottom() + 1 - ShadTraceCollapsiblePrivate::kContentBottom;
    if (bottom <= top)
    {
        return;
    }
    const QRect logicalRect(kFocusRingWidth + ShadTraceCollapsiblePrivate::kContentIndent, top, ShadTraceCollapsiblePrivate::kBorderWidth, bottom - top);
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    painter.fillRect(ShadStyleHelper::visualRect(layoutDirection(), rect(), logicalRect), shadTheme->getColor(ShadThemeType::Border));
}

ShadTrace::ShadTrace(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTracePrivate())
{
    Q_D(ShadTrace);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    // gap-2，扣除相邻行上下的焦点环边距
    d->_layout->setSpacing(ShadTracePrivate::kSpacing - 2 * ShadStyleHelper::kFocusRingWidth);
}

ShadTrace::~ShadTrace()
{
}

ShadTraceLine* ShadTrace::addLine(ShadIconType::IconName icon, const QString& title)
{
    ShadTraceLine * line = new ShadTraceLine(icon, title, this);
    addWidget(line);
    return line;
}

ShadTraceCollapsible* ShadTrace::addCollapsible(ShadIconType::IconName icon, const QString& title)
{
    ShadTraceCollapsible * collapsible = new ShadTraceCollapsible(icon, title, this);
    addWidget(collapsible);
    return collapsible;
}

void ShadTrace::addWidget(QWidget* widget)
{
    Q_D(ShadTrace);
    if (widget == nullptr)
    {
        return;
    }
    // 行按内容宽度起始侧对齐（w-fit），可展开的行与其他组件占满宽度
    const bool isLine = qobject_cast<ShadTraceLine*>(widget) != nullptr;
    d->_layout->addWidget(widget, 0, isLine ? Qt::AlignLeft : Qt::Alignment());
}

void ShadTrace::clear()
{
    Q_D(ShadTrace);
    while (QLayoutItem* item = d->_layout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
}

int ShadTrace::count() const
{
    Q_D(const ShadTrace);
    return d->_layout->count();
}
