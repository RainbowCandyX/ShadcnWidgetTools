#include "ShadTooltip.h"

#include <QApplication>
#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QWidget>

#include "ShadKbd.h"
#include "ShadStyleHelper.h"
#include "ShadTooltipPrivate.h"
#include "ShadTooltipWindow.h"

namespace
{
    constexpr const char* kSideProperty = "_shadTooltipSide";
    constexpr const char* kShortcutProperty = "_shadTooltipShortcut";
    constexpr const char* kShowDelayProperty = "_shadTooltipShowDelay";
} // namespace

ShadTooltipPrivate::ShadTooltipPrivate()
{
}

ShadTooltipPrivate::~ShadTooltipPrivate()
{
}

ShadTooltipWindow* ShadTooltipPrivate::window()
{
    if (_window.isNull())
    {
        _window = new ShadTooltipWindow();
    }
    return _window.data();
}

void ShadTooltipPrivate::showFor(QWidget* widget, const QRect& rect, const QString& text, const QStringList& keys)
{
    if (widget == nullptr || text.isEmpty() || !widget->isVisible())
    {
        hide();
        return;
    }
    _target = widget;
    _targetRect = rect;
    const QRect anchor(widget->mapToGlobal(rect.topLeft()), rect.size());
    window()->showFor(anchor, text, keys, ShadTooltip::getSide(widget), widget->layoutDirection());
}

void ShadTooltipPrivate::hide()
{
    _showTimer->stop();
    _pendingWidget = nullptr;
    _target = nullptr;
    if (!_window.isNull())
    {
        _window->hideAnimated();
    }
}

void ShadTooltipPrivate::requestAt(const QPoint& globalPos)
{
    QWidget* widget = QApplication::widgetAt(globalPos);
    while (widget != nullptr)
    {
        QHelpEvent event(QEvent::ToolTip, widget->mapFromGlobal(globalPos), globalPos);
        QCoreApplication::sendEvent(widget, &event);
        if (event.isAccepted() || widget->isWindow())
        {
            return;
        }
        widget = widget->parentWidget();
    }
}

bool ShadTooltipPrivate::hasToolTip(const QWidget* widget)
{
    return widget != nullptr && (!widget->toolTip().isEmpty() || widget->property(ShadStyleHelper::kCustomToolTipProperty).toBool());
}

ShadTooltip::ShadTooltip(QObject* parent)
    : QObject(parent), d_ptr(new ShadTooltipPrivate())
{
    Q_D(ShadTooltip);
    d->q_ptr = this;
    d->_showTimer = new QTimer(this);
    d->_showTimer->setSingleShot(true);
    connect(d->_showTimer, &QTimer::timeout, this, [d]()
    {
        d->_pendingWidget = nullptr;
        d->requestAt(QCursor::pos());
    });
}

ShadTooltip::~ShadTooltip()
{
    // 提示窗口在程序退出前（aboutToQuit）删除；静态对象析构时 QApplication 已销毁，不能再删除窗口
}

ShadTooltip* ShadTooltip::getInstance()
{
    // C++11 起局部静态变量初始化是线程安全的
    static ShadTooltip instance;
    return &instance;
}

void ShadTooltip::install()
{
    Q_D(ShadTooltip);
    if (d->_isInstalled || qApp == nullptr)
    {
        if (qApp == nullptr)
        {
            qWarning("ShadTooltip::install 需要在创建 QApplication 之后调用");
        }
        return;
    }
    d->_isInstalled = true;
    qApp->installEventFilter(this);
    // 重复 install / uninstall 时连接多次也没有影响：窗口删除后 QPointer 为空
    connect(qApp, &QCoreApplication::aboutToQuit, this, [d]()
    {
        d->_showTimer->stop();
        delete d->_window.data();
    });
}

void ShadTooltip::uninstall()
{
    Q_D(ShadTooltip);
    if (!d->_isInstalled)
    {
        return;
    }
    d->_isInstalled = false;
    d->hide();
    if (qApp != nullptr)
    {
        qApp->removeEventFilter(this);
    }
}

bool ShadTooltip::isInstalled() const
{
    Q_D(const ShadTooltip);
    return d->_isInstalled;
}

void ShadTooltip::setShowDelay(int milliseconds)
{
    Q_D(ShadTooltip);
    d->_showDelay = qMax(0, milliseconds);
}

int ShadTooltip::getShowDelay() const
{
    Q_D(const ShadTooltip);
    return d->_showDelay;
}

void ShadTooltip::setShowDelay(QWidget* widget, int milliseconds)
{
    if (widget != nullptr)
    {
        widget->setProperty(kShowDelayProperty, milliseconds < 0 ? QVariant() : QVariant(milliseconds));
    }
}

int ShadTooltip::getShowDelay(const QWidget* widget)
{
    if (widget == nullptr)
    {
        return -1;
    }
    const QVariant value = widget->property(kShowDelayProperty);
    return value.isValid() ? value.toInt() : -1;
}

void ShadTooltip::setSide(QWidget* widget, ShadPlacementType::Side side)
{
    if (widget != nullptr)
    {
        widget->setProperty(kSideProperty, side == ShadPlacementType::Top ? QVariant() : QVariant(int(side)));
    }
}

ShadPlacementType::Side ShadTooltip::getSide(const QWidget* widget)
{
    if (widget == nullptr)
    {
        return ShadPlacementType::Top;
    }
    const QVariant value = widget->property(kSideProperty);
    return value.isValid() ? static_cast<ShadPlacementType::Side>(value.toInt()) : ShadPlacementType::Top;
}

void ShadTooltip::setShortcut(QWidget* widget, const QStringList& keys)
{
    if (widget != nullptr)
    {
        widget->setProperty(kShortcutProperty, keys.isEmpty() ? QVariant() : QVariant(keys));
    }
}

void ShadTooltip::setShortcut(QWidget* widget, const QKeySequence& sequence)
{
    setShortcut(widget, ShadKbdGroup::keyTexts(sequence));
}

QStringList ShadTooltip::getShortcut(const QWidget* widget)
{
    return widget != nullptr ? widget->property(kShortcutProperty).toStringList() : QStringList();
}

void ShadTooltip::showText(QWidget* widget, const QRect& rect, const QString& text)
{
    Q_D(ShadTooltip);
    if (widget == nullptr)
    {
        return;
    }
    if (!d->_isInstalled)
    {
        QToolTip::showText(widget->mapToGlobal(rect.center()), text, widget, rect);
        return;
    }
    d->showFor(widget, rect, text, QStringList());
}

void ShadTooltip::hideText()
{
    Q_D(ShadTooltip);
    if (!d->_isInstalled)
    {
        QToolTip::hideText();
        return;
    }
    d->hide();
}

bool ShadTooltip::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadTooltip);
    QWidget* widget = watched->isWidgetType() ? static_cast<QWidget*>(watched) : nullptr;
    if (widget == nullptr || widget == d->_window.data())
    {
        return QObject::eventFilter(watched, event);
    }
    const bool isTarget = widget == d->_target.data();
    switch (event->type())
    {
        case QEvent::ToolTip:
            {
                // 悬停等待还没结束时屏蔽 Qt 自身按 WakeUpDelay 发出的提示事件，等待结束后由计时器重新请求
                if (d->_showTimer->isActive())
                {
                    event->accept();
                    return true;
                }
                // 自行处理提示的组件交给组件本身；没有提示文字时不处理，Qt 会交给父组件
                if (widget->property(ShadStyleHelper::kCustomToolTipProperty).toBool() || widget->toolTip().isEmpty())
                {
                    break;
                }
                d->showFor(widget, widget->rect(), widget->toolTip(), getShortcut(widget));
                event->accept();
                return true;
            }
        case QEvent::Enter:
            {
                // 不等待 Qt 的提示延迟，按 showDelay 主动请求
                if (ShadTooltipPrivate::hasToolTip(widget))
                {
                    const int widgetDelay = getShowDelay(widget);
                    d->_pendingWidget = widget;
                    d->_showTimer->start(widgetDelay >= 0 ? widgetDelay : d->_showDelay);
                }
                break;
            }
        case QEvent::Leave:
            {
                // 等待期间离开组件则取消，只是经过时不弹出
                if (widget == d->_pendingWidget.data())
                {
                    d->_showTimer->stop();
                    d->_pendingWidget = nullptr;
                }
                if (isTarget)
                {
                    d->hide();
                }
                break;
            }
        case QEvent::MouseMove:
            {
                // 组件内有多个提示区域时，光标离开当前区域即关闭
                if (isTarget && !d->_targetRect.contains(widget->mapFromGlobal(QCursor::pos())))
                {
                    d->hide();
                }
                break;
            }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            {
                d->hide();
                break;
            }
        case QEvent::KeyPress:
            {
                if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape && !d->_target.isNull())
                {
                    d->hide();
                }
                break;
            }
        case QEvent::FocusIn:
            {
                // 键盘聚焦时同样显示（Radix Tooltip 在 focus-visible 时打开）
                const Qt::FocusReason reason = static_cast<QFocusEvent*>(event)->reason();
                const bool isKeyboard = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason;
                if (isKeyboard && !widget->toolTip().isEmpty() && !widget->property(ShadStyleHelper::kCustomToolTipProperty).toBool())
                {
                    d->showFor(widget, widget->rect(), widget->toolTip(), getShortcut(widget));
                }
                break;
            }
        case QEvent::FocusOut:
        case QEvent::Hide:
        case QEvent::EnabledChange:
            {
                if (isTarget)
                {
                    d->hide();
                }
                break;
            }
        case QEvent::ToolTipChange:
            {
                // 提示文字变化时更新正在显示的提示
                if (isTarget && !widget->property(ShadStyleHelper::kCustomToolTipProperty).toBool())
                {
                    d->showFor(widget, widget->rect(), widget->toolTip(), getShortcut(widget));
                }
                break;
            }
        default:
            {
                break;
            }
    }
    return QObject::eventFilter(watched, event);
}
