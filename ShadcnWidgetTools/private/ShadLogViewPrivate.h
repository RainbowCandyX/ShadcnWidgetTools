#ifndef SHADLOGVIEWPRIVATE_H
#define SHADLOGVIEWPRIVATE_H

#include <QColor>
#include <QDateTime>
#include <QList>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class QAction;
class QTextCursor;
class ShadDropdownMenu;
class ShadLogView;

class ShadLogViewPrivate
{
    SHAD_D_CREATE(ShadLogView)

    public:
        ShadLogViewPrivate();
        ~ShadLogViewPrivate();

        static constexpr int kFrameInset = 1 + ShadStyleHelper::kFocusRingWidth;
        static constexpr int kPaddingX = 12;
        static constexpr int kPaddingY = 8;
        static constexpr int kFontPixelSize = 13;
        static constexpr int kDefaultWidth = 480;
        static constexpr int kDefaultHeight = 240;
        static constexpr int kMinHeight = 80;
        static constexpr int kDefaultMaxEntryCount = 5000;

        struct Entry
        {
            QDateTime time;
            QString message;
            ShadLogViewType::Level level{ShadLogViewType::Info};
            int blockCount{1};
            bool isVisible{true};
        };

        void applyFont();
        void applyThemeColors();
        void paintFrame();

        bool matches(const Entry& entry) const;
        QColor levelColor(ShadLogViewType::Level level) const;
        QColor messageColor(ShadLogViewType::Level level) const;
        void appendEntry(ShadLogViewType::Level level, const QString& message, const QDateTime& time);
        void insertEntry(QTextCursor& cursor, const Entry& entry, bool isFirst) const;
        int removeFirst(int count);
        void rebuild();
        void syncFollowing();
        void restoreScroll(bool wasFollowing, int value);
        void restoreFollowing(bool wasFollowing);

        QList<Entry> _entries;
        QString _timestampFormat{QStringLiteral("HH:mm:ss.zzz")};
        QString _filterText;
        ShadDropdownMenu* _contextMenu{nullptr};
        QAction* _copyAction{nullptr};
        QAction* _clearAction{nullptr};
        ShadLogViewType::Level _minimumLevel{ShadLogViewType::Debug};
        int _maxEntryCount{kDefaultMaxEntryCount};
        int _visibleCount{0};
        bool _isAutoScroll{true};
        bool _isTimestampVisible{true};
        bool _isLevelVisible{true};
        bool _isFollowing{true};
        bool _isUpdating{false};
        bool _isFocusVisible{false};
};

#endif // SHADLOGVIEWPRIVATE_H
