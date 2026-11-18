#include "ShadLogViewPrivate.h"

#include <QFont>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>

#include "ShadLogView.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kLevelTextWidth = 7;

    QString levelText(ShadLogViewType::Level level)
    {
        switch (level)
        {
            case ShadLogViewType::Debug:
                return QStringLiteral("DEBUG");
            case ShadLogViewType::Info:
                return QStringLiteral("INFO");
            case ShadLogViewType::Success:
                return QStringLiteral("SUCCESS");
            case ShadLogViewType::Warning:
                return QStringLiteral("WARNING");
            case ShadLogViewType::Error:
                return QStringLiteral("ERROR");
        }
        return QString();
    }
} // namespace

ShadLogViewPrivate::ShadLogViewPrivate()
{
}

ShadLogViewPrivate::~ShadLogViewPrivate()
{
}

void ShadLogViewPrivate::applyFont()
{
    Q_Q(ShadLogView);
    QFont font = q->font();
    font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Normal);
    q->setFont(font);
}

void ShadLogViewPrivate::applyThemeColors()
{
    Q_Q(ShadLogView);
    using ShadStyleHelper::kDisabledOpacity;
    using ShadStyleHelper::withAlpha;
    const QColor transparent(0, 0, 0, 0);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);

    QPalette palette = q->palette();
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        const qreal alpha = group == QPalette::Disabled ? kDisabledOpacity : 1.0;
        palette.setColor(group, QPalette::Base, transparent);
        palette.setColor(group, QPalette::Window, transparent);
        palette.setColor(group, QPalette::Text, withAlpha(foreground, alpha));
        palette.setColor(group, QPalette::PlaceholderText, withAlpha(placeholder, alpha));
        palette.setColor(group, QPalette::Highlight, selection);
        palette.setColor(group, QPalette::HighlightedText, selectionText);
    }
    q->setPalette(palette);
    q->update();
    q->viewport()->update();
}

void ShadLogViewPrivate::paintFrame()
{
    Q_Q(ShadLogView);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(q->rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(q);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(q->isEnabled() ? 1.0 : kDisabledOpacity);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(_isFocusVisible ? ShadThemeType::Ring : ShadThemeType::Input));
    if (_isFocusVisible)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false));
    }
}

bool ShadLogViewPrivate::matches(const Entry& entry) const
{
    return entry.level >= _minimumLevel && (_filterText.isEmpty() || entry.message.contains(_filterText, Qt::CaseInsensitive));
}

QColor ShadLogViewPrivate::levelColor(ShadLogViewType::Level level) const
{
    const bool isDark = shadTheme->isDark();
    switch (level)
    {
        case ShadLogViewType::Debug:
            return shadTheme->getColor(ShadThemeType::MutedForeground);
        case ShadLogViewType::Info:
            return isDark ? QColor(0x60, 0xa5, 0xfa) : QColor(0x25, 0x63, 0xeb);
        case ShadLogViewType::Success:
            return isDark ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
        case ShadLogViewType::Warning:
            return isDark ? QColor(0xfb, 0xbf, 0x24) : QColor(0xd9, 0x77, 0x06);
        case ShadLogViewType::Error:
            return shadTheme->getColor(ShadThemeType::Destructive);
    }
    return shadTheme->getColor(ShadThemeType::Foreground);
}

QColor ShadLogViewPrivate::messageColor(ShadLogViewType::Level level) const
{
    if (level == ShadLogViewType::Debug || level == ShadLogViewType::Warning || level == ShadLogViewType::Error)
    {
        return levelColor(level);
    }
    return shadTheme->getColor(ShadThemeType::Foreground);
}

void ShadLogViewPrivate::appendEntry(ShadLogViewType::Level level, const QString& message, const QDateTime& time)
{
    Q_Q(ShadLogView);
    Entry entry;
    entry.time = time.toUTC();
    entry.level = level;
    entry.message = message;
    entry.message.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    entry.message.replace(u'\r', u'\n');
    entry.blockCount = 1 + int(entry.message.count(u'\n'));
    entry.isVisible = matches(entry);

    const bool wasFollowing = _isFollowing;
    const int oldValue = q->verticalScrollBar()->value();
    _isUpdating = true;
    int removedLines = 0;
    if (_maxEntryCount > 0 && _entries.size() >= _maxEntryCount)
    {
        removedLines = removeFirst(int(_entries.size()) - _maxEntryCount + 1);
    }
    _entries.append(entry);
    if (entry.isVisible)
    {
        QTextCursor cursor(q->document());
        cursor.beginEditBlock();
        insertEntry(cursor, entry, _visibleCount == 0);
        cursor.endEditBlock();
        ++_visibleCount;
    }
    _isUpdating = false;
    restoreScroll(wasFollowing, oldValue - removedLines);
    Q_EMIT q->countChanged(int(_entries.size()));
}

void ShadLogViewPrivate::insertEntry(QTextCursor& cursor, const Entry& entry, bool isFirst) const
{
    cursor.movePosition(QTextCursor::End);
    if (!isFirst)
    {
        cursor.insertBlock();
    }
    QTextCharFormat format;
    if (_isTimestampVisible)
    {
        format.setForeground(shadTheme->getColor(ShadThemeType::MutedForeground));
        cursor.insertText(entry.time.toLocalTime().toString(_timestampFormat) + QStringLiteral("  "), format);
    }
    if (_isLevelVisible)
    {
        format.setForeground(levelColor(entry.level));
        cursor.insertText(levelText(entry.level).leftJustified(kLevelTextWidth) + QStringLiteral("  "), format);
    }
    format.setForeground(messageColor(entry.level));
    cursor.insertText(entry.message, format);
}

int ShadLogViewPrivate::removeFirst(int count)
{
    Q_Q(ShadLogView);
    int blockCount = 0;
    int removedVisibleCount = 0;
    for (int index = 0; index < count && !_entries.isEmpty(); ++index)
    {
        const Entry entry = _entries.takeFirst();
        if (entry.isVisible)
        {
            blockCount += entry.blockCount;
            ++removedVisibleCount;
        }
    }
    if (removedVisibleCount == 0)
    {
        return 0;
    }
    _visibleCount -= removedVisibleCount;
    QTextCursor cursor(q->document());
    if (_visibleCount == 0)
    {
        cursor.select(QTextCursor::Document);
        cursor.removeSelectedText();
        return 0;
    }
    int lineCount = 0;
    QTextBlock block = q->document()->begin();
    for (int index = 0; index < blockCount && block.isValid(); ++index)
    {
        lineCount += qMax(1, block.lineCount());
        block = block.next();
    }
    cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, blockCount);
    cursor.removeSelectedText();
    return lineCount;
}

void ShadLogViewPrivate::rebuild()
{
    Q_Q(ShadLogView);
    const bool wasFollowing = _isFollowing;
    _isUpdating = true;
    q->setUpdatesEnabled(false);
    QTextCursor cursor(q->document());
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    _visibleCount = 0;
    for (Entry& entry : _entries)
    {
        entry.isVisible = matches(entry);
        if (!entry.isVisible)
        {
            continue;
        }
        insertEntry(cursor, entry, _visibleCount == 0);
        ++_visibleCount;
    }
    cursor.endEditBlock();
    q->setUpdatesEnabled(true);
    _isUpdating = false;
    restoreScroll(wasFollowing, q->verticalScrollBar()->value());
}

void ShadLogViewPrivate::syncFollowing()
{
    Q_Q(ShadLogView);
    const QScrollBar* scrollBar = q->verticalScrollBar();
    _isFollowing = scrollBar->value() >= scrollBar->maximum();
}

void ShadLogViewPrivate::restoreScroll(bool wasFollowing, int value)
{
    Q_Q(ShadLogView);
    if (!(_isAutoScroll && wasFollowing))
    {
        q->verticalScrollBar()->setValue(qMax(0, value));
    }
    restoreFollowing(wasFollowing);
}

void ShadLogViewPrivate::restoreFollowing(bool wasFollowing)
{
    Q_Q(ShadLogView);
    QScrollBar* scrollBar = q->verticalScrollBar();
    if (_isAutoScroll && wasFollowing)
    {
        scrollBar->setValue(scrollBar->maximum());
        _isFollowing = true;
        return;
    }
    syncFollowing();
}
