#include "ShadKbd.h"

#include <QAction>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QKeyCombination>
#include <QLayoutItem>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QShortcut>
#include <QSizePolicy>

#include "ShadKbdPrivate.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // KbdGroup 的 gap-1
    constexpr int kGroupGap = 4;

    // 一段组合键拆成的按键文字：修饰键在前，主键在后
    QStringList combinationTexts(QKeyCombination combination)
    {
        QStringList texts;
        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
#ifdef Q_OS_MACOS
        // macOS 习惯顺序 ⌃ ⌥ ⇧ ⌘；Qt 在 macOS 上 Qt::MetaModifier 对应 Control 键，Qt::ControlModifier 对应 Command 键
        if (modifiers.testFlag(Qt::MetaModifier))
        {
            texts.append(QStringLiteral("⌃"));
        }
        if (modifiers.testFlag(Qt::AltModifier))
        {
            texts.append(QStringLiteral("⌥"));
        }
        if (modifiers.testFlag(Qt::ShiftModifier))
        {
            texts.append(QStringLiteral("⇧"));
        }
        if (modifiers.testFlag(Qt::ControlModifier))
        {
            texts.append(QStringLiteral("⌘"));
        }
#else
        if (modifiers.testFlag(Qt::ControlModifier))
        {
            texts.append(QStringLiteral("Ctrl"));
        }
        if (modifiers.testFlag(Qt::AltModifier))
        {
            texts.append(QStringLiteral("Alt"));
        }
        if (modifiers.testFlag(Qt::ShiftModifier))
        {
            texts.append(QStringLiteral("Shift"));
        }
        if (modifiers.testFlag(Qt::MetaModifier))
        {
#ifdef Q_OS_WIN
            texts.append(QStringLiteral("Win"));
#else
            texts.append(QStringLiteral("Meta"));
#endif
        }
#endif
        // 主键使用 Qt 的本地化名称，例如 "K"、"F5"；macOS 上 Qt 已把 Esc、Tab、退格、方向键、翻页等换成 ⎋ ⇥ ⌫ ↑ ⇞ 等符号
        const Qt::Key key = combination.key();
        if (key == Qt::Key_unknown || key == Qt::Key(0))
        {
            return texts;
        }
#ifdef Q_OS_MACOS
        // Qt 把 Return 显示为 ↵，macOS 菜单与键盘使用 ↩
        if (key == Qt::Key_Return)
        {
            texts.append(QStringLiteral("↩"));
            return texts;
        }
#endif
        texts.append(QKeySequence(key).toString(QKeySequence::NativeText));
        return texts;
    }
} // namespace

ShadKbdPrivate::ShadKbdPrivate()
{
}

ShadKbdPrivate::~ShadKbdPrivate()
{
}

ShadKbdGroupPrivate::ShadKbdGroupPrivate()
{
}

ShadKbdGroupPrivate::~ShadKbdGroupPrivate()
{
}

void ShadKbdGroupPrivate::rebuildKeys()
{
    Q_Q(ShadKbdGroup);
    q->clear();
    for (int index = 0; index < _keySequence.count(); ++index)
    {
        // 多段组合键之间显示逗号，与 QKeySequence::toString 一致
        if (index > 0)
        {
            q->addText(QStringLiteral(","));
        }
        for (const QString& text : combinationTexts(_keySequence[index]))
        {
            q->addKey(text);
        }
    }
}

void ShadKbdGroupPrivate::updateShortcut()
{
    Q_Q(ShadKbdGroup);
    const bool isNeeded = _isShortcutEnabled && _boundAction.isNull() && !_keySequence.isEmpty();
    if (!isNeeded)
    {
        delete _shortcut;
        _shortcut = nullptr;
        return;
    }
    if (_shortcut == nullptr)
    {
        // 以按键组为父对象：所在窗口激活且按键组可见、可用时生效
        _shortcut = new QShortcut(q);
        _shortcut->setContext(Qt::WindowShortcut);
        QObject::connect(_shortcut, &QShortcut::activated, q, &ShadKbdGroup::activated);
    }
    _shortcut->setKey(_keySequence);
}

ShadKbd::ShadKbd(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadKbdPrivate())
{
    Q_D(ShadKbd);
    d->q_ptr = this;
    // pointer-events-none：鼠标事件交给下层组件（例如输入组聚焦输入框）
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadKbd::ShadKbd(const QString& text, QWidget* parent)
    : ShadKbd(parent)
{
    setText(text);
}

ShadKbd::~ShadKbd()
{
}

void ShadKbd::setText(const QString& text)
{
    Q_D(ShadKbd);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    setAccessibleName(text);
    updateGeometry();
    update();
}

QString ShadKbd::getText() const
{
    Q_D(const ShadKbd);
    return d->_text;
}

QSize ShadKbd::sizeHint() const
{
    Q_D(const ShadKbd);
    return ShadStyleHelper::kbdSize(font(), d->_text);
}

QSize ShadKbd::minimumSizeHint() const
{
    return sizeHint();
}

void ShadKbd::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QWidget::changeEvent(event);
}

void ShadKbd::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadKbd);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    // 尺寸由布局决定时按 sizeHint 居中绘制，保持 h-5
    const QSize size = sizeHint();
    const QRectF keyRect((width() - size.width()) / 2.0, (height() - size.height()) / 2.0, size.width(), size.height());
    ShadStyleHelper::drawKbd(&painter, keyRect, font(), d->_text, false);
}

ShadKbdGroup::ShadKbdGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadKbdGroupPrivate())
{
    Q_D(ShadKbdGroup);
    d->q_ptr = this;
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kGroupGap);
}

ShadKbdGroup::ShadKbdGroup(const QStringList& keys, QWidget* parent)
    : ShadKbdGroup(parent)
{
    for (const QString& key : keys)
    {
        addKey(key);
    }
}

ShadKbdGroup::~ShadKbdGroup()
{
}

ShadKbd* ShadKbdGroup::addKey(const QString& text)
{
    ShadKbd * key = new ShadKbd(text, this);
    layout()->addWidget(key);
    return key;
}

ShadLabel* ShadKbdGroup::addText(const QString& text)
{
    ShadLabel* label = new ShadLabel(text, this);
    label->setColorRole(ShadThemeType::MutedForeground);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    ShadStyleHelper::applyFont(label, 12, QFont::Normal);
    layout()->addWidget(label);
    return label;
}

void ShadKbdGroup::clear()
{
    while (QLayoutItem* item = layout()->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
}

void ShadKbdGroup::setKeySequence(const QKeySequence& sequence)
{
    Q_D(ShadKbdGroup);
    d->_keySequence = sequence;
    d->rebuildKeys();
    d->updateShortcut();
}

QKeySequence ShadKbdGroup::getKeySequence() const
{
    Q_D(const ShadKbdGroup);
    return d->_keySequence;
}

void ShadKbdGroup::setShortcutEnabled(bool enabled)
{
    Q_D(ShadKbdGroup);
    if (d->_isShortcutEnabled == enabled)
    {
        return;
    }
    d->_isShortcutEnabled = enabled;
    d->updateShortcut();
}

bool ShadKbdGroup::isShortcutEnabled() const
{
    Q_D(const ShadKbdGroup);
    return d->_isShortcutEnabled;
}

void ShadKbdGroup::bindAction(QAction* action)
{
    Q_D(ShadKbdGroup);
    QObject::disconnect(d->_actionConnection);
    d->_boundAction = action;
    if (action != nullptr)
    {
        // QAction::changed 在快捷键、文字等属性变化时发出，只有快捷键变化时才重新生成按键
        d->_actionConnection = connect(action, &QAction::changed, this, [this, d]()
        {
            if (!d->_boundAction.isNull() && d->_boundAction->shortcut() != d->_keySequence)
            {
                setKeySequence(d->_boundAction->shortcut());
            }
        });
        setKeySequence(action->shortcut());
        return;
    }
    d->updateShortcut();
}

QAction* ShadKbdGroup::getBoundAction() const
{
    Q_D(const ShadKbdGroup);
    return d->_boundAction.data();
}

QString ShadKbdGroup::shortcutText(const QKeySequence& sequence)
{
    QStringList segments;
    for (int index = 0; index < sequence.count(); ++index)
    {
#ifdef Q_OS_MACOS
        // macOS 菜单的写法：符号直接相连，例如 ⇧⌘P
        segments.append(combinationTexts(sequence[index]).join(QString()));
#else
        segments.append(combinationTexts(sequence[index]).join(QStringLiteral("+")));
#endif
    }
    return segments.join(QStringLiteral(", "));
}

QStringList ShadKbdGroup::keyTexts(QKeyCombination combination)
{
    return combinationTexts(combination);
}

QStringList ShadKbdGroup::keyTexts(const QKeySequence& sequence)
{
    QStringList texts;
    for (int index = 0; index < sequence.count(); ++index)
    {
        texts.append(combinationTexts(sequence[index]));
    }
    return texts;
}
