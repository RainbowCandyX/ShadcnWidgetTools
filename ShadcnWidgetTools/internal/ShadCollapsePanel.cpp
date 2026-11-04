#include "ShadCollapsePanel.h"

#include <QEvent>
#include <QLayout>
#include <QObject>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QWidget>
#include <QtGlobal>

ShadCollapsePanel::ShadCollapsePanel(QWidget* parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setVisible(false);
}

ShadCollapsePanel::~ShadCollapsePanel()
{
    if (_content != nullptr)
    {
        _content->removeEventFilter(this);
    }
}

void ShadCollapsePanel::setContent(QWidget* content)
{
    if (_content == content)
    {
        return;
    }
    if (_content != nullptr)
    {
        _content->removeEventFilter(this);
        delete _content;
    }
    _content = content;
    if (_content != nullptr)
    {
        _content->setParent(this);
        _content->installEventFilter(this);
        _content->show();
    }
    updateGeometry();
    layoutContent();
}

QWidget* ShadCollapsePanel::getContent() const
{
    return _content;
}

void ShadCollapsePanel::setProgress(qreal progress)
{
    const qreal value = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(_progress + 1.0, value + 1.0))
    {
        return;
    }
    _progress = value;
    // 收起后不参与 Tab 焦点
    setVisible(_progress > 0.0);
    updateGeometry();
    syncAncestorLayouts();
}

qreal ShadCollapsePanel::progress() const
{
    return _progress;
}

QSize ShadCollapsePanel::sizeHint() const
{
    const int width = _content != nullptr ? _content->sizeHint().width() : 0;
    return QSize(width, heightForWidth(width));
}

QSize ShadCollapsePanel::minimumSizeHint() const
{
    return QSize(_content != nullptr ? _content->minimumSizeHint().width() : 0, 0);
}

bool ShadCollapsePanel::hasHeightForWidth() const
{
    return true;
}

int ShadCollapsePanel::heightForWidth(int width) const
{
    return qRound(contentHeight(width) * _progress);
}

bool ShadCollapsePanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _content && event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        layoutContent();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadCollapsePanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutContent();
}

int ShadCollapsePanel::contentHeight(int width) const
{
    if (_content == nullptr)
    {
        return 0;
    }
    const int height = _content->hasHeightForWidth() ? _content->heightForWidth(qMax(1, width)) : _content->sizeHint().height();
    return qMax(height, _content->minimumSizeHint().height());
}

void ShadCollapsePanel::layoutContent()
{
    if (_content == nullptr)
    {
        return;
    }
    _content->setGeometry(0, 0, width(), contentHeight(width()));
}

void ShadCollapsePanel::syncAncestorLayouts()
{
    for (QWidget* widget = parentWidget(); widget != nullptr; widget = widget->parentWidget())
    {
        if (QLayout* layout = widget->layout())
        {
            layout->activate();
        }
        if (widget->isWindow())
        {
            break;
        }
    }
}
