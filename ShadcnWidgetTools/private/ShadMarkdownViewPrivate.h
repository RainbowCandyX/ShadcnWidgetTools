#ifndef SHADMARKDOWNVIEWPRIVATE_H
#define SHADMARKDOWNVIEWPRIVATE_H

#include <QList>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include "ShadProperty.h"

class QWidget;
class ShadCodeBlock;
class ShadMarkdownText;
class ShadMarkdownView;

class ShadMarkdownViewPrivate
{
    SHAD_D_CREATE(ShadMarkdownView)

    public:
        struct ParsedSegment
        {
            bool isCode{false};
            qsizetype offset{0};
            QString text;
            QString language;
        };

        struct Segment
        {
            bool isCode{false};
            qsizetype offset{0};
            QString text;
            QString language;
            ShadMarkdownText* textWidget{nullptr};
            ShadCodeBlock* codeBlock{nullptr};
        };

        ShadMarkdownViewPrivate();
        ~ShadMarkdownViewPrivate();

        static QList<ParsedSegment> parse(const QString& markdown, qsizetype from, bool isStreaming);
        void sync(qsizetype fromOffset, int firstIndex, bool isStreaming);
        void removeSegmentsFrom(int index);
        void createSegment(const ParsedSegment& parsed);
        void updateSegment(Segment& segment, const ParsedSegment& parsed);
        void handleLink(const QUrl& url);
        static QWidget* segmentWidget(const Segment& segment);
        static int segmentHeight(const QWidget* widget, int width);
        int contentHeight(int width) const;
        void arrangeSegments();

        static constexpr int kSegmentSpacing = 12;

        QString _markdown;
        bool _openLinks{true};
        QUrl _baseUrl;
        QList<Segment> _segments;
};

#endif // SHADMARKDOWNVIEWPRIVATE_H
