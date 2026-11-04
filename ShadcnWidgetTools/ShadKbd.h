#ifndef SHADKBD_H
#define SHADKBD_H

#include <QKeyCombination>
#include <QKeySequence>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadProperty.h"

class QAction;
class QEvent;
class QPaintEvent;
class ShadLabel;

// 键盘按键，对应 shadcn Kbd：h-5 min-w-5 px-1 rounded-sm bg-muted text-xs font-medium text-muted-foreground
// 不响应鼠标（pointer-events-none），不预留焦点环边距
class ShadKbdPrivate;

class SHADCN_EXPORT ShadKbd : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadKbd)
    Q_PROPERTY(QString text READ getText WRITE setText)

    public:
        explicit ShadKbd(QWidget* parent = nullptr);
        explicit ShadKbd(const QString& text, QWidget* parent = nullptr);
        ~ShadKbd() override;

        // 按键文字，例如 "Ctrl"、"⌘"、"⏎"
        void setText(const QString& text);
        QString getText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

// 按键组合，对应 KbdGroup：按键之间 gap-1，可插入普通文字（例如 "+"）
// 除了手动添加按键，也可以用 setKeySequence 按 QKeySequence 生成，并可选绑定为快捷键，或用 bindAction 跟随 QAction 的快捷键
class ShadKbdGroupPrivate;

class SHADCN_EXPORT ShadKbdGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadKbdGroup)
    Q_PROPERTY(QKeySequence keySequence READ getKeySequence WRITE setKeySequence)
    Q_PROPERTY(bool shortcutEnabled READ isShortcutEnabled WRITE setShortcutEnabled)

    public:
        explicit ShadKbdGroup(QWidget* parent = nullptr);
        // 依次添加 keys 中的每个按键
        explicit ShadKbdGroup(const QStringList& keys, QWidget* parent = nullptr);
        ~ShadKbdGroup() override;

        ShadKbd* addKey(const QString& text);
        // 按键之间的普通文字，text-xs text-muted-foreground
        ShadLabel* addText(const QString& text);
        // 删除全部按键与文字
        void clear();

        // 按 QKeySequence 重新生成按键（会清除手动添加的按键）：macOS 显示 ⌘ ⇧ ⌥ ⌃ 符号，其他平台显示 Ctrl / Shift / Alt
        // Qt 在 macOS 上把 Qt::CTRL 对应到 Command 键，因此 "Ctrl+K" 在 macOS 上显示为 ⌘K；多段组合键之间显示逗号
        void setKeySequence(const QKeySequence& sequence);
        QKeySequence getKeySequence() const;

        // 绑定为快捷键：在所在窗口内按下组合键时发出 activated，默认关闭；按键组隐藏或禁用时快捷键同时失效
        // 使用 bindAction 时不会再注册快捷键，由 QAction 负责触发，避免同一组合键注册两次
        void setShortcutEnabled(bool enabled);
        bool isShortcutEnabled() const;

        // 显示 action 的快捷键，并在 action 的快捷键变化时更新；传 nullptr 解除跟随（保留当前显示）
        void bindAction(QAction* action);
        QAction* getBoundAction() const;

        // QKeySequence 拆成的按键文字，规则与 setKeySequence 相同（多段组合键依次排列，不含逗号）
        static QStringList keyTexts(const QKeySequence& sequence);
        // 一段组合键拆成的按键文字；主键为 0 时只返回修饰键（用于显示正在按住的修饰键）
        static QStringList keyTexts(QKeyCombination combination);
        // 快捷键的纯文字写法，用于菜单的快捷键提示等：macOS 为 "⇧⌘P"，其他平台为 "Ctrl+Shift+P"，多段之间为 ", "
        static QString shortcutText(const QKeySequence& sequence);

    Q_SIGNALS:
        // setShortcutEnabled(true) 后按下组合键时发出
        void activated();
};

#endif // SHADKBD_H
