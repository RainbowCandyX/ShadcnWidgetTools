#ifndef SHADSTATUSBARPRIVATE_H
#define SHADSTATUSBARPRIVATE_H

#include <QList>
#include <QString>

#include "ShadProperty.h"

class QHBoxLayout;
class QTimer;
class QWidget;
class ShadLabel;
class ShadSeparator;
class ShadStatusBar;

class ShadStatusBarPrivate
{
    SHAD_D_CREATE(ShadStatusBar)

    public:
        ShadStatusBarPrivate();
        ~ShadStatusBarPrivate();

        static constexpr int kMinHeight = 28;
        static constexpr int kPaddingX = 12;
        static constexpr int kSpacing = 12;
        static constexpr int kSeparatorHeight = 12;
        static constexpr int kFontSize = 12;

        struct Entry
        {
            QWidget* widget{nullptr};
            ShadSeparator* separator{nullptr};
        };

        bool contains(const QWidget* widget) const;
        void insertEntry(QWidget* widget, int stretch, bool isPermanent);
        void removeEntry(QWidget* widget, bool isDestroyed);
        void updateSeparators();
        void updateMessageText();

        QWidget* _startArea{nullptr};
        QHBoxLayout* _startLayout{nullptr};
        QWidget* _endArea{nullptr};
        QHBoxLayout* _endLayout{nullptr};
        ShadLabel* _messageLabel{nullptr};
        QTimer* _messageTimer{nullptr};
        QString _message;
        QList<Entry> _startEntries;
        QList<Entry> _endEntries;
        bool _isSeparatorVisible{true};
};

#endif // SHADSTATUSBARPRIVATE_H
