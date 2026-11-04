#include "ShadField.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadFieldPrivate.h"
#include "ShadLabel.h"
#include "ShadMaxWidthBoxLayout.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadField::ShadField(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadFieldPrivate())
{
    Q_D(ShadField);
    d->q_ptr = this;
    // 设置最大宽度后，自动换行的说明文字按实际宽度计算高度
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setSpacing(0);
    d->_content = new QWidget(this);
    d->_contentLayout = new QVBoxLayout(d->_content);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(0);
    // FieldLabel：text-sm font-medium leading-snug
    d->_label = new ShadLabel(this);
    d->_label->setWordWrap(true);
    // FieldDescription：text-sm text-muted-foreground font-normal
    d->_description = new ShadLabel(this);
    d->_description->setColorRole(ShadThemeType::MutedForeground);
    d->_description->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_description, 14, QFont::Normal);
    // FieldError：text-sm text-destructive font-normal
    d->_error = new ShadLabel(this);
    d->_error->setColorRole(ShadThemeType::Destructive);
    d->_error->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_error, 14, QFont::Normal);
    d->applyMargins();
    d->rebuildLayout();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadField::ShadField(const QString& label, QWidget* control, QWidget* parent)
    : ShadField(parent)
{
    setLabel(label);
    setControl(control);
}

ShadField::~ShadField()
{
    Q_D(ShadField);
    // 私有数据先于子组件释放，断开捕获 this 的连接
    QObject::disconnect(d->_toggledConnection);
    if (!d->_control.isNull())
    {
        d->_control->removeEventFilter(this);
    }
}

void ShadField::setLabel(const QString& label)
{
    Q_D(ShadField);
    d->_label->setText(label);
    d->rebuildLayout();
}

QString ShadField::getLabel() const
{
    Q_D(const ShadField);
    return d->_label->text();
}

ShadLabel* ShadField::labelWidget() const
{
    Q_D(const ShadField);
    return d->_label;
}

void ShadField::setControl(QWidget* control)
{
    Q_D(ShadField);
    if (d->_control.data() == control)
    {
        return;
    }
    if (!d->_control.isNull())
    {
        QWidget* oldControl = d->_control.data();
        QObject::disconnect(d->_toggledConnection);
        oldControl->removeEventFilter(this);
        d->_layout->removeWidget(oldControl);
        ShadStyleHelper::clearFocusWithin(oldControl);
        oldControl->hide();
        oldControl->deleteLater();
    }
    d->_control = control;
    d->_isControlFocusVisible = false;
    if (control != nullptr)
    {
        control->setParent(this);
        control->installEventFilter(this);
        // 选择卡片随控件的选中状态变化
        if (QAbstractButton* button = qobject_cast<QAbstractButton*>(control))
        {
            d->_toggledConnection = connect(button, &QAbstractButton::toggled, this, [this]() { update(); });
        }
    }
    d->_label->setBuddy(control);
    d->applyInvalid();
    d->rebuildLayout();
    update();
}

QWidget* ShadField::getControl() const
{
    Q_D(const ShadField);
    return d->_control.data();
}

void ShadField::setDescription(const QString& description)
{
    Q_D(ShadField);
    d->_description->setText(description);
    d->rebuildLayout();
}

QString ShadField::getDescription() const
{
    Q_D(const ShadField);
    return d->_description->text();
}

void ShadField::setErrors(const QStringList& errors)
{
    Q_D(ShadField);
    d->_errors = errors;
    d->updateErrorText();
    d->rebuildLayout();
}

QStringList ShadField::getErrors() const
{
    Q_D(const ShadField);
    return d->_errors;
}

void ShadField::setError(const QString& error)
{
    setErrors(error.isEmpty() ? QStringList() : QStringList{error});
}

void ShadField::clearErrors()
{
    setErrors(QStringList());
}

void ShadField::setInvalid(bool invalid)
{
    Q_D(ShadField);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    d->applyInvalid();
    Q_EMIT invalidChanged();
}

bool ShadField::isInvalid() const
{
    Q_D(const ShadField);
    return d->_isInvalid;
}

void ShadField::setOrientation(ShadFieldType::Orientation orientation)
{
    Q_D(ShadField);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    d->rebuildLayout();
}

ShadFieldType::Orientation ShadField::getOrientation() const
{
    Q_D(const ShadField);
    return d->_orientation;
}

void ShadField::setControlPosition(ShadFieldType::ControlPosition position)
{
    Q_D(ShadField);
    if (d->_controlPosition == position)
    {
        return;
    }
    d->_controlPosition = position;
    d->rebuildLayout();
}

ShadFieldType::ControlPosition ShadField::getControlPosition() const
{
    Q_D(const ShadField);
    return d->_controlPosition;
}

void ShadField::setChoiceCard(bool choiceCard)
{
    Q_D(ShadField);
    if (d->_isChoiceCard == choiceCard)
    {
        return;
    }
    d->_isChoiceCard = choiceCard;
    d->applyMargins();
    setCursor(choiceCard ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

bool ShadField::isChoiceCard() const
{
    Q_D(const ShadField);
    return d->_isChoiceCard;
}

bool ShadField::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadField);
    if (watched == d->_control.data())
    {
        switch (event->type())
        {
            case QEvent::FocusIn:
                {
                    d->_isControlFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isControlFocusVisible, static_cast<QFocusEvent*>(event));
                    update();
                    break;
                }
            case QEvent::FocusOut:
                {
                    d->_isControlFocusVisible = false;
                    update();
                    break;
                }
            case QEvent::EnabledChange:
                {
                    update();
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadField::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadField);
    QWidget::resizeEvent(event);
    // Responsive 宽度跨过阈值时切换横排与竖排
    if (d->_orientation == ShadFieldType::Responsive && d->isHorizontal() != d->_isLaidOutHorizontal)
    {
        d->rebuildLayout();
    }
}

void ShadField::enterEvent(QEnterEvent* event)
{
    Q_D(ShadField);
    d->_isHovered = true;
    if (d->_isChoiceCard)
    {
        update();
    }
    QWidget::enterEvent(event);
}

void ShadField::leaveEvent(QEvent* event)
{
    Q_D(ShadField);
    d->_isHovered = false;
    if (d->_isChoiceCard)
    {
        update();
    }
    QWidget::leaveEvent(event);
}

void ShadField::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadField);
    // 选择卡片接受按下，松开时才会交给本组件
    if (d->_isChoiceCard && event->button() == Qt::LeftButton)
    {
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadField::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadField);
    QAbstractButton* button = qobject_cast<QAbstractButton*>(d->_control.data());
    const QPoint pos = event->position().toPoint();
    // 点击卡片任意位置切换控件；点击标签时标签已经切换过控件（label for），不再重复切换
    if (d->_isChoiceCard && event->button() == Qt::LeftButton && button != nullptr && button->isEnabled() && rect().contains(pos) && childAt(pos) != d->_label)
    {
        button->click();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadField::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadField);
    if (!d->_isChoiceCard)
    {
        return;
    }
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isDark = shadTheme->isDark();
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    const bool isControlEnabled = !d->_control.isNull() && d->_control->isEnabled();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    // has-data-checked:bg-primary/5 border-primary/30（暗色 bg-primary/10 border-primary/20）
    if (d->isControlChecked())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(primary, isDark ? 0.1 : 0.05));
        borderColor = withAlpha(primary, isDark ? 0.2 : 0.3);
    }
    else if (d->_isHovered && isControlEnabled)
    {
        // not-has-[:disabled]:hover:bg-muted/50
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Muted), 0.5));
    }
    // has-[:focus-visible]:border-ring ring-ring/50 ring-3
    const bool isFocusVisible = d->_isControlFocusVisible && !d->_control.isNull() && d->_control->hasFocus();
    if (isFocusVisible)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Ring);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (isFocusVisible)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false));
    }
}
