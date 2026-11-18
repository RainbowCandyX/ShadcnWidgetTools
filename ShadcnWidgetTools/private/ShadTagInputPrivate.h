#ifndef SHADTAGINPUTPRIVATE_H
#define SHADTAGINPUTPRIVATE_H

#include <QChar>
#include <QFont>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>

#include "ShadProperty.h"

class QKeyEvent;
class QPainter;
class ShadInput;
class ShadTagInput;

class ShadTagInputPrivate
{
    SHAD_D_CREATE(ShadTagInput)

    public:
        ShadTagInputPrivate();
        ~ShadTagInputPrivate();

        static constexpr int kDefaultWidth = 240;
        static constexpr int kPaddingX = 8;
        static constexpr int kPaddingY = 6;
        static constexpr int kTagHeight = 24;
        static constexpr int kTagGap = 4;
        static constexpr int kTagPaddingX = 8;
        static constexpr int kTagFontSize = 12;
        static constexpr int kCloseSize = 16;
        static constexpr int kCloseIconSize = 12;
        static constexpr int kCloseGap = 2;
        static constexpr int kCloseEndPadding = 4;
        static constexpr int kMinInputWidth = 80;
        static constexpr int kInputPaddingX = 5;

        struct Layout
        {
            QList<QRect> tagRects;
            QRect inputRect;
            int height{0};
        };

        QFont tagFont() const;
        bool isCloseVisible() const;
        Layout computeLayout(int width) const;
        void relayout();
        QRect visualTagRect(int index) const;
        QRect closeRect(int index) const;
        int tagIndexAt(const QPoint& pos) const;
        void paintTags(QPainter* painter) const;

        bool isSeparator(QChar ch) const;
        bool canAccept(const QString& tag) const;
        QStringList normalized(const QStringList& tags) const;
        void applyConstraints();
        void removeAt(int index);
        void commitText();
        void handleTextEdited(const QString& text);
        bool handleKeyPress(QKeyEvent* event);
        void setActiveIndex(int index);
        void updateHover(const QPoint& pos);
        void updatePlaceholder();

        ShadInput* _input{nullptr};
        QStringList _tags;
        QList<QRect> _tagRects;
        QString _placeholderText;
        QString _separators{QStringLiteral(",")};
        int _layoutHeight{0};
        int _maxTags{0};
        int _activeIndex{-1};
        int _hoveredCloseIndex{-1};
        int _pressedCloseIndex{-1};
        bool _isDuplicatesAllowed{false};
        bool _isReadOnly{false};
        bool _isInvalid{false};
};

#endif // SHADTAGINPUTPRIVATE_H
