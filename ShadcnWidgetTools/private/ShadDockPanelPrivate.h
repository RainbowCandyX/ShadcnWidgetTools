#ifndef SHADDOCKPANELPRIVATE_H
#define SHADDOCKPANELPRIVATE_H

#include <QPointer>
#include <QRect>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QVBoxLayout;
class QWidget;
class ShadDockGroup;
class ShadDockManager;
class ShadDockPanel;

class ShadDockPanelPrivate
{
    SHAD_D_CREATE(ShadDockPanel)

    public:
        ShadDockPanelPrivate();
        ~ShadDockPanelPrivate();

        QString _id;
        QVBoxLayout* _layout{nullptr};
        QPointer<QWidget> _widget;
        QString _title;
        ShadIconType::IconName _icon{ShadIconType::None};
        QString _titleToolTip;
        ShadDockType::PanelFlags _flags{ShadDockType::NoPanelFlag};
        bool _isDefaultCloseEnabled{true};
        QAction* _toggleViewAction{nullptr};

        QPointer<ShadDockManager> _manager;
        QPointer<ShadDockGroup> _group;
        bool _isDocument{false};
        bool _isOpen{false};

        // 关闭前的位置
        QPointer<ShadDockGroup> _lastGroup;
        int _lastIndex{-1};
        bool _wasFloating{false};
        QRect _lastFloatingGeometry;
        ShadDockType::DockArea _defaultArea{ShadDockType::RightArea};
};

#endif // SHADDOCKPANELPRIVATE_H
