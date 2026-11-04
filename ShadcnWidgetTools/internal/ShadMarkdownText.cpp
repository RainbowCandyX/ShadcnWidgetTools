#include "ShadMarkdownText.h"

#include <QAbstractTextDocumentLayout>
#include <QBrush>
#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QImage>
#include <QList>
#include <QMetaType>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizeF>
#include <QSizePolicy>
#include <QStringList>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextFormat>
#include <QTextFragment>
#include <QTextFrame>
#include <QTextImageFormat>
#include <QTextLength>
#include <QTextList>
#include <QTextOption>
#include <QTextTable>
#include <QTextTableCell>
#include <QTextTableCellFormat>
#include <QTextTableFormat>
#include <QVariant>
#include <QWheelEvent>
#include <QtGlobal>
#include <QtGui/qtguiglobal.h>
#include <QtMath>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kBodyPixelSize = 14;
    constexpr int kCodePixelSize = 13;
    constexpr qreal kLineHeightPercent = 160.0;
    constexpr qreal kParagraphGap = 12.0;
    constexpr qreal kListItemGap = 4.0;
    constexpr qreal kQuoteItemGap = 8.0;
    constexpr qreal kQuoteIndent = 16.0;
    constexpr qreal kQuoteBarWidth = 2.0;
    constexpr qreal kListIndentWidth = 24.0;
    constexpr qreal kTablePadding = 8.0;
    constexpr int kDefaultWidth = 400;
    constexpr int kMinimumWidth = 32;

    struct FragmentInfo
    {
        int position{0};
        int length{0};
        QTextCharFormat format;
    };

    int headingPixelSize(int level)
    {
        switch (level)
        {
            case 1:
                return 30;
            case 2:
                return 24;
            case 3:
                return 20;
            case 4:
                return 18;
            default:
                return 16;
        }
    }

    int headingWeight(int level)
    {
        return level == 1 ? static_cast<int>(QFont::Bold) : static_cast<int>(QFont::DemiBold);
    }

    QSize imageNaturalSize(const QVariant& resource)
    {
        switch (resource.typeId())
        {
            case QMetaType::QImage:
                return resource.value<QImage>().size();
            case QMetaType::QPixmap:
                return resource.value<QPixmap>().size();
            case QMetaType::QByteArray:
                return QImage::fromData(resource.toByteArray()).size();
            default:
                return QSize();
        }
    }

    void styleTables(QTextDocument* document, QTextFrame* frame, const QColor& border, const QColor& muted)
    {
        const QList<QTextFrame*> children = frame->childFrames();
        for (QTextFrame* child : children)
        {
            QTextTable* table = qobject_cast<QTextTable*>(child);
            if (table != nullptr)
            {
                QTextCursor afterCursor = table->lastCursorPosition();
                const bool hasNext = afterCursor.movePosition(QTextCursor::NextBlock);
                const bool isTrailing = !hasNext || (afterCursor.block() == document->lastBlock() && afterCursor.block().length() <= 1);
                QTextTableFormat tableFormat = table->format();
                tableFormat.setBorder(1);
                tableFormat.setBorderBrush(QBrush(border));
                tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
                tableFormat.setBorderCollapse(true);
                tableFormat.setCellPadding(kTablePadding);
                tableFormat.setCellSpacing(0);
                tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
                tableFormat.setTopMargin(0);
                tableFormat.setBottomMargin(isTrailing ? 0 : kParagraphGap);
                table->setFormat(tableFormat);
                const int headerRows = qMax(1, table->format().headerRowCount());
                for (int row = 0; row < qMin(headerRows, table->rows()); ++row)
                {
                    for (int column = 0; column < table->columns(); ++column)
                    {
                        QTextTableCell cell = table->cellAt(row, column);
                        QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
                        cellFormat.setBackground(muted);
                        cell.setFormat(cellFormat);
                        QTextCursor cellCursor = cell.firstCursorPosition();
                        cellCursor.setPosition(cell.lastCursorPosition().position(), QTextCursor::KeepAnchor);
                        QTextCharFormat boldFormat;
                        boldFormat.setFontWeight(QFont::Bold);
                        cellCursor.mergeCharFormat(boldFormat);
                    }
                }
            }
            styleTables(document, child, border, muted);
        }
    }
} // namespace

ShadMarkdownText::ShadMarkdownText(QWidget* parent)
    : QTextBrowser(parent)
{
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    viewport()->setAutoFillBackground(false);
    document()->setDocumentMargin(0);
    document()->setUndoRedoEnabled(false);
    document()->setIndentWidth(kListIndentWidth);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    QFont baseFont = font();
    baseFont.setPixelSize(kBodyPixelSize);
    setFont(baseFont);
    applyPalette();
    connect(this, &QTextBrowser::anchorClicked, this, &ShadMarkdownText::linkActivated);
    connect(document(), &QTextDocument::contentsChanged, this, [this]()
    {
        _cachedWidth = -1;
        updateGeometry();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        applyPalette();
        rebuildDocument();
    });
}

ShadMarkdownText::~ShadMarkdownText()
{
}

void ShadMarkdownText::setMarkdownText(const QString& markdown)
{
    _markdown = markdown;
    rebuildDocument();
}

QString ShadMarkdownText::getMarkdownText() const
{
    return _markdown;
}

void ShadMarkdownText::setBaseUrl(const QUrl& url)
{
    document()->setBaseUrl(url);
    if (url.isLocalFile())
    {
        const QFileInfo info(url.toLocalFile());
        setSearchPaths(QStringList{info.isDir() ? info.absoluteFilePath() : info.absolutePath()});
    }
    else if (url.scheme() == QLatin1String("qrc"))
    {
        setSearchPaths(QStringList{QStringLiteral(":") + url.path()});
    }
    else
    {
        setSearchPaths(QStringList());
    }
    rebuildDocument();
}

int ShadMarkdownText::heightForWidth(int width) const
{
    if (width <= 0)
    {
        width = kDefaultWidth;
    }
    if (_cachedWidth == width)
    {
        return _cachedHeight;
    }
    QTextDocument* doc = document();
    const qreal oldWidth = doc->textWidth();
    doc->setTextWidth(width);
    const int height = qCeil(doc->size().height());
    if (!qFuzzyCompare(oldWidth + 1.0, static_cast<qreal>(width) + 1.0))
    {
        doc->setTextWidth(oldWidth);
    }
    _cachedWidth = width;
    _cachedHeight = height;
    return height;
}

QSize ShadMarkdownText::sizeHint() const
{
    return QSize(kDefaultWidth, heightForWidth(kDefaultWidth));
}

QSize ShadMarkdownText::minimumSizeHint() const
{
    return QSize(kMinimumWidth, 0);
}

void ShadMarkdownText::paintEvent(QPaintEvent* event)
{
    {
        QPainter painter(viewport());
        painter.setClipRect(event->rect());
        const QPointF offset(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
        const QColor border = shadTheme->getColor(ShadThemeType::Border);
        QAbstractTextDocumentLayout* layout = document()->documentLayout();
        for (QTextBlock block = document()->begin(); block.isValid(); block = block.next())
        {
            const QTextBlockFormat blockFormat = block.blockFormat();
            const int level = blockFormat.intProperty(QTextFormat::BlockQuoteLevel);
            if (level <= 0)
            {
                continue;
            }
            const QRectF rect = layout->blockBoundingRect(block).translated(offset);
            if (rect.bottom() + kParagraphGap < event->rect().top() || rect.top() > event->rect().bottom())
            {
                continue;
            }
            const QTextBlock next = block.next();
            const qreal extension = (next.isValid() && next.blockFormat().intProperty(QTextFormat::BlockQuoteLevel) > 0) ? blockFormat.bottomMargin() : 0.0;
            for (int current = 1; current <= level; ++current)
            {
                painter.fillRect(QRectF(offset.x() + (current - 1) * kQuoteIndent, rect.top(), kQuoteBarWidth, rect.height() + extension), border);
            }
        }
    }
    QTextBrowser::paintEvent(event);
}

void ShadMarkdownText::resizeEvent(QResizeEvent* event)
{
    QTextBrowser::resizeEvent(event);
    if (_hasImages && !_applying && _styledWidth != viewport()->width())
    {
        applyStyle();
    }
}

void ShadMarkdownText::wheelEvent(QWheelEvent* event)
{
    event->ignore();
}

void ShadMarkdownText::changeEvent(QEvent* event)
{
    QTextBrowser::changeEvent(event);
    if (event->type() == QEvent::FontChange && !_applying)
    {
        rebuildDocument();
    }
}

void ShadMarkdownText::applyPalette()
{
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    QPalette textPalette = palette();
    textPalette.setColor(QPalette::Base, Qt::transparent);
    textPalette.setColor(QPalette::Text, foreground);
    textPalette.setColor(QPalette::WindowText, foreground);
    textPalette.setColor(QPalette::Link, primary);
    textPalette.setColor(QPalette::LinkVisited, primary);
    textPalette.setColor(QPalette::Dark, shadTheme->getColor(ShadThemeType::Border));
    textPalette.setColor(QPalette::Highlight, ShadStyleHelper::withAlpha(primary, 0.3));
    textPalette.setColor(QPalette::HighlightedText, foreground);
    setPalette(textPalette);
}

void ShadMarkdownText::rebuildDocument()
{
    if (_applying)
    {
        return;
    }
    _applying = true;
#if QT_CONFIG(textmarkdownreader)
    document()->setMarkdown(_markdown, QTextDocument::MarkdownDialectGitHub);
#else
    document()->setPlainText(_markdown);
#endif
    applyStyle();
    _applying = false;
    _cachedWidth = -1;
    updateGeometry();
}

void ShadMarkdownText::applyStyle()
{
    const bool wasApplying = _applying;
    _applying = true;
    QTextDocument* doc = document();
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor muted = shadTheme->getColor(ShadThemeType::Muted);
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    const QColor border = shadTheme->getColor(ShadThemeType::Border);
    const QString codeFamily = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    const int availableWidth = viewport()->width();
    _hasImages = false;
    _styledWidth = availableWidth;

    QTextCursor editCursor(doc);
    editCursor.beginEditBlock();
    const QTextBlock firstBlock = doc->firstBlock();
    const QTextBlock lastBlock = doc->lastBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next())
    {
        const QTextBlockFormat sourceFormat = block.blockFormat();
        const int headingLevel = sourceFormat.headingLevel();
        const int quoteLevel = sourceFormat.intProperty(QTextFormat::BlockQuoteLevel);
        const bool isRule = sourceFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
        QTextCursor blockCursor(block);
        const bool inTable = blockCursor.currentTable() != nullptr;
        const QTextBlock nextBlock = block.next();

        QTextBlockFormat blockFormat;
        qreal topMargin = 0.0;
        qreal bottomMargin = kParagraphGap;
        if (inTable)
        {
            bottomMargin = 0.0;
        }
        else if (headingLevel > 0)
        {
            topMargin = headingLevel <= 2 ? 24.0 : 16.0;
            bottomMargin = 8.0;
        }
        else if (block.textList() != nullptr)
        {
            bottomMargin = (nextBlock.isValid() && nextBlock.textList() != nullptr) ? kListItemGap : kParagraphGap;
        }
        else if (quoteLevel > 0)
        {
            bottomMargin = (nextBlock.isValid() && nextBlock.blockFormat().intProperty(QTextFormat::BlockQuoteLevel) > 0) ? kQuoteItemGap : kParagraphGap;
        }
        if (block == firstBlock)
        {
            topMargin = 0.0;
        }
        if (block == lastBlock)
        {
            bottomMargin = 0.0;
        }
        const bool isTrailingEmpty = block == lastBlock && block.length() <= 1 && !isRule && block != firstBlock;
        if (isTrailingEmpty)
        {
            blockFormat.setLineHeight(1.0, QTextBlockFormat::FixedHeight);
        }
        else
        {
            blockFormat.setLineHeight(kLineHeightPercent, QTextBlockFormat::ProportionalHeight);
        }
        blockFormat.setTopMargin(topMargin);
        blockFormat.setBottomMargin(bottomMargin);
        if (quoteLevel > 0 && !inTable)
        {
            blockFormat.setLeftMargin(quoteLevel * kQuoteIndent);
            if (block.textList() == nullptr)
            {
                blockFormat.setIndent(0);
            }
        }
        blockCursor.mergeBlockFormat(blockFormat);
        if (isRule)
        {
            QTextCharFormat ruleFormat = block.charFormat();
            ruleFormat.setForeground(border);
            blockCursor.setBlockCharFormat(ruleFormat);
        }

        QList<FragmentInfo> fragments;
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it)
        {
            const QTextFragment fragment = it.fragment();
            if (fragment.isValid())
            {
                fragments.append({fragment.position(), fragment.length(), fragment.charFormat()});
            }
        }
        for (const FragmentInfo& info : fragments)
        {
            QTextCharFormat format = info.format;
            QTextCursor fragmentCursor(doc);
            fragmentCursor.setPosition(info.position);
            fragmentCursor.setPosition(info.position + info.length, QTextCursor::KeepAnchor);
            if (format.isImageFormat())
            {
                QTextImageFormat imageFormat = format.toImageFormat();
                const QSize natural = imageNaturalSize(doc->resource(QTextDocument::ImageResource, QUrl(imageFormat.name())));
                _hasImages = true;
                if (natural.isValid() && availableWidth > 0)
                {
                    const int width = qMin(natural.width(), availableWidth);
                    imageFormat.setWidth(width);
                    imageFormat.setHeight(static_cast<qreal>(natural.height()) * width / natural.width());
                    fragmentCursor.setCharFormat(imageFormat);
                }
                continue;
            }
            const bool isCode = format.fontFixedPitch() || format.font().fixedPitch();
            const bool isLink = format.isAnchor() && !format.anchorHref().isEmpty();
            int pixelSize = kBodyPixelSize;
            if (headingLevel > 0)
            {
                pixelSize = headingPixelSize(headingLevel);
            }
            else if (isCode)
            {
                pixelSize = kCodePixelSize;
            }
            format.clearProperty(QTextFormat::FontPointSize);
            format.clearProperty(QTextFormat::FontSizeAdjustment);
            format.setProperty(QTextFormat::FontPixelSize, pixelSize);
            if (headingLevel > 0)
            {
                format.setFontWeight(headingWeight(headingLevel));
            }
            format.setForeground(quoteLevel > 0 ? mutedForeground : foreground);
            if (quoteLevel > 0)
            {
                format.setFontItalic(true);
            }
            if (isCode)
            {
                format.clearProperty(QTextFormat::FontFamily);
                format.setFontFamilies(QStringList{codeFamily});
                format.setFontFixedPitch(true);
                format.setBackground(muted);
                format.setForeground(foreground);
            }
            if (isLink)
            {
                format.setForeground(primary);
                format.setFontUnderline(true);
            }
            fragmentCursor.setCharFormat(format);
        }
    }
    styleTables(doc, doc->rootFrame(), border, muted);
    editCursor.endEditBlock();
    _applying = wasApplying;
}
