#include "ShadWidget.h"

#include <QByteArray>
#include <QCloseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadTitleBar.h"
#include "ShadWidgetPrivate.h"
#include "ShadWindowFrame.h"

ShadWidget::ShadWidget(QWidget* parent)
    : QWidget(parent, Qt::Window), d_ptr(new ShadWidgetPrivate())
{
    Q_D(ShadWidget);
    d->q_ptr = this;
    d->_titleBar = new ShadTitleBar(this);
    d->_titleBar->setBarHeight(d->_titleBarHeight);
    d->_titleBar->setWindowActive(false);
    d->_frame = new ShadWindowFrame(this, d->_titleBar);
    // 标题栏不进入布局，窗口的上边距让出标题栏高度，使用者的布局排在标题栏下方
    d->_frame->setTitleBarFloating(true);
    connect(d->_titleBar, &ShadTitleBar::closeButtonClicked, this, [this]()
    {
        Q_EMIT closeButtonClicked();
        // 关闭了默认关闭行为时，关闭按钮与系统关闭请求统一通过 closeRequested 交给调用方
        if (!isDefaultCloseEnabled())
        {
            Q_EMIT closeRequested();
        }
    });
    connect(d->_titleBar, &ShadTitleBar::stayOnTopChanged, this, &ShadWidget::stayOnTopChanged);
    resize(ShadWidgetPrivate::kDefaultWidth, ShadWidgetPrivate::kDefaultHeight);
}

ShadWidget::~ShadWidget()
{
}

void ShadWidget::setTitleBarWidget(ShadWindowType::TitleBarArea area, QWidget* widget)
{
    Q_D(ShadWidget);
    d->_titleBar->setAreaWidget(area, widget);
}

QWidget* ShadWidget::getTitleBarWidget(ShadWindowType::TitleBarArea area) const
{
    Q_D(const ShadWidget);
    return d->_titleBar->getAreaWidget(area);
}

void ShadWidget::setTitleBarHeight(int height)
{
    Q_D(ShadWidget);
    const int barHeight = qMax(0, height);
    if (d->_titleBarHeight == barHeight)
    {
        return;
    }
    d->_titleBarHeight = barHeight;
    d->_titleBar->setBarHeight(barHeight);
    // 上边距与标题栏位置随高度变化
    d->_frame->updateFrameLayout();
}

int ShadWidget::getTitleBarHeight() const
{
    Q_D(const ShadWidget);
    return d->_titleBarHeight;
}

void ShadWidget::setButtonFlags(ShadWindowType::ButtonFlags buttonFlags)
{
    Q_D(ShadWidget);
    d->_titleBar->setButtonFlags(buttonFlags);
}

ShadWindowType::ButtonFlags ShadWidget::getButtonFlags() const
{
    Q_D(const ShadWidget);
    return d->_titleBar->getButtonFlags();
}

void ShadWidget::setButtonPosition(ShadWindowType::ButtonPosition position)
{
    Q_D(ShadWidget);
    d->_titleBar->setButtonPosition(position);
}

ShadWindowType::ButtonPosition ShadWidget::getButtonPosition() const
{
    Q_D(const ShadWidget);
    return d->_titleBar->getButtonPosition();
}

void ShadWidget::setStayOnTop(bool stayOnTop)
{
    Q_D(ShadWidget);
    d->_titleBar->setStayOnTop(stayOnTop);
}

bool ShadWidget::isStayOnTop() const
{
    Q_D(const ShadWidget);
    return d->_titleBar->isStayOnTop();
}

void ShadWidget::setDefaultCloseEnabled(bool enabled)
{
    Q_D(ShadWidget);
    d->_titleBar->setDefaultCloseEnabled(enabled);
}

bool ShadWidget::isDefaultCloseEnabled() const
{
    Q_D(const ShadWidget);
    return d->_titleBar->isDefaultCloseEnabled();
}

void ShadWidget::moveToCenter()
{
    if (isMaximized() || isFullScreen())
    {
        return;
    }
    QScreen* currentScreen = screen();
    if (currentScreen == nullptr)
    {
        return;
    }
    // 按客户区居中；move 使用含系统边框的窗口位置，扣除两者的偏移
    QRect clientRect = geometry();
    clientRect.moveCenter(currentScreen->availableGeometry().center());
    const QPoint frameOffset = geometry().topLeft() - pos();
    move(clientRect.topLeft() - frameOffset);
}

void ShadWidget::closeEvent(QCloseEvent* event)
{
    // 系统发起的关闭（Alt+F4、任务栏或系统菜单的关闭、窗口管理器）为 spontaneous 事件；
    // 关闭了默认关闭行为时拦截下来交给调用方，代码调用 close() 不受影响
    if (!isDefaultCloseEnabled() && event->spontaneous())
    {
        event->ignore();
        Q_EMIT closeRequested();
        return;
    }
    QWidget::closeEvent(event);
}

bool ShadWidget::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_D(ShadWidget);
    if (d->_frame->handleNativeEvent(eventType, message, result))
    {
        return true;
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void ShadWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadWidget);
    QPainter painter(this);
    d->_frame->paint(&painter);
}
