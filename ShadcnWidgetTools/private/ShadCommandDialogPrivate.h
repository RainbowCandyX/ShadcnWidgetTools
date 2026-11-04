#ifndef SHADCOMMANDDIALOGPRIVATE_H
#define SHADCOMMANDDIALOGPRIVATE_H

#include <QKeySequence>

#include "ShadProperty.h"

class QKeyEvent;
class ShadCommand;
class ShadCommandDialog;

class ShadCommandDialogPrivate
{
    SHAD_D_CREATE(ShadCommandDialog)

    public:
        ShadCommandDialogPrivate();
        ~ShadCommandDialogPrivate();

        // sm:max-w-lg
        static constexpr int kWidth = 512;

        bool matchesShortcut(const QKeyEvent* event) const;

        ShadCommand* _command{nullptr};
        QKeySequence _shortcut;
        bool _isResizable{false};
};

#endif // SHADCOMMANDDIALOGPRIVATE_H
