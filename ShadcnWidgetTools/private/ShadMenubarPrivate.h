#ifndef SHADMENUBARPRIVATE_H
#define SHADMENUBARPRIVATE_H

#include <QList>
#include <QPoint>
#include <QPointer>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QHBoxLayout;
class QMenu;
class QMenuBar;
class QObject;
class ShadDropdownMenu;
class ShadMenubar;
class ShadMenubarTrigger;

class ShadMenubarPrivate
{
    SHAD_D_CREATE(ShadMenubar)

    public:
        ShadMenubarPrivate();
        ~ShadMenubarPrivate();

        struct Menu
        {
            ShadMenubarTrigger* trigger{nullptr};
            ShadDropdownMenu* menu{nullptr};
        };

        // h-9 gap-1 p-1 border
        static constexpr int kBarHeight = 36;
        static constexpr int kBorderWidth = 1;
        static constexpr int kPadding = 4;
        static constexpr int kGap = 4;
        static constexpr int kTriggerHeight = 28;
        // sideOffset=8 alignOffset=-4
        static constexpr int kSideOffset = 8;
        static constexpr int kAlignOffset = -4;

        int indexOfMenu(const QObject* menu) const;
        int indexOfTrigger(const QObject* trigger) const;
        int triggerAt(const QPoint& globalPos) const;
        // highlight：1 高亮第一项，-1 高亮最后一项，0 不高亮
        void openMenu(int index, int highlight);
        void onMenuHidden(ShadDropdownMenu* menu);
        int wrappedIndex(int index, int step) const;
        void focusTrigger(int index, Qt::FocusReason reason);
        // 只有最近获得焦点的触发项参与 Tab 切换
        void updateFocusPolicies();

        bool usesNativeMenuBar() const;
        void requestNativeSync();
        void syncNativeMenuBar();
        void fillNativeMenu(QMenu* nativeMenu, ShadDropdownMenu* menu);

        QHBoxLayout* _layout{nullptr};
        QList<Menu> _menus;
        int _openIndex{-1};
        int _focusIndex{0};
        bool _isSwitching{false};
        bool _isKeyboardClose{false};

        bool _isNativeMenuBar{false};
        bool _isNativeSyncPending{false};
        bool _wasHiddenBeforeNative{false};
        QPointer<QMenuBar> _nativeMenuBar;
        QPointer<QWidget> _nativeWindow;
};

#endif // SHADMENUBARPRIVATE_H
