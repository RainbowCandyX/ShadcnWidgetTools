#include "ShadAccordionPanel.h"

#include <QEvent>
#include <QLayout>
#include <QObject>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QWidget>
#include <QtGlobal>

#include "ShadStyleHelper.h"

namespace
{
    constexpr int kBottomPadding = 16;
} // namespace

ShadAccordionPanel::ShadAccordionPanel(QWidget* parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setVisible(false);
}

ShadAccordionPanel::~ShadAccordionPanel()
{
    if (_content != nullptr)
    {
        _content->removeEventFilter(this);
    }
}

void ShadAccordionPanel::setContent(QWidget* content)
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

QWidget* ShadAccordionPanel::getContent() const
{
    return _content;
}

void ShadAccordionPanel::setProgress(qreal progress)
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

qreal ShadAccordionPanel::progress() const
{
    return _progress;
}

QSize ShadAccordionPanel::sizeHint() const
{
    const int contentWidth = _content != nullptr ? _content->sizeHint().width() : 0;
    const int width = contentWidth + 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(width, heightForWidth(width));
}

QSize ShadAccordionPanel::minimumSizeHint() const
{
    const int contentWidth = _content != nullptr ? _content->minimumSizeHint().width() : 0;
    return QSize(contentWidth + 2 * ShadStyleHelper::kFocusRingWidth, 0);
}

bool ShadAccordionPanel::hasHeightForWidth() const
{
    return true;
}

int ShadAccordionPanel::heightForWidth(int width) const
{
    const int contentWidth = width - 2 * ShadStyleHelper::kFocusRingWidth;
    return qRound((contentHeight(contentWidth) + kBottomPadding) * _progress);
}

bool ShadAccordionPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _content && event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        layoutContent();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadAccordionPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutContent();
}

int ShadAccordionPanel::contentHeight(int width) const
{
    if (_content == nullptr)
    {
        return 0;
    }
    const int height = _content->hasHeightForWidth() ? _content->heightForWidth(qMax(1, width)) : _content->sizeHint().height();
    return qMax(height, _content->minimumSizeHint().height());
}

void ShadAccordionPanel::layoutContent()
{
    if (_content == nullptr)
    {
        return;
    }
    const int ring = ShadStyleHelper::kFocusRingWidth;
    const int contentWidth = qMax(0, width() - 2 * ring);
    _content->setGeometry(ring, 0, contentWidth, contentHeight(contentWidth));
}

void ShadAccordionPanel::syncAncestorLayouts()
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
