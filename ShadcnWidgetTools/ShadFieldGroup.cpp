#include "ShadFieldGroup.h"

#include <QBoxLayout>
#include <QChildEvent>
#include <QEvent>
#include <QObject>
#include <QtGlobal>

#include "ShadFieldGroupPrivate.h"
#include "ShadFieldSeparator.h"
#include "ShadMaxWidthBoxLayout.h"

ShadFieldGroup::ShadFieldGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadFieldGroupPrivate())
{
    Q_D(ShadFieldGroup);
    d->q_ptr = this;
    // 设置最大宽度后，自动换行的说明文字按实际宽度计算高度
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
}

ShadFieldGroup::~ShadFieldGroup()
{
}

void ShadFieldGroup::addWidget(QWidget* widget)
{
    Q_D(ShadFieldGroup);
    if (widget == nullptr)
    {
        return;
    }
    d->_widgets.removeAll(widget);
    d->_widgets.append(widget);
    d->rebuildLayout();
}

ShadFieldSeparator* ShadFieldGroup::addSeparator(const QString& text)
{
    ShadFieldSeparator* separator = new ShadFieldSeparator(text, this);
    addWidget(separator);
    return separator;
}

void ShadFieldGroup::removeWidget(QWidget* widget)
{
    Q_D(ShadFieldGroup);
    if (widget == nullptr || d->_widgets.removeAll(widget) == 0)
    {
        return;
    }
    d->rebuildLayout();
    widget->setParent(nullptr);
}

void ShadFieldGroup::setSpacing(int spacing)
{
    Q_D(ShadFieldGroup);
    d->_spacing = qMax(0, spacing);
    d->rebuildLayout();
}

int ShadFieldGroup::getSpacing() const
{
    Q_D(const ShadFieldGroup);
    return d->_spacing;
}

void ShadFieldGroup::childEvent(QChildEvent* event)
{
    Q_D(ShadFieldGroup);
    // 组件被删除或移到别处时重建布局，去掉多余的间距；只按 QObject 指针比较，析构过程中的组件不会被访问
    if (event->type() == QEvent::ChildRemoved)
    {
        bool isRemoved = false;
        for (int index = int(d->_widgets.size()) - 1; index >= 0; --index)
        {
            const QPointer<QWidget>& widget = d->_widgets.at(index);
            if (widget.isNull() || static_cast<QObject*>(widget.data()) == event->child())
            {
                d->_widgets.removeAt(index);
                isRemoved = true;
            }
        }
        if (isRemoved)
        {
            d->rebuildLayout();
        }
    }
    QWidget::childEvent(event);
}
