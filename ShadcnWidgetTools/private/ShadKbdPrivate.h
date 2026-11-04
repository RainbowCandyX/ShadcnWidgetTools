#ifndef SHADKBDPRIVATE_H
#define SHADKBDPRIVATE_H

#include <QKeySequence>
#include <QMetaObject>
#include <QPointer>
#include <QString>

#include "ShadProperty.h"

class QAction;
class QShortcut;
class ShadKbd;
class ShadKbdGroup;

class ShadKbdPrivate
{
    SHAD_D_CREATE(ShadKbd)

    public:
        ShadKbdPrivate();
        ~ShadKbdPrivate();

        QString _text;
};

class ShadKbdGroupPrivate
{
    SHAD_D_CREATE(ShadKbdGroup)

    public:
        ShadKbdGroupPrivate();
        ~ShadKbdGroupPrivate();

        // 按 _keySequence 重新生成按键
        void rebuildKeys();
        // 按启用状态与是否跟随 QAction 创建、更新或删除快捷键
        void updateShortcut();

        QKeySequence _keySequence;
        bool _isShortcutEnabled{false};
        QShortcut* _shortcut{nullptr};
        QPointer<QAction> _boundAction;
        QMetaObject::Connection _actionConnection;
};

#endif // SHADKBDPRIVATE_H
