#include "ShadMarkdownViewPrivate.h"

#include <QChar>
#include <QDesktopServices>
#include <QLatin1Char>
#include <QObject>
#include <QString>
#include <QSize>
#include <QStringList>
#include <QWidget>

#include "ShadCodeBlock.h"
#include "ShadMarkdownText.h"
#include "ShadMarkdownView.h"

namespace
{
    struct FenceInfo
    {
        QChar character;
        qsizetype length{0};
        QString info;
    };

    bool matchFence(const QString& line, FenceInfo* fence)
    {
        qsizetype index = 0;
        while (index < line.size() && line.at(index) == QLatin1Char(' '))
        {
            ++index;
        }
        if (index > 3 || index >= line.size())
        {
            return false;
        }
        const QChar character = line.at(index);
        if (character != QLatin1Char('`') && character != QLatin1Char('~'))
        {
            return false;
        }
        qsizetype run = 0;
        while (index + run < line.size() && line.at(index + run) == character)
        {
            ++run;
        }
        if (run < 3)
        {
            return false;
        }
        const QString info = line.mid(index + run).trimmed();
        if (character == QLatin1Char('`') && info.contains(QLatin1Char('`')))
        {
            return false;
        }
        fence->character = character;
        fence->length = run;
        fence->info = info;
        return true;
    }

    QString firstWord(const QString& info)
    {
        qsizetype end = 0;
        while (end < info.size() && !info.at(end).isSpace())
        {
            ++end;
        }
        return info.left(end);
    }

    bool isPendingMarker(const QString& line)
    {
        const QString text = line.trimmed();
        if (text.isEmpty())
        {
            return false;
        }
        FenceInfo fence;
        if (matchFence(text, &fence))
        {
            return true;
        }
        const QString markers = QStringLiteral("#>-*+=_`~ ");
        bool isMarkerOnly = true;
        for (const QChar character : text)
        {
            if (!markers.contains(character))
            {
                isMarkerOnly = false;
                break;
            }
        }
        if (isMarkerOnly)
        {
            return true;
        }
        qsizetype index = 0;
        while (index < text.size() && text.at(index).isDigit())
        {
            ++index;
        }
        return index > 0 && (index == text.size() || (index == text.size() - 1 && (text.at(index) == QLatin1Char('.') || text.at(index) == QLatin1Char(')'))));
    }

    bool isPendingFenceClose(const QString& line, QChar character)
    {
        const QString text = line.trimmed();
        if (text.isEmpty())
        {
            return false;
        }
        for (const QChar current : text)
        {
            if (current != character)
            {
                return false;
            }
        }
        return true;
    }
} // namespace

ShadMarkdownViewPrivate::ShadMarkdownViewPrivate()
{
}

ShadMarkdownViewPrivate::~ShadMarkdownViewPrivate()
{
}

QList<ShadMarkdownViewPrivate::ParsedSegment> ShadMarkdownViewPrivate::parse(const QString& markdown, qsizetype from, bool isStreaming)
{
    QList<ParsedSegment> result;
    QStringList lines;
    qsizetype segmentOffset = from;
    QString language;
    bool inCode = false;
    FenceInfo openFence;
    qsizetype position = from;
    const qsizetype size = markdown.size();

    const auto flushText = [&]()
    {
        const QString text = lines.join(QLatin1Char('\n'));
        if (!text.trimmed().isEmpty())
        {
            ParsedSegment segment;
            segment.isCode = false;
            segment.offset = segmentOffset;
            segment.text = text;
            result.append(segment);
        }
        lines.clear();
    };
    const auto flushCode = [&]()
    {
        ParsedSegment segment;
        segment.isCode = true;
        segment.offset = segmentOffset;
        segment.text = lines.join(QLatin1Char('\n'));
        segment.language = language;
        result.append(segment);
        lines.clear();
    };

    while (position < size)
    {
        qsizetype lineEnd = markdown.indexOf(QLatin1Char('\n'), position);
        const bool hasNewline = lineEnd >= 0;
        if (!hasNewline)
        {
            lineEnd = size;
        }
        QString line = markdown.mid(position, lineEnd - position);
        if (line.endsWith(QLatin1Char('\r')))
        {
            line.chop(1);
        }
        const qsizetype lineOffset = position;
        position = hasNewline ? lineEnd + 1 : size;
        if (isStreaming && !hasNewline && (inCode ? isPendingFenceClose(line, openFence.character) : isPendingMarker(line)))
        {
            break;
        }

        FenceInfo fence;
        const bool isFence = matchFence(line, &fence);
        if (inCode)
        {
            if (isFence && fence.character == openFence.character && fence.length >= openFence.length && fence.info.isEmpty())
            {
                flushCode();
                inCode = false;
                segmentOffset = position;
            }
            else
            {
                lines.append(line);
            }
        }
        else if (isFence)
        {
            flushText();
            inCode = true;
            openFence = fence;
            language = firstWord(fence.info);
            segmentOffset = lineOffset;
        }
        else
        {
            if (lines.isEmpty())
            {
                segmentOffset = lineOffset;
            }
            lines.append(line);
        }
    }
    if (inCode)
    {
        flushCode();
    }
    else
    {
        flushText();
    }
    return result;
}

void ShadMarkdownViewPrivate::sync(qsizetype fromOffset, int firstIndex, bool isStreaming)
{
    const QList<ParsedSegment> parsed = parse(_markdown, fromOffset, isStreaming);
    int index = firstIndex;
    for (const ParsedSegment& item : parsed)
    {
        if (index < _segments.size() && _segments.at(index).isCode == item.isCode)
        {
            updateSegment(_segments[index], item);
        }
        else
        {
            removeSegmentsFrom(index);
            createSegment(item);
        }
        ++index;
    }
    removeSegmentsFrom(index);
}

void ShadMarkdownViewPrivate::removeSegmentsFrom(int index)
{
    if (index >= _segments.size())
    {
        return;
    }
    for (int current = _segments.size() - 1; current >= index; --current)
    {
        const Segment& segment = _segments.at(current);
        QWidget* widget = segmentWidget(segment);
        widget->hide();
        widget->deleteLater();
    }
    _segments.remove(index, _segments.size() - index);
}

void ShadMarkdownViewPrivate::createSegment(const ParsedSegment& parsed)
{
    Q_Q(ShadMarkdownView);
    Segment segment;
    segment.isCode = parsed.isCode;
    segment.offset = parsed.offset;
    segment.text = parsed.text;
    segment.language = parsed.language;
    if (parsed.isCode)
    {
        segment.codeBlock = new ShadCodeBlock(q);
        segment.codeBlock->setCopyButtonVisible(true);
        segment.codeBlock->setLanguage(parsed.language);
        segment.codeBlock->setTitle(parsed.language);
        segment.codeBlock->setCode(parsed.text);
    }
    else
    {
        segment.textWidget = new ShadMarkdownText(q);
        segment.textWidget->setBaseUrl(_baseUrl);
        segment.textWidget->setMarkdownText(parsed.text);
        QObject::connect(segment.textWidget, &ShadMarkdownText::linkActivated, q, [this](const QUrl& url) { handleLink(url); });
    }
    _segments.append(segment);
}

void ShadMarkdownViewPrivate::updateSegment(Segment& segment, const ParsedSegment& parsed)
{
    segment.offset = parsed.offset;
    if (parsed.isCode)
    {
        if (segment.language != parsed.language)
        {
            segment.language = parsed.language;
            segment.codeBlock->setLanguage(parsed.language);
            segment.codeBlock->setTitle(parsed.language);
        }
        if (segment.text != parsed.text)
        {
            if (parsed.text.startsWith(segment.text))
            {
                segment.codeBlock->appendCode(parsed.text.mid(segment.text.size()));
            }
            else
            {
                segment.codeBlock->setCode(parsed.text);
            }
            segment.text = parsed.text;
        }
    }
    else if (segment.text != parsed.text)
    {
        segment.text = parsed.text;
        segment.textWidget->setMarkdownText(parsed.text);
    }
}

void ShadMarkdownViewPrivate::handleLink(const QUrl& url)
{
    Q_Q(ShadMarkdownView);
    Q_EMIT q->linkActivated(url);
    if (_openLinks)
    {
        QDesktopServices::openUrl(url);
    }
}

QWidget* ShadMarkdownViewPrivate::segmentWidget(const Segment& segment)
{
    return segment.isCode ? static_cast<QWidget*>(segment.codeBlock) : static_cast<QWidget*>(segment.textWidget);
}

int ShadMarkdownViewPrivate::segmentHeight(const QWidget* widget, int width)
{
    return widget->hasHeightForWidth() ? widget->heightForWidth(width) : widget->sizeHint().height();
}

int ShadMarkdownViewPrivate::contentHeight(int width) const
{
    int height = 0;
    for (const Segment& segment : _segments)
    {
        height += segmentHeight(segmentWidget(segment), width);
    }
    if (_segments.size() > 1)
    {
        height += static_cast<int>(_segments.size() - 1) * kSegmentSpacing;
    }
    return height;
}

void ShadMarkdownViewPrivate::arrangeSegments()
{
    Q_Q(ShadMarkdownView);
    const int width = q->width();
    int top = 0;
    for (const Segment& segment : _segments)
    {
        QWidget* widget = segmentWidget(segment);
        const int height = segmentHeight(widget, width);
        widget->setGeometry(0, top, width, height);
        if (widget->isHidden())
        {
            widget->show();
        }
        top += height + kSegmentSpacing;
    }
}
