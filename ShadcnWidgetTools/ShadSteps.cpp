#include "ShadSteps.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadStepsPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSteps::ShadSteps(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadStepsPrivate())
{
    Q_D(ShadSteps);
    d->q_ptr = this;
    setMouseTracking(true);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    ShadStyleHelper::applyFont(this, ShadStepsPrivate::kTextFontSize, QFont::Normal);
    d->updateFocusPolicy();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSteps::~ShadSteps()
{
}

int ShadSteps::addStep(const QString& title, const QString& description)
{
    Q_D(ShadSteps);
    ShadStepsPrivate::Step step;
    step.title = title;
    step.description = description;
    d->_steps.append(step);
    updateGeometry();
    update();
    if (d->_currentIndex < 0)
    {
        setCurrentIndex(0);
    }
    return int(d->_steps.size()) - 1;
}

void ShadSteps::setStepTitle(int index, const QString& title)
{
    Q_D(ShadSteps);
    if (index < 0 || index >= d->_steps.size() || d->_steps.at(index).title == title)
    {
        return;
    }
    d->_steps[index].title = title;
    updateGeometry();
    update();
}

QString ShadSteps::getStepTitle(int index) const
{
    Q_D(const ShadSteps);
    if (index < 0 || index >= d->_steps.size())
    {
        return QString();
    }
    return d->_steps.at(index).title;
}

void ShadSteps::setStepDescription(int index, const QString& description)
{
    Q_D(ShadSteps);
    if (index < 0 || index >= d->_steps.size() || d->_steps.at(index).description == description)
    {
        return;
    }
    d->_steps[index].description = description;
    updateGeometry();
    update();
}

QString ShadSteps::getStepDescription(int index) const
{
    Q_D(const ShadSteps);
    if (index < 0 || index >= d->_steps.size())
    {
        return QString();
    }
    return d->_steps.at(index).description;
}

void ShadSteps::setStepIcon(int index, ShadIconType::IconName icon)
{
    Q_D(ShadSteps);
    if (index < 0 || index >= d->_steps.size())
    {
        return;
    }
    d->_steps[index].icon = icon;
    update();
}

ShadIconType::IconName ShadSteps::getStepIcon(int index) const
{
    Q_D(const ShadSteps);
    if (index < 0 || index >= d->_steps.size())
    {
        return ShadIconType::None;
    }
    return d->_steps.at(index).icon;
}

void ShadSteps::setStepDisabled(int index, bool disabled)
{
    Q_D(ShadSteps);
    if (index < 0 || index >= d->_steps.size())
    {
        return;
    }
    d->_steps[index].isDisabled = disabled;
    update();
}

bool ShadSteps::isStepDisabled(int index) const
{
    Q_D(const ShadSteps);
    return index >= 0 && index < d->_steps.size() && d->_steps.at(index).isDisabled;
}

int ShadSteps::count() const
{
    Q_D(const ShadSteps);
    return int(d->_steps.size());
}

void ShadSteps::clear()
{
    Q_D(ShadSteps);
    d->_steps.clear();
    d->_hoverIndex = -1;
    d->_pressedIndex = -1;
    d->_focusIndex = 0;
    unsetCursor();
    updateGeometry();
    update();
    if (d->_currentIndex != -1)
    {
        d->_currentIndex = -1;
        Q_EMIT currentIndexChanged(-1);
    }
}

void ShadSteps::setCurrentIndex(int index)
{
    Q_D(ShadSteps);
    const int stepCount = int(d->_steps.size());
    index = stepCount == 0 ? -1 : qBound(0, index, stepCount - 1);
    if (d->_currentIndex == index)
    {
        return;
    }
    d->_currentIndex = index;
    if (!hasFocus() || !d->_isFocusVisible)
    {
        d->_focusIndex = qMax(0, index);
    }
    update();
    Q_EMIT currentIndexChanged(index);
}

int ShadSteps::getCurrentIndex() const
{
    Q_D(const ShadSteps);
    return d->_currentIndex;
}

void ShadSteps::setStatus(ShadStepsType::Status status)
{
    Q_D(ShadSteps);
    if (d->_status == status)
    {
        return;
    }
    d->_status = status;
    update();
}

ShadStepsType::Status ShadSteps::getStatus() const
{
    Q_D(const ShadSteps);
    return d->_status;
}

ShadStepsType::Status ShadSteps::getStepStatus(int index) const
{
    Q_D(const ShadSteps);
    if (index < d->_currentIndex)
    {
        return ShadStepsType::Finish;
    }
    if (index == d->_currentIndex)
    {
        return d->_status;
    }
    return ShadStepsType::Wait;
}

void ShadSteps::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadSteps);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    updateGeometry();
    update();
}

Qt::Orientation ShadSteps::getOrientation() const
{
    Q_D(const ShadSteps);
    return d->_orientation;
}

void ShadSteps::setClickable(bool clickable)
{
    Q_D(ShadSteps);
    if (d->_isClickable == clickable)
    {
        return;
    }
    d->_isClickable = clickable;
    d->updateFocusPolicy();
    update();
}

bool ShadSteps::isClickable() const
{
    Q_D(const ShadSteps);
    return d->_isClickable;
}

bool ShadSteps::hasHeightForWidth() const
{
    return true;
}

int ShadSteps::heightForWidth(int width) const
{
    Q_D(const ShadSteps);
    return d->computeLayout(width).height;
}

QSize ShadSteps::sizeHint() const
{
    Q_D(const ShadSteps);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int stepCount = int(d->_steps.size());
    int width = 2 * margin;
    if (d->_orientation == Qt::Horizontal)
    {
        for (int index = 0; index < stepCount; ++index)
        {
            const int textWidth = d->naturalTextWidth(index);
            width += ShadStepsPrivate::kIndicatorSize + (textWidth > 0 ? ShadStepsPrivate::kHorizontalTextGap + textWidth : 0);
        }
        width += qMax(0, stepCount - 1) * (ShadStepsPrivate::kPreferredLineLength + 2 * ShadStepsPrivate::kLineGap);
    }
    else
    {
        int textWidth = 0;
        for (int index = 0; index < stepCount; ++index)
        {
            textWidth = qMax(textWidth, d->naturalTextWidth(index));
        }
        width += ShadStepsPrivate::kIndicatorSize + ShadStepsPrivate::kVerticalTextGap + qMin(textWidth, ShadStepsPrivate::kMaxNaturalTextWidth);
    }
    return QSize(width, heightForWidth(width));
}

QSize ShadSteps::minimumSizeHint() const
{
    Q_D(const ShadSteps);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int stepCount = int(d->_steps.size());
    int width = 2 * margin;
    if (d->_orientation == Qt::Horizontal)
    {
        width += stepCount * ShadStepsPrivate::kIndicatorSize + qMax(0, stepCount - 1) * (ShadStepsPrivate::kMinLineLength + 2 * ShadStepsPrivate::kLineGap);
    }
    else
    {
        width += ShadStepsPrivate::kIndicatorSize + ShadStepsPrivate::kVerticalTextGap + ShadStepsPrivate::kMinTextWidth;
    }
    return QSize(width, 2 * margin + ShadStepsPrivate::kIndicatorSize);
}

void ShadSteps::next()
{
    Q_D(ShadSteps);
    setCurrentIndex(d->_currentIndex + 1);
}

void ShadSteps::previous()
{
    Q_D(ShadSteps);
    setCurrentIndex(qMax(0, d->_currentIndex - 1));
}

bool ShadSteps::event(QEvent* event)
{
    Q_D(ShadSteps);
    if (event->type() == QEvent::Leave && d->_hoverIndex != -1)
    {
        d->_hoverIndex = -1;
        unsetCursor();
        update();
    }
    return QWidget::event(event);
}

void ShadSteps::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
        update();
    }
    else if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadSteps::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadSteps);
    if (d->_isClickable)
    {
        const int index = d->indexAt(event->position().toPoint());
        const int hoverIndex = d->isSelectable(index) ? index : -1;
        if (hoverIndex != d->_hoverIndex)
        {
            d->_hoverIndex = hoverIndex;
            if (hoverIndex >= 0)
            {
                setCursor(Qt::PointingHandCursor);
            }
            else
            {
                unsetCursor();
            }
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void ShadSteps::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSteps);
    if (d->_isClickable && event->button() == Qt::LeftButton)
    {
        d->_pressedIndex = d->indexAt(event->position().toPoint());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadSteps::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSteps);
    if (d->_isClickable && event->button() == Qt::LeftButton)
    {
        const int index = d->indexAt(event->position().toPoint());
        const bool isClick = index >= 0 && index == d->_pressedIndex;
        d->_pressedIndex = -1;
        if (isClick)
        {
            d->activate(index);
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadSteps::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSteps);
    if (!d->_isClickable || d->_steps.isEmpty())
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const int forward = layoutDirection() == Qt::RightToLeft ? -1 : 1;
    switch (event->key())
    {
        case Qt::Key_Left:
            d->moveFocus(-forward);
            break;
        case Qt::Key_Right:
            d->moveFocus(forward);
            break;
        case Qt::Key_Up:
            d->moveFocus(-1);
            break;
        case Qt::Key_Down:
            d->moveFocus(1);
            break;
        case Qt::Key_Home:
            d->_focusIndex = -1;
            d->moveFocus(1);
            break;
        case Qt::Key_End:
            d->_focusIndex = int(d->_steps.size());
            d->moveFocus(-1);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            d->activate(d->_focusIndex);
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    d->_isFocusVisible = true;
    update();
    event->accept();
}

void ShadSteps::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSteps);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason)
    {
        d->_focusIndex = qMax(0, d->_currentIndex);
    }
    update();
    QWidget::focusInEvent(event);
}

void ShadSteps::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadSteps);
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        d->_isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadSteps::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSteps);
    using namespace ShadStyleHelper;
    const ShadStepsPrivate::LayoutResult layout = d->computeLayout(width());
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isRightToLeft = direction == Qt::RightToLeft;
    const QRectF bounds(rect());
    const bool isHorizontal = d->_orientation == Qt::Horizontal;
    const QFont titleFont = d->titleFont();
    const QFont descriptionFont = d->descriptionFont();
    const QFont numberFont = d->numberFont();
    const Qt::Alignment textAlignment = visualAlignment(direction, isHorizontal ? (Qt::AlignLeft | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignTop));
    const int textFlags = int(textAlignment) | (isHorizontal ? 0 : int(Qt::TextWordWrap));
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    for (int index = 0; index < layout.steps.size(); ++index)
    {
        const ShadStepsPrivate::StepLayout& stepLayout = layout.steps.at(index);
        const ShadStepsPrivate::Step& step = d->_steps.at(index);
        const ShadStepsType::Status status = getStepStatus(index);
        painter.setOpacity(isEnabled() && !step.isDisabled ? 1.0 : kDisabledOpacity);

        if (!stepLayout.line.isNull())
        {
            QLineF line = stepLayout.line;
            if (isRightToLeft)
            {
                line = QLineF(width() - line.x1(), line.y1(), width() - line.x2(), line.y2());
            }
            const QColor lineColor = shadTheme->getColor(index < d->_currentIndex ? ShadThemeType::Primary : ShadThemeType::Border);
            QPen linePen(lineColor, ShadStepsPrivate::kLineWidth);
            linePen.setCapStyle(Qt::FlatCap);
            painter.setPen(linePen);
            painter.drawLine(line);
        }

        QColor indicatorColor;
        QColor contentColor;
        switch (status)
        {
            case ShadStepsType::Wait:
                indicatorColor = shadTheme->getColor(ShadThemeType::Muted);
                contentColor = shadTheme->getColor(ShadThemeType::MutedForeground);
                break;
            case ShadStepsType::Process:
            case ShadStepsType::Finish:
                indicatorColor = shadTheme->getColor(ShadThemeType::Primary);
                contentColor = shadTheme->getColor(ShadThemeType::PrimaryForeground);
                break;
            case ShadStepsType::Error:
                indicatorColor = shadTheme->getColor(ShadThemeType::Destructive);
                contentColor = shadTheme->getColor(ShadThemeType::DestructiveForeground);
                break;
        }
        const QRectF indicatorRect = visualRect(direction, bounds, stepLayout.indicator);
        painter.setPen(Qt::NoPen);
        painter.setBrush(indicatorColor);
        painter.drawEllipse(indicatorRect);
        ShadIconType::IconName icon = step.icon;
        if (status == ShadStepsType::Error)
        {
            icon = ShadIconType::X;
        }
        else if (status == ShadStepsType::Finish && icon == ShadIconType::None)
        {
            icon = ShadIconType::Check;
        }
        if (icon != ShadIconType::None)
        {
            const qreal iconOffset = (ShadStepsPrivate::kIndicatorSize - ShadStepsPrivate::kIconSize) / 2.0;
            ShadIcon::paint(&painter, indicatorRect.adjusted(iconOffset, iconOffset, -iconOffset, -iconOffset), icon, contentColor, ShadStepsPrivate::kIconSize);
        }
        else
        {
            painter.setFont(numberFont);
            painter.setPen(contentColor);
            painter.drawText(indicatorRect, Qt::AlignCenter, QString::number(index + 1));
        }
        if (hasFocus() && d->_isFocusVisible && index == d->_focusIndex)
        {
            drawFocusRing(&painter, indicatorRect, indicatorRect.height() / 2.0, focusRingColor(false));
        }

        QColor titleColor = shadTheme->getColor(ShadThemeType::Foreground);
        if (status == ShadStepsType::Error)
        {
            titleColor = shadTheme->getColor(ShadThemeType::Destructive);
        }
        else if (status == ShadStepsType::Wait && index != d->_hoverIndex)
        {
            titleColor = shadTheme->getColor(ShadThemeType::MutedForeground);
        }
        if (!stepLayout.titleText.isEmpty())
        {
            painter.setFont(titleFont);
            painter.setPen(titleColor);
            painter.drawText(visualRect(direction, bounds, stepLayout.title), textFlags, stepLayout.titleText);
        }
        if (!stepLayout.descriptionText.isEmpty())
        {
            painter.setFont(descriptionFont);
            painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
            painter.drawText(visualRect(direction, bounds, stepLayout.description), textFlags, stepLayout.descriptionText);
        }
    }
}
