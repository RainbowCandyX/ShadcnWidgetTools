#include "ShadKeySequenceEditPrivate.h"

#include <QKeyCombination>

#include "ShadKbd.h"
#include "ShadKeySequenceEdit.h"
#include "ShadStyleHelper.h"

namespace
{
    // 清除按钮 size-6，距边框 6px，与 ShadInput 的尾部按钮一致
    constexpr int kClearButtonSize = 24;
    constexpr int kClearRightGap = 6;
} // namespace

ShadKeySequenceEditPrivate::ShadKeySequenceEditPrivate()
{
}

ShadKeySequenceEditPrivate::~ShadKeySequenceEditPrivate()
{
}

Qt::KeyboardModifiers ShadKeySequenceEditPrivate::relevantModifiers(Qt::KeyboardModifiers modifiers)
{
    return modifiers & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
}

bool ShadKeySequenceEditPrivate::isModifierKey(int key)
{
    switch (key)
    {
        case Qt::Key_Shift:
        case Qt::Key_Control:
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
        case Qt::Key_Meta:
        case Qt::Key_Super_L:
        case Qt::Key_Super_R:
        case Qt::Key_Hyper_L:
        case Qt::Key_Hyper_R:
        case Qt::Key_CapsLock:
            return true;
        default:
            break;
    }
    return false;
}

QRectF ShadKeySequenceEditPrivate::clearButtonRect() const
{
    Q_Q(const ShadKeySequenceEdit);
    // 按 LTR 计算，RTL 时镜像到左侧
    const int inset = 1 + ShadStyleHelper::kFocusRingWidth;
    const QRectF logicalRect(q->width() - inset - kClearRightGap - kClearButtonSize, (q->height() - kClearButtonSize) / 2.0, kClearButtonSize, kClearButtonSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalRect);
}

bool ShadKeySequenceEditPrivate::isClearVisible() const
{
    Q_Q(const ShadKeySequenceEdit);
    return !_sequence.isEmpty() && q->isEnabled();
}

QStringList ShadKeySequenceEditPrivate::displayKeys() const
{
    Q_Q(const ShadKeySequenceEdit);
    // 录制中按住修饰键时先显示修饰键，松开前还没有主键
    if (q->hasFocus() && _pendingModifiers != Qt::NoModifier)
    {
        return ShadKbdGroup::keyTexts(QKeyCombination(_pendingModifiers, Qt::Key(0)));
    }
    return ShadKbdGroup::keyTexts(_sequence);
}

void ShadKeySequenceEditPrivate::applySequence(const QKeySequence& sequence)
{
    Q_Q(ShadKeySequenceEdit);
    // 只保留第一段组合键
    const QKeySequence single = sequence.isEmpty() ? QKeySequence() : QKeySequence(sequence[0]);
    if (single == _sequence)
    {
        q->update();
        return;
    }
    _sequence = single;
    _isClearHovered = false;
    q->update();
    Q_EMIT q->keySequenceChanged(_sequence);
}

void ShadKeySequenceEditPrivate::updateClearHover(const QPoint& pos)
{
    Q_Q(ShadKeySequenceEdit);
    const bool isHovered = isClearVisible() && clearButtonRect().contains(pos);
    if (isHovered == _isClearHovered)
    {
        return;
    }
    _isClearHovered = isHovered;
    q->setCursor(isHovered ? Qt::PointingHandCursor : Qt::ArrowCursor);
    q->update();
}
