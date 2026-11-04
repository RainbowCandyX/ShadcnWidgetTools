#include "ShadSidebarLayoutPrivate.h"

#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QMargins>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadSidebar.h"
#include "ShadSidebarLayout.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSidebarInsetArea::ShadSidebarInsetArea(QWidget* parent)
    : QWidget(parent)
{
}

void ShadSidebarInsetArea::setInset(bool inset)
{
    if (_isInset == inset)
    {
        return;
    }
    _isInset = inset;
    update();
}

bool ShadSidebarInsetArea::isInset() const
{
    return _isInset;
}

void ShadSidebarInsetArea::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QColor background = shadTheme->getColor(ShadThemeType::Background);
    if (!_isInset)
    {
        // 窗口已按特效绘制底色
        painter.fillRect(rect(), windowSurfaceColor(this, ShadThemeType::Background, 0.0));
        return;
    }
    // bg-background rounded-xl shadow-sm，绘制在布局边距以内
    const QRectF cardRect(contentsRect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge);
    drawShadowSm(&painter, cardRect, radius);
    fillRoundedRect(&painter, cardRect, radius, background);
}

ShadSidebarLayoutPrivate::ShadSidebarLayoutPrivate()
{
}

ShadSidebarLayoutPrivate::~ShadSidebarLayoutPrivate()
{
}

void ShadSidebarLayoutPrivate::arrange()
{
    // 先取出两项再按侧栏位置重新加入；QHBoxLayout 在 RTL 下自动镜像，起始侧即为第一项
    _layout->removeWidget(_insetArea);
    if (!_sidebar.isNull())
    {
        _layout->removeWidget(_sidebar);
    }
    const bool isSidebarAtEnd = !_sidebar.isNull() && _sidebar->getSide() == ShadSidebarType::End;
    if (!_sidebar.isNull() && !isSidebarAtEnd)
    {
        _layout->addWidget(_sidebar);
    }
    _layout->addWidget(_insetArea, 1);
    if (isSidebarAtEnd)
    {
        _layout->addWidget(_sidebar);
    }
    updateInsetMargins();
}

void ShadSidebarLayoutPrivate::updateInsetMargins()
{
    Q_Q(ShadSidebarLayout);
    const bool isInset = !_sidebar.isNull() && _sidebar->getVariant() == ShadSidebarType::Inset;
    QMargins margins;
    if (isInset)
    {
        // 逻辑上的起始侧 / 结束侧边距，再按布局方向换成物理左右
        const bool isSidebarAtStart = _sidebar->getSide() == ShadSidebarType::Start;
        const int sidebarSideMargin = _sidebar->width() == 0 ? kInsetMargin : 0;
        const int startMargin = isSidebarAtStart ? sidebarSideMargin : kInsetMargin;
        const int endMargin = isSidebarAtStart ? kInsetMargin : sidebarSideMargin;
        const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
        const int left = isRightToLeft ? endMargin : startMargin;
        const int right = isRightToLeft ? startMargin : endMargin;
        margins = QMargins(left, kInsetMargin, right, kInsetMargin);
    }
    // 侧栏宽度动画每一帧都会调用；状态不变时不重绘，否则整个主内容区每帧都要重绘
    if (_insetArea->isInset() == isInset && _insetArea->contentsMargins() == margins)
    {
        return;
    }
    _insetArea->setInset(isInset);
    _insetArea->setContentsMargins(margins);
    q->update();
}

void ShadSidebarLayoutPrivate::connectSidebar()
{
    if (_sidebar.isNull())
    {
        return;
    }
    _sideConnection = QObject::connect(_sidebar, &ShadSidebar::sideChanged, this, [this]() { arrange(); });
    _variantConnection = QObject::connect(_sidebar, &ShadSidebar::variantChanged, this, [this]() { updateInsetMargins(); });
    _sidebar->installEventFilter(this);
}

void ShadSidebarLayoutPrivate::disconnectSidebar()
{
    QObject::disconnect(_sideConnection);
    QObject::disconnect(_variantConnection);
    if (!_sidebar.isNull())
    {
        _sidebar->removeEventFilter(this);
    }
}

bool ShadSidebarLayoutPrivate::eventFilter(QObject* watched, QEvent* event)
{
    // 侧栏宽度变为 0 或从 0 恢复时调整 Inset 边距
    if (watched == _sidebar.data() && event->type() == QEvent::Resize)
    {
        updateInsetMargins();
    }
    return QObject::eventFilter(watched, event);
}
