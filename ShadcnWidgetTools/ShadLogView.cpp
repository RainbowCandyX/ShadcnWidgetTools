#include "ShadLogView.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QKeySequence>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTextCursor>
#include <QTextOption>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadLogViewPrivate.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadLogView::ShadLogView(QWidget* parent)
    : QPlainTextEdit(parent), d_ptr(new ShadLogViewPrivate())
{
    Q_D(ShadLogView);
    d->q_ptr = this;
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(ShadLogViewPrivate::kFrameInset);
    setViewportMargins(ShadLogViewPrivate::kPaddingX, ShadLogViewPrivate::kPaddingY, ShadLogViewPrivate::kPaddingX, ShadLogViewPrivate::kPaddingY);
    viewport()->setAutoFillBackground(false);
    ShadScrollBar* verticalBar = new ShadScrollBar(Qt::Vertical, this);
    verticalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadLogViewPrivate::kFrameInset);
    setVerticalScrollBar(verticalBar);
    ShadScrollBar* horizontalBar = new ShadScrollBar(Qt::Horizontal, this);
    horizontalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadLogViewPrivate::kFrameInset);
    setHorizontalScrollBar(horizontalBar);
    setReadOnly(true);
    setUndoRedoEnabled(false);
    setTextInteractionFlags(Qt::TextSelectableByMouse);
    setFocusPolicy(Qt::StrongFocus);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->applyFont();
    d->applyThemeColors();
    connect(verticalBar, &QScrollBar::valueChanged, this, [d]()
    {
        if (!d->_isUpdating)
        {
            d->syncFollowing();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]()
    {
        d->applyThemeColors();
        d->rebuild();
    });
}

ShadLogView::~ShadLogView()
{
}

void ShadLogView::append(ShadLogViewType::Level level, const QString& message)
{
    Q_D(ShadLogView);
    d->appendEntry(level, message, QDateTime::currentDateTimeUtc());
}

void ShadLogView::append(ShadLogViewType::Level level, const QString& message, const QDateTime& time)
{
    Q_D(ShadLogView);
    d->appendEntry(level, message, time);
}

int ShadLogView::count() const
{
    Q_D(const ShadLogView);
    return int(d->_entries.size());
}

int ShadLogView::visibleCount() const
{
    Q_D(const ShadLogView);
    return d->_visibleCount;
}

void ShadLogView::setMaxEntryCount(int count)
{
    Q_D(ShadLogView);
    d->_maxEntryCount = qMax(0, count);
    const int excess = d->_maxEntryCount > 0 ? int(d->_entries.size()) - d->_maxEntryCount : 0;
    if (excess <= 0)
    {
        return;
    }
    d->_entries.remove(0, excess);
    d->rebuild();
    Q_EMIT countChanged(int(d->_entries.size()));
}

int ShadLogView::getMaxEntryCount() const
{
    Q_D(const ShadLogView);
    return d->_maxEntryCount;
}

void ShadLogView::setAutoScroll(bool autoScroll)
{
    Q_D(ShadLogView);
    if (d->_isAutoScroll == autoScroll)
    {
        return;
    }
    d->_isAutoScroll = autoScroll;
    if (autoScroll)
    {
        scrollToBottom();
    }
}

bool ShadLogView::isAutoScroll() const
{
    Q_D(const ShadLogView);
    return d->_isAutoScroll;
}

void ShadLogView::setWordWrap(bool wordWrap)
{
    Q_D(ShadLogView);
    if (isWordWrap() == wordWrap)
    {
        return;
    }
    const bool wasFollowing = d->_isFollowing;
    d->_isUpdating = true;
    setLineWrapMode(wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    d->_isUpdating = false;
    d->restoreFollowing(wasFollowing);
}

bool ShadLogView::isWordWrap() const
{
    return lineWrapMode() == QPlainTextEdit::WidgetWidth;
}

void ShadLogView::setTimestampVisible(bool visible)
{
    Q_D(ShadLogView);
    if (d->_isTimestampVisible == visible)
    {
        return;
    }
    d->_isTimestampVisible = visible;
    d->rebuild();
}

bool ShadLogView::isTimestampVisible() const
{
    Q_D(const ShadLogView);
    return d->_isTimestampVisible;
}

void ShadLogView::setLevelVisible(bool visible)
{
    Q_D(ShadLogView);
    if (d->_isLevelVisible == visible)
    {
        return;
    }
    d->_isLevelVisible = visible;
    d->rebuild();
}

bool ShadLogView::isLevelVisible() const
{
    Q_D(const ShadLogView);
    return d->_isLevelVisible;
}

void ShadLogView::setTimestampFormat(const QString& format)
{
    Q_D(ShadLogView);
    if (d->_timestampFormat == format)
    {
        return;
    }
    d->_timestampFormat = format;
    d->rebuild();
}

QString ShadLogView::getTimestampFormat() const
{
    Q_D(const ShadLogView);
    return d->_timestampFormat;
}

void ShadLogView::setMinimumLevel(ShadLogViewType::Level level)
{
    Q_D(ShadLogView);
    if (d->_minimumLevel == level)
    {
        return;
    }
    d->_minimumLevel = level;
    d->rebuild();
}

ShadLogViewType::Level ShadLogView::getMinimumLevel() const
{
    Q_D(const ShadLogView);
    return d->_minimumLevel;
}

void ShadLogView::setFilterText(const QString& text)
{
    Q_D(ShadLogView);
    if (d->_filterText == text)
    {
        return;
    }
    d->_filterText = text;
    d->rebuild();
}

QString ShadLogView::getFilterText() const
{
    Q_D(const ShadLogView);
    return d->_filterText;
}

QSize ShadLogView::sizeHint() const
{
    return QSize(ShadLogViewPrivate::kDefaultWidth, ShadLogViewPrivate::kDefaultHeight + 2 * ShadStyleHelper::kFocusRingWidth);
}

QSize ShadLogView::minimumSizeHint() const
{
    return QSize(QPlainTextEdit::minimumSizeHint().width(), ShadLogViewPrivate::kMinHeight + 2 * ShadStyleHelper::kFocusRingWidth);
}

void ShadLogView::debug(const QString& message)
{
    append(ShadLogViewType::Debug, message);
}

void ShadLogView::info(const QString& message)
{
    append(ShadLogViewType::Info, message);
}

void ShadLogView::success(const QString& message)
{
    append(ShadLogViewType::Success, message);
}

void ShadLogView::warning(const QString& message)
{
    append(ShadLogViewType::Warning, message);
}

void ShadLogView::error(const QString& message)
{
    append(ShadLogViewType::Error, message);
}

void ShadLogView::clear()
{
    Q_D(ShadLogView);
    if (d->_entries.isEmpty())
    {
        return;
    }
    d->_entries.clear();
    d->_isFollowing = true;
    d->rebuild();
    Q_EMIT countChanged(0);
}

void ShadLogView::scrollToBottom()
{
    Q_D(ShadLogView);
    QScrollBar* scrollBar = verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    d->_isFollowing = true;
}

bool ShadLogView::event(QEvent* event)
{
    Q_D(ShadLogView);
    if (event->type() == QEvent::Paint)
    {
        d->paintFrame();
        return true;
    }
    if (event->type() == QEvent::LanguageChange && d->_contextMenu != nullptr)
    {
        d->_contextMenu->deleteLater();
        d->_contextMenu = nullptr;
        d->_copyAction = nullptr;
        d->_clearAction = nullptr;
    }
    return QPlainTextEdit::event(event);
}

void ShadLogView::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadLogView);
    QPlainTextEdit::focusInEvent(event);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
}

void ShadLogView::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadLogView);
    QPlainTextEdit::focusOutEvent(event);
    d->_isFocusVisible = false;
    update();
}

void ShadLogView::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadLogView);
    const bool wasFollowing = d->_isFollowing;
    d->_isUpdating = true;
    QPlainTextEdit::resizeEvent(event);
    d->_isUpdating = false;
    d->restoreFollowing(wasFollowing);
}

void ShadLogView::showEvent(QShowEvent* event)
{
    Q_D(ShadLogView);
    const bool wasFollowing = d->_isFollowing;
    d->_isUpdating = true;
    QPlainTextEdit::showEvent(event);
    d->_isUpdating = false;
    d->restoreFollowing(wasFollowing);
}

void ShadLogView::contextMenuEvent(QContextMenuEvent* event)
{
    Q_D(ShadLogView);
    if (d->_contextMenu == nullptr)
    {
        d->_contextMenu = new ShadDropdownMenu(this);
        d->_copyAction = d->_contextMenu->addItem(tr("复制"), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Copy)));
        QAction* selectAllAction = d->_contextMenu->addItem(tr("全选"), ShadIconType::TextSelect, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::SelectAll)));
        d->_contextMenu->addSeparator();
        d->_clearAction = d->_contextMenu->addItem(tr("清空"), ShadIconType::Trash2);
        connect(d->_copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);
        connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);
        connect(d->_clearAction, &QAction::triggered, this, &ShadLogView::clear);
    }
    d->_copyAction->setEnabled(textCursor().hasSelection());
    d->_clearAction->setEnabled(!d->_entries.isEmpty());
    d->_contextMenu->popupAt(event->globalPos());
    event->accept();
}
