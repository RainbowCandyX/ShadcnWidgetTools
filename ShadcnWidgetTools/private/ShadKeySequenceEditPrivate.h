#ifndef SHADKEYSEQUENCEEDITPRIVATE_H
#define SHADKEYSEQUENCEEDITPRIVATE_H

#include <QKeySequence>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class ShadKeySequenceEdit;

class ShadKeySequenceEditPrivate
{
    SHAD_D_CREATE(ShadKeySequenceEdit)

    public:
        ShadKeySequenceEditPrivate();
        ~ShadKeySequenceEditPrivate();

        // 只关心这四个修饰键，忽略小键盘等标志
        static Qt::KeyboardModifiers relevantModifiers(Qt::KeyboardModifiers modifiers);
        static bool isModifierKey(int key);

        // 结束侧清除按钮的区域（组件坐标），没有值或禁用时不显示
        QRectF clearButtonRect() const;
        bool isClearVisible() const;
        // 当前应显示的按键：按住修饰键时为修饰键，否则为已录制的组合键
        QStringList displayKeys() const;
        // 设置新值，变化时发出 keySequenceChanged
        void applySequence(const QKeySequence& sequence);
        void updateClearHover(const QPoint& pos);

        QKeySequence _sequence;
        // 获得焦点时的值，Esc 时恢复
        QKeySequence _sequenceBeforeEdit;
        QString _placeholderText;
        Qt::KeyboardModifiers _pendingModifiers;
        bool _isInvalid{false};
        bool _isClearHovered{false};
};

#endif // SHADKEYSEQUENCEEDITPRIVATE_H
