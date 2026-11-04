#include "ShadSidebarCollapseArea.h"

#include <QEvent>
#include <QLayout>
#include <QObject>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtGlobal>

ShadSidebarCollapseArea::ShadSidebarCollapseArea(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _content = new QWidget(this);
    // 内容的布局变化（子项增减、子菜单展开）时同步容器高度
    _content->installEventFilter(this);
}

ShadSidebarCollapseArea::~ShadSidebarCollapseArea()
{
    _content->removeEventFilter(this);
}

QWidget* ShadSidebarCollapseArea::contentWidget() const
{
    return _content;
}

void ShadSidebarCollapseArea::setProgress(qreal progress)
{
    const qreal value = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(_progress, value))
    {
        return;
    }
    _progress = value;
    updateGeometry();
    syncAncestorLayouts();
}

qreal ShadSidebarCollapseArea::progress() const
{
    return _progress;
}

QSize ShadSidebarCollapseArea::sizeHint() const
{
    return QSize(_content->sizeHint().width(), visibleHeight());
}

QSize ShadSidebarCollapseArea::minimumSizeHint() const
{
    return QSize(_content->minimumSizeHint().width(), visibleHeight());
}

bool ShadSidebarCollapseArea::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _content && event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        layoutContent();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadSidebarCollapseArea::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutContent();
}

int ShadSidebarCollapseArea::contentHeight() const
{
    return qMax(_content->sizeHint().height(), _content->minimumSizeHint().height());
}

int ShadSidebarCollapseArea::visibleHeight() const
{
    return qRound(contentHeight() * _progress);
}

void ShadSidebarCollapseArea::syncAncestorLayouts()
{
    // 高度变化要经过菜单项、菜单区、分组等多层布局才落到相邻组件上，每层默认在下一轮事件循环才重排；
    // 若中间先绘制一帧，外层容器仍是旧高度，布局会把变矮的内容垂直居中，表现为菜单项上下跳动。由内向外逐层立即重排
    for (QWidget* widget = parentWidget(); widget != nullptr; widget = widget->parentWidget())
    {
        if (ShadSidebarCollapseArea * area = qobject_cast<ShadSidebarCollapseArea*>(widget))
        {
            // 裁剪容器没有布局，内容组件的尺寸由它自己设置
            area->updateGeometry();
            area->layoutContent();
        }
        else if (QLayout* layout = widget->layout())
        {
            layout->activate();
        }
        if (widget->isWindow())
        {
            break;
        }
    }
}

void ShadSidebarCollapseArea::layoutContent()
{
    // 内容贴顶、宽度跟随容器，高度保持完整，容器以外的部分由父子裁剪自然隐藏
    _content->setGeometry(0, 0, width(), contentHeight());
}
