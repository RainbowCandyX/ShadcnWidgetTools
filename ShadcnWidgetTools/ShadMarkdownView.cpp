#include "ShadMarkdownView.h"

#include <QEvent>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadMarkdownText.h"
#include "ShadMarkdownViewPrivate.h"

ShadMarkdownView::ShadMarkdownView(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadMarkdownViewPrivate())
{
    Q_D(ShadMarkdownView);
    d->q_ptr = this;
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

ShadMarkdownView::~ShadMarkdownView()
{
}

void ShadMarkdownView::setMarkdown(const QString& markdown)
{
    Q_D(ShadMarkdownView);
    if (d->_markdown == markdown)
    {
        return;
    }
    d->_markdown = markdown;
    d->sync(0, 0, false);
    d->arrangeSegments();
    updateGeometry();
}

QString ShadMarkdownView::getMarkdown() const
{
    Q_D(const ShadMarkdownView);
    return d->_markdown;
}

void ShadMarkdownView::appendMarkdown(const QString& markdown)
{
    Q_D(ShadMarkdownView);
    if (markdown.isEmpty())
    {
        return;
    }
    d->_markdown += markdown;
    if (d->_segments.isEmpty())
    {
        d->sync(0, 0, true);
    }
    else
    {
        const int lastIndex = static_cast<int>(d->_segments.size()) - 1;
        d->sync(d->_segments.at(lastIndex).offset, lastIndex, true);
    }
    d->arrangeSegments();
    updateGeometry();
}

void ShadMarkdownView::clear()
{
    Q_D(ShadMarkdownView);
    d->_markdown.clear();
    d->removeSegmentsFrom(0);
    updateGeometry();
}

void ShadMarkdownView::setOpenLinks(bool open)
{
    Q_D(ShadMarkdownView);
    d->_openLinks = open;
}

bool ShadMarkdownView::isOpenLinks() const
{
    Q_D(const ShadMarkdownView);
    return d->_openLinks;
}

void ShadMarkdownView::setBaseUrl(const QUrl& url)
{
    Q_D(ShadMarkdownView);
    d->_baseUrl = url;
    for (const ShadMarkdownViewPrivate::Segment& segment : d->_segments)
    {
        if (segment.textWidget != nullptr)
        {
            segment.textWidget->setBaseUrl(url);
        }
    }
}

QUrl ShadMarkdownView::getBaseUrl() const
{
    Q_D(const ShadMarkdownView);
    return d->_baseUrl;
}

int ShadMarkdownView::heightForWidth(int width) const
{
    Q_D(const ShadMarkdownView);
    return d->contentHeight(width);
}

QSize ShadMarkdownView::sizeHint() const
{
    Q_D(const ShadMarkdownView);
    int width = 0;
    for (const ShadMarkdownViewPrivate::Segment& segment : d->_segments)
    {
        width = qMax(width, ShadMarkdownViewPrivate::segmentWidget(segment)->sizeHint().width());
    }
    return QSize(width, d->contentHeight(width));
}

QSize ShadMarkdownView::minimumSizeHint() const
{
    Q_D(const ShadMarkdownView);
    int width = 0;
    for (const ShadMarkdownViewPrivate::Segment& segment : d->_segments)
    {
        width = qMax(width, ShadMarkdownViewPrivate::segmentWidget(segment)->minimumSizeHint().width());
    }
    return QSize(width, 0);
}

bool ShadMarkdownView::event(QEvent* event)
{
    if (event->type() == QEvent::LayoutRequest)
    {
        Q_D(ShadMarkdownView);
        d->arrangeSegments();
        updateGeometry();
    }
    return QWidget::event(event);
}

void ShadMarkdownView::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadMarkdownView);
    QWidget::resizeEvent(event);
    d->arrangeSegments();
}
