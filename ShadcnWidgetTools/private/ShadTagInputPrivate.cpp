#include "ShadTagInputPrivate.h"

#include <QColor>
#include <QKeyEvent>
#include <QPainter>
#include <QRectF>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadStyleHelper.h"
#include "ShadTagInput.h"
#include "ShadTheme.h"

ShadTagInputPrivate::ShadTagInputPrivate()
{
}

ShadTagInputPrivate::~ShadTagInputPrivate()
{
}

QFont ShadTagInputPrivate::tagFont() const
{
    Q_Q(const ShadTagInput);
    QFont font = q->font();
    font.setPixelSize(kTagFontSize);
    font.setWeight(QFont::Medium);
    return font;
}

bool ShadTagInputPrivate::isCloseVisible() const
{
    Q_Q(const ShadTagInput);
    return !_isReadOnly && q->isEnabled();
}

ShadTagInputPrivate::Layout ShadTagInputPrivate::computeLayout(int width) const
{
    const QFont font = tagFont();
    const int tagChrome = kTagPaddingX + (isCloseVisible() ? kCloseGap + kCloseSize + kCloseEndPadding : kTagPaddingX);
    const int left = ShadStyleHelper::kFocusRingWidth + kPaddingX;
    const int right = qMax(left + 1, width - left);
    Layout layout;
    layout.tagRects.reserve(_tags.size());
    int x = left;
    int y = ShadStyleHelper::kFocusRingWidth + kPaddingY;
    for (const QString& tag : _tags)
    {
        const int tagWidth = qMin(right - left, ShadStyleHelper::textWidth(font, tag) + tagChrome);
        if (x > left && x + tagWidth > right)
        {
            x = left;
            y += kTagHeight + kTagGap;
        }
        layout.tagRects.append(QRect(x, y, tagWidth, kTagHeight));
        x += tagWidth + kTagGap;
    }
    if (!_isReadOnly)
    {
        if (x > left && right - x < kMinInputWidth)
        {
            x = left;
            y += kTagHeight + kTagGap;
        }
        const int inputLeft = x == left ? left + kInputPaddingX : x;
        layout.inputRect = QRect(inputLeft, y, qMax(1, right - inputLeft), kTagHeight);
    }
    layout.height = y + kTagHeight + kPaddingY + ShadStyleHelper::kFocusRingWidth;
    return layout;
}

void ShadTagInputPrivate::relayout()
{
    Q_Q(ShadTagInput);
    const Layout layout = computeLayout(q->width());
    _tagRects = layout.tagRects;
    if (!_isReadOnly)
    {
        _input->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), layout.inputRect));
    }
    if (_layoutHeight != layout.height)
    {
        _layoutHeight = layout.height;
        q->updateGeometry();
    }
    q->update();
}

QRect ShadTagInputPrivate::visualTagRect(int index) const
{
    Q_Q(const ShadTagInput);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), _tagRects.at(index));
}

QRect ShadTagInputPrivate::closeRect(int index) const
{
    Q_Q(const ShadTagInput);
    const QRect tagRect = _tagRects.at(index);
    const QRect logicalRect(tagRect.right() + 1 - kCloseEndPadding - kCloseSize, tagRect.top() + (tagRect.height() - kCloseSize) / 2, kCloseSize, kCloseSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect);
}

int ShadTagInputPrivate::tagIndexAt(const QPoint& pos) const
{
    for (int index = 0; index < int(_tagRects.size()); ++index)
    {
        if (visualTagRect(index).contains(pos))
        {
            return index;
        }
    }
    return -1;
}

void ShadTagInputPrivate::paintTags(QPainter* painter) const
{
    Q_Q(const ShadTagInput);
    using namespace ShadStyleHelper;
    const QFont font = tagFont();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Small);
    const QColor background = shadTheme->getColor(ShadThemeType::Secondary);
    const QColor textColor = shadTheme->getColor(ShadThemeType::SecondaryForeground);
    const bool hasClose = isCloseVisible();
    const int textChrome = kTagPaddingX + (hasClose ? kCloseGap + kCloseSize + kCloseEndPadding : kTagPaddingX);
    for (int index = 0; index < int(_tagRects.size()); ++index)
    {
        const QRect logicalRect = _tagRects.at(index);
        const QRectF tagRect = visualTagRect(index);
        fillRoundedRect(painter, tagRect, radius, background);
        if (index == _activeIndex)
        {
            strokeRoundedRect(painter, tagRect, radius, shadTheme->getColor(ShadThemeType::Ring));
        }
        const int textWidth = qMax(0, logicalRect.width() - textChrome);
        const QRect textRect = visualRect(q->layoutDirection(), q->rect(), QRect(logicalRect.left() + kTagPaddingX, logicalRect.top(), textWidth, logicalRect.height()));
        painter->setFont(font);
        painter->setPen(textColor);
        painter->drawText(textRect, visualAlignment(q->layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), elideText(font, _tags.at(index), textWidth));
        if (!hasClose)
        {
            continue;
        }
        const QRectF buttonRect = closeRect(index);
        const bool isHovered = index == _hoveredCloseIndex;
        if (isHovered)
        {
            fillRoundedRect(painter, buttonRect, radius, withAlpha(textColor, 0.1));
        }
        ShadIcon::paint(painter, buttonRect, ShadIconType::X, isHovered ? textColor : shadTheme->getColor(ShadThemeType::MutedForeground), kCloseIconSize);
    }
}

bool ShadTagInputPrivate::isSeparator(QChar ch) const
{
    return ch == u'\n' || ch == u'\r' || ch == QChar::LineSeparator || ch == QChar::ParagraphSeparator || _separators.contains(ch);
}

bool ShadTagInputPrivate::canAccept(const QString& tag) const
{
    if (_maxTags > 0 && _tags.size() >= _maxTags)
    {
        return false;
    }
    return _isDuplicatesAllowed || !_tags.contains(tag);
}

QStringList ShadTagInputPrivate::normalized(const QStringList& tags) const
{
    QStringList result;
    for (const QString& tag : tags)
    {
        if (_maxTags > 0 && result.size() >= _maxTags)
        {
            break;
        }
        const QString text = tag.trimmed();
        if (text.isEmpty() || (!_isDuplicatesAllowed && result.contains(text)))
        {
            continue;
        }
        result.append(text);
    }
    return result;
}

void ShadTagInputPrivate::applyConstraints()
{
    Q_Q(ShadTagInput);
    q->setTags(_tags);
}

void ShadTagInputPrivate::removeAt(int index)
{
    Q_Q(ShadTagInput);
    if (index < 0 || index >= _tags.size())
    {
        return;
    }
    const QString tag = _tags.takeAt(index);
    _activeIndex = -1;
    _hoveredCloseIndex = -1;
    _pressedCloseIndex = -1;
    updatePlaceholder();
    relayout();
    Q_EMIT q->tagRemoved(tag);
    Q_EMIT q->tagsChanged(_tags);
}

void ShadTagInputPrivate::commitText()
{
    Q_Q(ShadTagInput);
    const QString text = _input->text();
    _input->clear();
    q->addTag(text);
}

void ShadTagInputPrivate::handleTextEdited(const QString& text)
{
    Q_Q(ShadTagInput);
    setActiveIndex(-1);
    bool hasSeparator = false;
    for (const QChar ch : text)
    {
        if (isSeparator(ch))
        {
            hasSeparator = true;
            break;
        }
    }
    if (!hasSeparator)
    {
        return;
    }
    QString pending;
    for (const QChar ch : text)
    {
        if (!isSeparator(ch))
        {
            pending.append(ch);
            continue;
        }
        q->addTag(pending);
        pending.clear();
    }
    _input->setText(pending);
}

bool ShadTagInputPrivate::handleKeyPress(QKeyEvent* event)
{
    Q_Q(ShadTagInput);
    const int key = event->key();
    const int tagCount = int(_tags.size());
    if (key == Qt::Key_Return || key == Qt::Key_Enter)
    {
        if (_input->text().trimmed().isEmpty())
        {
            return false;
        }
        commitText();
        return true;
    }
    if (key == Qt::Key_Backspace && _input->text().isEmpty() && tagCount > 0)
    {
        if (_activeIndex < 0)
        {
            setActiveIndex(tagCount - 1);
        }
        else
        {
            removeAt(_activeIndex);
        }
        return true;
    }
    if (key == Qt::Key_Delete && _activeIndex >= 0)
    {
        const int index = _activeIndex;
        removeAt(index);
        setActiveIndex(index < _tags.size() ? index : -1);
        return true;
    }
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    const int previousKey = isRightToLeft ? Qt::Key_Right : Qt::Key_Left;
    const int nextKey = isRightToLeft ? Qt::Key_Left : Qt::Key_Right;
    const bool hasModifier = (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) != Qt::NoModifier;
    if (key == previousKey && !hasModifier && tagCount > 0 && _input->cursorPosition() == 0 && !_input->hasSelectedText())
    {
        setActiveIndex(_activeIndex < 0 ? tagCount - 1 : qMax(0, _activeIndex - 1));
        return true;
    }
    if (key == nextKey && _activeIndex >= 0)
    {
        setActiveIndex(_activeIndex + 1 < tagCount ? _activeIndex + 1 : -1);
        return true;
    }
    if (key == Qt::Key_Escape && _activeIndex >= 0)
    {
        setActiveIndex(-1);
        return true;
    }
    if (!event->text().isEmpty())
    {
        setActiveIndex(-1);
    }
    return false;
}

void ShadTagInputPrivate::setActiveIndex(int index)
{
    Q_Q(ShadTagInput);
    const int activeIndex = index >= 0 && index < _tags.size() ? index : -1;
    if (_activeIndex == activeIndex)
    {
        return;
    }
    _activeIndex = activeIndex;
    q->update();
}

void ShadTagInputPrivate::updateHover(const QPoint& pos)
{
    Q_Q(ShadTagInput);
    const int tagIndex = tagIndexAt(pos);
    q->setCursor(tagIndex >= 0 || _isReadOnly ? Qt::ArrowCursor : Qt::IBeamCursor);
    const int hoveredIndex = tagIndex >= 0 && isCloseVisible() && closeRect(tagIndex).contains(pos) ? tagIndex : -1;
    if (_hoveredCloseIndex == hoveredIndex)
    {
        return;
    }
    _hoveredCloseIndex = hoveredIndex;
    q->update();
}

void ShadTagInputPrivate::updatePlaceholder()
{
    _input->setPlaceholderText(_tags.isEmpty() ? _placeholderText : QString());
}
