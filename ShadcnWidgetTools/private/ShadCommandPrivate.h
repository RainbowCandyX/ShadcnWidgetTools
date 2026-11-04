#ifndef SHADCOMMANDPRIVATE_H
#define SHADCOMMANDPRIVATE_H

#include <QList>
#include <QString>
#include <QStringList>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QKeyEvent;
class QLineEdit;
class ShadCommand;
class ShadCommandList;
class ShadScrollArea;

class ShadCommandPrivate
{
    SHAD_D_CREATE(ShadCommand)

    public:
        ShadCommandPrivate();
        ~ShadCommandPrivate();

        enum class EntryKind
        {
            Group,
            Item,
            Separator,
        };

        struct Entry
        {
            EntryKind kind{EntryKind::Item};
            QString heading;
            QAction* action{nullptr};
            ShadIconType::IconName icon{ShadIconType::None};
            QString shortcutText;
            QStringList keywords;
        };

        // rounded-lg border
        static constexpr int kBorderWidth = 1;
        // 搜索栏 h-9 px-3 gap-2，对话框中 h-12
        static constexpr int kSearchHeight = 36;
        static constexpr int kLargeSearchHeight = 48;
        static constexpr int kSearchPaddingX = 12;
        static constexpr int kSearchIconSize = 16;
        static constexpr int kLargeSearchIconSize = 20;
        static constexpr int kSearchIconGap = 8;
        // max-h-[300px]
        static constexpr int kListMaxHeight = 300;

        int chromeMargin() const;
        int searchHeight() const;
        int searchIconSize() const;
        int listHeight() const;
        void rebuildRows();
        void layoutChildren();
        void scrollToHighlight();
        void activateHighlighted();
        bool handleNavigationKey(QKeyEvent* event);
        void applyThemeColors();
        // 对话框样式
        void setDialogStyle(bool isDialogStyle);

        QList<Entry> _entries;
        QLineEdit* _searchEdit{nullptr};
        ShadScrollArea* _scrollArea{nullptr};
        ShadCommandList* _list{nullptr};
        QString _emptyText;
        bool _isDialogStyle{false};
};

#endif // SHADCOMMANDPRIVATE_H
