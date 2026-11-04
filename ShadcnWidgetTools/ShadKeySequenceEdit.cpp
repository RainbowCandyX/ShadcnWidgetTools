#include "ShadKeySequenceEdit.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QHelpEvent>
#include <QKeyCombination>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QSizePolicy>
#include <QStringList>

#include "ShadIcon.h"
#include "ShadKeySequenceEditPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"

namespace
{
    // h-9 px-3，与 ShadInput 一致；按键之间 gap-1，与清除按钮之间 gap-1
    constexpr int kBodyHeight = 36;
    constexpr int kPaddingX = 12;
    constexpr int kKeyGap = 4;
    constexpr int kDefaultWidth = 200;
    constexpr int kClearIconSize = 14;
} // namespace

ShadKeySequenceEdit::ShadKeySequenceEdit(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadKeySequenceEditPrivate())
{
    Q_D(ShadKeySequenceEdit);
    d->q_ptr = this;
    ShadStyleHelper::applyDefaultText(this, "placeholderText", d->_placeholderText, tr("点击后按下快捷键"), [this](const QString& text) { setPlaceholderText(text); });
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // 清除按钮需要悬停态
    setMouseTracking(true);
    // 只录制按键，不需要输入法组字
    setAttribute(Qt::WA_InputMethodEnabled, false);
    // 清除按钮有单独的提示，提示事件由 event 自行处理
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadKeySequenceEdit::ShadKeySequenceEdit(const QKeySequence& sequence, QWidget* parent)
    : ShadKeySequenceEdit(parent)
{
    setKeySequence(sequence);
}

ShadKeySequenceEdit::~ShadKeySequenceEdit()
{
}

void ShadKeySequenceEdit::setKeySequence(const QKeySequence& sequence)
{
    Q_D(ShadKeySequenceEdit);
    d->applySequence(sequence);
}

QKeySequence ShadKeySequenceEdit::getKeySequence() const
{
    Q_D(const ShadKeySequenceEdit);
    return d->_sequence;
}

void ShadKeySequenceEdit::clear()
{
    Q_D(ShadKeySequenceEdit);
    d->applySequence(QKeySequence());
}

void ShadKeySequenceEdit::setPlaceholderText(const QString& text)
{
    Q_D(ShadKeySequenceEdit);
    d->_placeholderText = text;
    update();
}

QString ShadKeySequenceEdit::getPlaceholderText() const
{
    Q_D(const ShadKeySequenceEdit);
    return d->_placeholderText;
}

void ShadKeySequenceEdit::setInvalid(bool invalid)
{
    Q_D(ShadKeySequenceEdit);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
}

bool ShadKeySequenceEdit::isInvalid() const
{
    Q_D(const ShadKeySequenceEdit);
    return d->_isInvalid;
}

bool ShadKeySequenceEdit::isRecording() const
{
    return hasFocus();
}

QSize ShadKeySequenceEdit::sizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(kDefaultWidth + margin, kBodyHeight + margin);
}

QSize ShadKeySequenceEdit::minimumSizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(2 * kPaddingX + 2 * kBodyHeight + margin, kBodyHeight + margin);
}

bool ShadKeySequenceEdit::event(QEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    // 录制时接管所有组合键：接受 ShortcutOverride 后程序中的快捷键不会触发，按键作为普通 KeyPress 交给本组件
    if (event->type() == QEvent::ShortcutOverride && hasFocus())
    {
        const QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        const bool isFocusKey = (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && !(keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier));
        if (!isFocusKey)
        {
            event->accept();
            return true;
        }
    }
    if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        if (d->isClearVisible() && d->clearButtonRect().contains(helpEvent->pos()))
        {
            shadTooltip->showText(this, d->clearButtonRect().toAlignedRect(), tr("清除"));
            return true;
        }
        if (!toolTip().isEmpty())
        {
            shadTooltip->showText(this, rect(), toolTip());
            return true;
        }
        event->ignore();
        return false;
    }
    return QWidget::event(event);
}

void ShadKeySequenceEdit::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    using Private = ShadKeySequenceEditPrivate;
    const int key = event->key();
    const Qt::KeyboardModifiers modifiers = Private::relevantModifiers(event->modifiers());
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
    if (key == 0 || key == Qt::Key_unknown)
    {
        return;
    }
    // 只按下修饰键：实时显示，等待主键
    if (Private::isModifierKey(key))
    {
        d->_pendingModifiers = modifiers;
        update();
        return;
    }
    d->_pendingModifiers = Qt::NoModifier;
    if (modifiers == Qt::NoModifier && key == Qt::Key_Escape)
    {
        // 取消：恢复聚焦前的值并结束录制，失去焦点时发出 editingFinished
        d->applySequence(d->_sequenceBeforeEdit);
        clearFocus();
        return;
    }
    if (modifiers == Qt::NoModifier && (key == Qt::Key_Backspace || key == Qt::Key_Delete))
    {
        d->applySequence(QKeySequence());
        return;
    }
    // 修饰键 + 主键完成一次录制；继续按下其他组合键会替换当前值
    d->applySequence(QKeySequence(QKeyCombination(modifiers, Qt::Key(key))));
    d->_sequenceBeforeEdit = d->_sequence;
    Q_EMIT editingFinished();
}

void ShadKeySequenceEdit::keyReleaseEvent(QKeyEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    using Private = ShadKeySequenceEditPrivate;
    if (!hasFocus())
    {
        QWidget::keyReleaseEvent(event);
        return;
    }
    // 部分平台松开修饰键时事件里仍带着该修饰键，这里手动去掉
    Qt::KeyboardModifiers remaining = Private::relevantModifiers(event->modifiers());
    switch (event->key())
    {
        case Qt::Key_Shift:
            remaining &= ~Qt::ShiftModifier;
            break;
        case Qt::Key_Control:
            remaining &= ~Qt::ControlModifier;
            break;
        case Qt::Key_Alt:
        case Qt::Key_AltGr:
            remaining &= ~Qt::AltModifier;
            break;
        case Qt::Key_Meta:
            remaining &= ~Qt::MetaModifier;
            break;
        default:
            break;
    }
    if (d->_pendingModifiers != Qt::NoModifier)
    {
        d->_pendingModifiers = remaining;
        update();
    }
    event->accept();
}

void ShadKeySequenceEdit::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    d->_sequenceBeforeEdit = d->_sequence;
    d->_pendingModifiers = Qt::NoModifier;
    QWidget::focusInEvent(event);
    update();
}

void ShadKeySequenceEdit::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    d->_pendingModifiers = Qt::NoModifier;
    QWidget::focusOutEvent(event);
    update();
    // 弹出窗口（例如提示）抢走焦点不算结束编辑
    if (event->reason() != Qt::PopupFocusReason)
    {
        Q_EMIT editingFinished();
    }
}

void ShadKeySequenceEdit::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    if (event->button() == Qt::LeftButton && d->isClearVisible() && d->clearButtonRect().contains(event->position()))
    {
        d->applySequence(QKeySequence());
        d->_sequenceBeforeEdit = d->_sequence;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadKeySequenceEdit::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    d->updateClearHover(event->position().toPoint());
    QWidget::mouseMoveEvent(event);
}

void ShadKeySequenceEdit::leaveEvent(QEvent* event)
{
    Q_D(ShadKeySequenceEdit);
    d->updateClearHover(QPoint(-1, -1));
    QWidget::leaveEvent(event);
}

void ShadKeySequenceEdit::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange || event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        ShadStyleHelper::applyDefaultText(this, "placeholderText", getPlaceholderText(), tr("点击后按下快捷键"), [this](const QString& text) { setPlaceholderText(text); });
        update();
    }
    QWidget::changeEvent(event);
}

void ShadKeySequenceEdit::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadKeySequenceEdit);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isFocused = hasFocus();
    const Qt::LayoutDirection direction = layoutDirection();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 外框与 ShadInput 相同：聚焦即显示焦点环（录制中），错误态常驻 destructive 焦点环
    drawShadowXs(&painter, bodyRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (d->_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (isFocused || d->_isInvalid)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_isInvalid));
    }

    // 内容区：按 LTR 计算，RTL 时镜像；有清除按钮时给它让出位置
    const bool isClearVisible = d->isClearVisible();
    const qreal contentLeft = bodyRect.left() + 1 + kPaddingX;
    qreal contentRight = bodyRect.right() - 1 - kPaddingX;
    if (isClearVisible)
    {
        const QRectF logicalClear = visualRect(direction, QRectF(rect()), d->clearButtonRect());
        contentRight = logicalClear.left() - kKeyGap;
    }
    const QStringList keys = d->displayKeys();
    if (keys.isEmpty())
    {
        const QString text = isFocused ? tr("请按下组合键…") : d->_placeholderText;
        const QRectF textRect(contentLeft, bodyRect.top(), qMax<qreal>(0.0, contentRight - contentLeft), bodyRect.height());
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(visualRect(direction, QRectF(rect()), textRect), visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(font(), text, textRect.width()));
    }
    else
    {
        qreal x = contentLeft;
        for (const QString& key : keys)
        {
            const QSize keySize = kbdSize(font(), key);
            // 放不下的按键不再绘制
            if (x + keySize.width() > contentRight)
            {
                break;
            }
            const QRectF keyRect(x, bodyRect.center().y() - keySize.height() / 2.0, keySize.width(), keySize.height());
            drawKbd(&painter, visualRect(direction, QRectF(rect()), keyRect), font(), key, false);
            x += keySize.width() + kKeyGap;
        }
    }

    if (isClearVisible)
    {
        // 与 ShadInput 的尾部按钮相同：ghost 风格，悬停时 bg-accent（暗色 accent/50）
        const QRectF buttonRect = d->clearButtonRect();
        if (d->_isClearHovered)
        {
            const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
            fillRoundedRect(&painter, buttonRect, qMax(0, shadTheme->getBaseRadius() - 5), shadTheme->isDark() ? withAlpha(accent, 0.5) : accent);
        }
        const QRectF iconRect(buttonRect.center().x() - kClearIconSize / 2.0, buttonRect.center().y() - kClearIconSize / 2.0, kClearIconSize, kClearIconSize);
        ShadIcon::paint(&painter, iconRect, ShadIconType::X, shadTheme->getColor(ShadThemeType::MutedForeground), kClearIconSize);
    }
}
