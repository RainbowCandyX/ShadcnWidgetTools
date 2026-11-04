#include "ShadTimePicker.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadPickerTrigger.h"
#include "ShadPopover.h"
#include "ShadPopoverPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTimePanel.h"
#include "ShadTimePickerPrivate.h"
#include "ShadTransition.h"

namespace
{
    // 计算理想宽度时使用的示例时间，各位数字取较宽的 2 与 8
    constexpr int kSampleHour = 22;
    constexpr int kSampleMinute = 58;
    constexpr int kSampleSecond = 28;
} // namespace

ShadTimePicker::ShadTimePicker(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTimePickerPrivate())
{
    Q_D(ShadTimePicker);
    d->q_ptr = this;
    ShadStyleHelper::applyDefaultText(this, "placeholderText", d->_placeholderText, tr("选择时间"), [this](const QString& text) { setPlaceholderText(text); });
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_hoverTransition = new ShadTransition(this);

    // 面板自带页脚与分隔线，浮层不再留内边距
    d->_popover = new ShadPopover(this);
    d->_popover->setContentPadding(0);
    d->_panel = new ShadTimePanel(d->_popover);
    d->_popover->contentLayout()->addWidget(d->_panel);
    connect(d->_panel, &ShadTimePanel::confirmed, this, [this, d](const QTime& time)
    {
        hidePopup();
        if (d->_time == time)
        {
            return;
        }
        d->_time = time;
        updateGeometry();
        update();
        Q_EMIT timeChanged(time);
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTimePicker::~ShadTimePicker()
{
    Q_D(ShadTimePicker);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    QObject::disconnect(d->_panel, nullptr, this, nullptr);
}

void ShadTimePicker::setTime(const QTime& time)
{
    Q_D(ShadTimePicker);
    QTime value = time.isValid() ? time : QTime();
    // 隐藏秒时只保留时与分
    if (value.isValid() && !d->_isSecondsVisible)
    {
        value = QTime(value.hour(), value.minute());
    }
    if (d->_time == value)
    {
        return;
    }
    d->_time = value;
    updateGeometry();
    update();
    Q_EMIT timeChanged(value);
}

QTime ShadTimePicker::getTime() const
{
    Q_D(const ShadTimePicker);
    return d->_time;
}

void ShadTimePicker::clear()
{
    setTime(QTime());
}

void ShadTimePicker::setSecondsVisible(bool visible)
{
    Q_D(ShadTimePicker);
    if (d->_isSecondsVisible == visible)
    {
        return;
    }
    d->_isSecondsVisible = visible;
    d->_panel->setSecondsVisible(visible);
    if (!visible && d->_time.isValid() && d->_time.second() != 0)
    {
        setTime(QTime(d->_time.hour(), d->_time.minute()));
    }
    updateGeometry();
    update();
}

bool ShadTimePicker::isSecondsVisible() const
{
    Q_D(const ShadTimePicker);
    return d->_isSecondsVisible;
}

void ShadTimePicker::setPlaceholderText(const QString& text)
{
    Q_D(ShadTimePicker);
    d->_placeholderText = text;
    updateGeometry();
    update();
}

QString ShadTimePicker::getPlaceholderText() const
{
    Q_D(const ShadTimePicker);
    return d->_placeholderText;
}

void ShadTimePicker::setDisplayFormat(const QString& format)
{
    Q_D(ShadTimePicker);
    d->_displayFormat = format;
    updateGeometry();
    update();
}

QString ShadTimePicker::getDisplayFormat() const
{
    Q_D(const ShadTimePicker);
    return d->_displayFormat;
}

QString ShadTimePicker::getDisplayText() const
{
    Q_D(const ShadTimePicker);
    return d->displayText();
}

void ShadTimePicker::setClockIconVisible(bool visible)
{
    Q_D(ShadTimePicker);
    d->_isClockIconVisible = visible;
    updateGeometry();
    update();
}

bool ShadTimePicker::isClockIconVisible() const
{
    Q_D(const ShadTimePicker);
    return d->_isClockIconVisible;
}

void ShadTimePicker::setChevronVisible(bool visible)
{
    Q_D(ShadTimePicker);
    d->_isChevronVisible = visible;
    updateGeometry();
    update();
}

bool ShadTimePicker::isChevronVisible() const
{
    Q_D(const ShadTimePicker);
    return d->_isChevronVisible;
}

void ShadTimePicker::showPopup()
{
    Q_D(ShadTimePicker);
    if (d->_popover->isVisible())
    {
        return;
    }
    // 每次打开都从当前时间开始编辑，关闭时没有确定的修改被丢弃
    d->_panel->setTime(d->_time);
    d->_popover->setLocale(locale());
    d->_popover->setContentWidth(d->_panel->sizeHint().width() + 2 * ShadPopoverPrivate::kBorderWidth);
    d->_popover->popup(this, ShadPlacementType::Bottom, ShadPlacementType::Start);
    // 第一次显示时列的滚动范围要等布局完成后才确定，立即滚动一次并在事件循环中再滚动一次
    d->_panel->scrollToSelection();
    ShadTimePanel* panel = d->_panel;
    QMetaObject::invokeMethod(panel, [panel]() { panel->scrollToSelection(); }, Qt::QueuedConnection);
    d->_panel->focusFirstColumn();
}

void ShadTimePicker::hidePopup()
{
    Q_D(ShadTimePicker);
    d->_popover->hide();
}

bool ShadTimePicker::isPopupVisible() const
{
    Q_D(const ShadTimePicker);
    return d->_popover->isVisible();
}

QSize ShadTimePicker::sizeHint() const
{
    Q_D(const ShadTimePicker);
    const QString sampleText = QTime(kSampleHour, kSampleMinute, kSampleSecond).toString(d->effectiveFormat());
    return ShadPickerTrigger::sizeHint(font(), {d->_placeholderText, sampleText, d->displayText()}, d->_isClockIconVisible, d->_isChevronVisible);
}

QSize ShadTimePicker::minimumSizeHint() const
{
    return ShadPickerTrigger::minimumSizeHint();
}

bool ShadTimePicker::event(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::LocaleChange:
        case QEvent::FontChange:
        case QEvent::LayoutDirectionChange:
        case QEvent::EnabledChange:
            updateGeometry();
            update();
            break;
        case QEvent::LanguageChange:
            ShadStyleHelper::applyDefaultText(this, "placeholderText", getPlaceholderText(), tr("选择时间"), [this](const QString& text) { setPlaceholderText(text); });
            break;
        default:
            break;
    }
    return QWidget::event(event);
}

void ShadTimePicker::enterEvent(QEnterEvent* event)
{
    Q_D(ShadTimePicker);
    d->_hoverTransition->animateTo(1.0);
    QWidget::enterEvent(event);
}

void ShadTimePicker::leaveEvent(QEvent* event)
{
    Q_D(ShadTimePicker);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadTimePicker::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadTimePicker);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadTimePicker::focusOutEvent(QFocusEvent* event)
{
    update();
    QWidget::focusOutEvent(event);
}

void ShadTimePicker::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Down:
            showPopup();
            event->accept();
            return;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
}

void ShadTimePicker::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadTimePicker);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    // 浮层打开时点击触发框由浮层自身关闭，并且这次按下不会传到这里
    d->_isFocusVisible = false;
    setFocus(Qt::MouseFocusReason);
    showPopup();
    event->accept();
}

void ShadTimePicker::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadTimePicker);
    ShadPickerTrigger::Options options;
    options.text = d->displayText();
    options.placeholderText = d->_placeholderText;
    options.leadingIcon = d->_isClockIconVisible ? ShadIconType::Clock : ShadIconType::None;
    options.hasChevron = d->_isChevronVisible;
    options.hoverProgress = d->_hoverTransition->value();
    options.showFocusRing = hasFocus() && d->_isFocusVisible;
    QPainter painter(this);
    ShadPickerTrigger::paint(&painter, this, options);
}
