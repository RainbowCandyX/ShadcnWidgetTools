#include "ShadInputOtpPrivate.h"

#include <QChar>
#include <QTimer>

#include "ShadInputOtp.h"
#include "ShadStyleHelper.h"

ShadInputOtpPrivate::ShadInputOtpPrivate()
{
}

ShadInputOtpPrivate::~ShadInputOtpPrivate()
{
}

ShadInputOtpPrivate::Geometry ShadInputOtpPrivate::geometry() const
{
    Q_Q(const ShadInputOtp);
    // 按 LTR 从起始侧排列，RTL 时在组件内整体镜像
    Geometry result;
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRect bounds = q->rect();
    int x = margin;
    for (int group = 0; group < _groups.size(); ++group)
    {
        const int groupStart = x;
        for (int slot = 0; slot < _groups.at(group); ++slot)
        {
            result.slotRects.append(ShadStyleHelper::visualRect(direction, bounds, QRect(x, margin, _slotSize.width(), _slotSize.height())));
            x += _slotSize.width();
        }
        result.groups.append(ShadStyleHelper::visualRect(direction, bounds, QRect(groupStart, margin, x - groupStart, _slotSize.height())));
        if (group + 1 < _groups.size())
        {
            x += kGap;
            const QRect separatorRect(x, margin + (_slotSize.height() - kSeparatorSize) / 2, kSeparatorSize, kSeparatorSize);
            result.separators.append(ShadStyleHelper::visualRect(direction, bounds, separatorRect));
            x += kSeparatorSize + kGap;
        }
    }
    return result;
}

int ShadInputOtpPrivate::maxLength() const
{
    int length = 0;
    for (int size : _groups)
    {
        length += size;
    }
    return length;
}

bool ShadInputOtpPrivate::isGroupStart(int index) const
{
    int start = 0;
    for (int size : _groups)
    {
        if (index == start)
        {
            return true;
        }
        start += size;
    }
    return false;
}

bool ShadInputOtpPrivate::isGroupEnd(int index) const
{
    int end = -1;
    for (int size : _groups)
    {
        end += size;
        if (index == end)
        {
            return true;
        }
    }
    return false;
}

int ShadInputOtpPrivate::activeIndex() const
{
    Q_Q(const ShadInputOtp);
    const int length = maxLength();
    if (!q->hasFocus() || length <= 0)
    {
        return -1;
    }
    return qMin(_cursor, length - 1);
}

int ShadInputOtpPrivate::slotAt(const QPoint& pos) const
{
    const QList<QRect> slotRects = geometry().slotRects;
    for (int index = 0; index < slotRects.size(); ++index)
    {
        if (slotRects.at(index).contains(pos))
        {
            return index;
        }
    }
    return -1;
}

bool ShadInputOtpPrivate::isAcceptable(const QString& value) const
{
    if (value.isEmpty() || !_pattern.isValid() || _pattern.pattern().isEmpty())
    {
        return true;
    }
    return _pattern.match(value).hasMatch();
}

bool ShadInputOtpPrivate::insertText(const QString& text)
{
    const int length = maxLength();
    QString value = _value;
    int cursor = _cursor;
    bool isChanged = false;
    for (const QChar rawCharacter : text)
    {
        if (length <= 0 || rawCharacter.isSpace() || !rawCharacter.isPrint())
        {
            continue;
        }
        const QChar character = _isUpperCase ? rawCharacter.toUpper() : rawCharacter;
        // 填满后当前格为最后一格，再输入时替换最后一个字符
        const int index = qMin(cursor, length - 1);
        QString candidate = value;
        if (index < candidate.size())
        {
            candidate[index] = character;
        }
        else
        {
            candidate.append(character);
        }
        if (!isAcceptable(candidate))
        {
            continue;
        }
        value = candidate;
        cursor = index + 1;
        isChanged = true;
        // 粘贴的文字超出总位数时截断
        if (cursor >= length)
        {
            break;
        }
    }
    if (isChanged)
    {
        commit(value, cursor, true);
    }
    return isChanged;
}

void ShadInputOtpPrivate::removeBefore()
{
    if (_cursor <= 0 || _value.isEmpty())
    {
        return;
    }
    QString value = _value;
    value.remove(_cursor - 1, 1);
    commit(value, _cursor - 1, false);
}

void ShadInputOtpPrivate::removeAt()
{
    if (_cursor >= _value.size())
    {
        return;
    }
    QString value = _value;
    value.remove(_cursor, 1);
    commit(value, _cursor, false);
}

void ShadInputOtpPrivate::commit(const QString& value, int cursor, bool canComplete)
{
    Q_Q(ShadInputOtp);
    const bool isChanged = value != _value;
    _value = value;
    _cursor = qBound(0, cursor, int(_value.size()));
    restartCaret();
    q->update();
    if (!isChanged)
    {
        return;
    }
    Q_EMIT q->valueChanged(_value);
    if (canComplete && _value.size() == maxLength())
    {
        Q_EMIT q->completed(_value);
    }
}

void ShadInputOtpPrivate::setCursor(int cursor)
{
    Q_Q(ShadInputOtp);
    _cursor = qBound(0, cursor, int(_value.size()));
    restartCaret();
    q->update();
}

qreal ShadInputOtpPrivate::caretOpacity() const
{
    const qreal progress = static_cast<qreal>(_caretClock.elapsed() % kCaretPeriodMs) / kCaretPeriodMs;
    if (progress < 0.2)
    {
        return 1.0 - progress / 0.2;
    }
    if (progress < 0.5)
    {
        return 0.0;
    }
    if (progress < 0.7)
    {
        return (progress - 0.5) / 0.2;
    }
    return 1.0;
}

void ShadInputOtpPrivate::updateCaretTimer()
{
    Q_Q(ShadInputOtp);
    const int active = activeIndex();
    const bool isCaretShown = q->isEnabled() && active >= 0 && active >= _value.size();
    if (isCaretShown && !_caretTimer->isActive())
    {
        _caretTimer->start(kCaretTickMs);
    }
    else if (!isCaretShown && _caretTimer->isActive())
    {
        _caretTimer->stop();
    }
}

void ShadInputOtpPrivate::restartCaret()
{
    _caretClock.restart();
    updateCaretTimer();
}
