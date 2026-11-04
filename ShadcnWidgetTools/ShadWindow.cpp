#include "ShadWindow.h"

#include <QByteArray>
#include <QLayoutItem>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTitleBar.h"
#include "ShadWindowFrame.h"
#include "ShadWindowPrivate.h"

ShadWindow::ShadWindow(QWidget* parent)
    : QWidget(parent, Qt::Window), d_ptr(new ShadWindowPrivate())
{
    Q_D(ShadWindow);
    d->q_ptr = this;
    d->_titleBar = new ShadTitleBar(this);
    d->_titleBar->setBarHeight(d->_titleBarHeight);
    d->_titleBar->setWindowActive(false);
    d->_frame = new ShadWindowFrame(this, d->_titleBar);
    connect(d->_titleBar, &ShadTitleBar::stayOnTopChanged, this, &ShadWindow::stayOnTopChanged);

    // 第 0 项为标题栏，第 1 项为内容区组件，未设置内容区时为弹性空间
    d->_rootLayout = new QVBoxLayout(this);
    d->_rootLayout->setContentsMargins(0, 0, 0, 0);
    d->_rootLayout->setSpacing(0);
    d->_rootLayout->addWidget(d->_titleBar);
    d->_rootLayout->addStretch(1);
}

ShadWindow::~ShadWindow()
{
}

void ShadWindow::setStayOnTop(bool stayOnTop)
{
    Q_D(ShadWindow);
    d->_titleBar->setStayOnTop(stayOnTop);
}

bool ShadWindow::isStayOnTop() const
{
    Q_D(const ShadWindow);
    return d->_titleBar->isStayOnTop();
}

void ShadWindow::setCentralWidget(QWidget* widget)
{
    Q_D(ShadWindow);
    if (d->_centralWidget == widget)
    {
        return;
    }
    if (QLayoutItem* item = d->_rootLayout->takeAt(1))
    {
        if (QWidget* oldWidget = item->widget())
        {
            ShadStyleHelper::clearFocusWithin(oldWidget);
            oldWidget->hide();
            oldWidget->deleteLater();
        }
        delete item;
    }
    d->_centralWidget = widget;
    if (widget != nullptr)
    {
        d->_rootLayout->addWidget(widget, 1);
    }
    else
    {
        d->_rootLayout->addStretch(1);
    }
}

QWidget* ShadWindow::getCentralWidget() const
{
    Q_D(const ShadWindow);
    return d->_centralWidget.data();
}

void ShadWindow::setTitleBarWidget(ShadWindowType::TitleBarArea area, QWidget* widget)
{
    Q_D(ShadWindow);
    d->_titleBar->setAreaWidget(area, widget);
}

QWidget* ShadWindow::getTitleBarWidget(ShadWindowType::TitleBarArea area) const
{
    Q_D(const ShadWindow);
    return d->_titleBar->getAreaWidget(area);
}

void ShadWindow::setTitleBarHeight(int height)
{
    Q_D(ShadWindow);
    const int barHeight = qMax(0, height);
    if (d->_titleBarHeight == barHeight)
    {
        return;
    }
    d->_titleBarHeight = barHeight;
    d->_titleBar->setBarHeight(barHeight);
}

int ShadWindow::getTitleBarHeight() const
{
    Q_D(const ShadWindow);
    return d->_titleBarHeight;
}

void ShadWindow::setButtonFlags(ShadWindowType::ButtonFlags buttonFlags)
{
    Q_D(ShadWindow);
    d->_titleBar->setButtonFlags(buttonFlags);
}

ShadWindowType::ButtonFlags ShadWindow::getButtonFlags() const
{
    Q_D(const ShadWindow);
    return d->_titleBar->getButtonFlags();
}

void ShadWindow::setButtonPosition(ShadWindowType::ButtonPosition position)
{
    Q_D(ShadWindow);
    d->_titleBar->setButtonPosition(position);
}

ShadWindowType::ButtonPosition ShadWindow::getButtonPosition() const
{
    Q_D(const ShadWindow);
    return d->_titleBar->getButtonPosition();
}

bool ShadWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    Q_D(ShadWindow);
    if (d->_frame->handleNativeEvent(eventType, message, result))
    {
        return true;
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void ShadWindow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadWindow);
    QPainter painter(this);
    d->_frame->paint(&painter);
}
