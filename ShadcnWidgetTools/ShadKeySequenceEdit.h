#ifndef SHADKEYSEQUENCEEDIT_H
#define SHADKEYSEQUENCEEDIT_H

#include <QKeySequence>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 快捷键录制框：输入框外观（与 ShadInput 一致），获得焦点后录制一段组合键并以 Kbd 样式显示（macOS 为 ⌘⇧K，其他平台为 Ctrl Shift K）
// 只记录与显示，不注册快捷键；录制期间按下的组合键不会触发程序中已有的快捷键
// 按住修饰键时实时显示，再按普通键完成录制；Esc 取消并恢复聚焦前的值，单独按 Backspace / Delete 清空，Tab 照常切换焦点
// 有值时结束侧显示清除按钮；四周预留焦点环边距，RTL 下镜像
class ShadKeySequenceEditPrivate;

class SHADCN_EXPORT ShadKeySequenceEdit : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadKeySequenceEdit)
    Q_PROPERTY(QKeySequence keySequence READ getKeySequence WRITE setKeySequence NOTIFY keySequenceChanged)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)

    public:
        explicit ShadKeySequenceEdit(QWidget* parent = nullptr);
        explicit ShadKeySequenceEdit(const QKeySequence& sequence, QWidget* parent = nullptr);
        ~ShadKeySequenceEdit() override;

        // 只保留第一段组合键
        void setKeySequence(const QKeySequence& sequence);
        QKeySequence getKeySequence() const;
        void clear();

        // 没有值时显示的文字，默认 "点击后按下快捷键"；录制中且没有值时显示 "请按下组合键…"
        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        // 错误态：border-destructive 与 destructive 焦点环
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 获得焦点即处于录制状态
        bool isRecording() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 录制完成、清空或调用 setKeySequence 改变值时发出
        void keySequenceChanged(const QKeySequence& sequence);
        // 录制完成、取消或失去焦点时发出
        void editingFinished();

    protected:
        bool event(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADKEYSEQUENCEEDIT_H
