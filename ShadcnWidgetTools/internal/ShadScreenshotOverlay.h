#ifndef SHADSCREENSHOTOVERLAY_H
#define SHADSCREENSHOTOVERLAY_H

#include <QImage>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>

#include "ShadScreenshotAnnotation.h"

class QEnterEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPlainTextEdit;
class QScreen;
class ShadButton;
class ShadScreenshotToolbar;

class ShadScreenshotOverlay : public QWidget
{
    Q_OBJECT

    public:
        ShadScreenshotOverlay(QScreen* screen, const QPixmap& screenshot, QWidget* parent);
        ~ShadScreenshotOverlay() override;

        void setWindowRects(const QList<QRect>& rects);
        void setLocked(bool locked);
        void setCornerPinButtonVisible(bool visible);
        void syncWithCursor();

    Q_SIGNALS:
        void selectionChanged(bool hasSelection);
        void doneRequested(const QImage& image);
        void copyRequested(const QImage& image);
        void saveRequested(const QImage& image);
        void pinRequested(const QImage& image, const QPoint& globalPosition);
        void canceled();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        enum class DragMode
        {
            None = 0,
            Pending,
            Creating,
            Moving,
            Resizing,
        };

        enum Edge
        {
            NoEdge = 0x0,
            LeftEdge = 0x1,
            TopEdge = 0x2,
            RightEdge = 0x4,
            BottomEdge = 0x8,
        };

        bool isAnnotating() const;
        bool isEditingText() const;
        int edgesAt(const QPoint& position) const;
        void setSelection(const QRect& selection);
        void updateHighlight(const QPoint& position);
        void updateCursorShape(const QPoint& position);
        void updateToolbar();
        void requestPin();
        void retranslate();
        void handleToolChanged();
        void handleStyleChanged();
        void ensureMosaic(int blockSize);
        void startText(const QPoint& position);
        void updateTextEditor();
        void commitText();
        void clearAnnotations();
        void undo();
        QRect sizeLabelRect(const QRect& selection) const;
        QRect decorationRect(const QRect& selection) const;
        QString sizeText(const QRect& selection) const;
        QImage selectedImage();

        QPixmap _screenshot;
        QImage _image;
        QMap<int, QPixmap> _mosaics;
        qreal _ratio{1.0};
        QList<QRect> _windowRects;
        QRect _highlightRect;
        QRect _selection;
        QRect _dragStartSelection;
        QPoint _pressPosition;
        DragMode _dragMode{DragMode::None};
        int _dragEdges{NoEdge};
        bool _isLocked{false};
        bool _isCornerPinButtonVisible{false};
        ShadScreenshotToolbar* _toolbar{nullptr};
        ShadButton* _pinButton{nullptr};
        QList<ShadScreenshotAnnotation> _annotations;
        ShadScreenshotAnnotation _current;
        bool _isDrawing{false};
        QPlainTextEdit* _textEditor{nullptr};
        ShadScreenshotAnnotation _textAnnotation;
        QRect _textFrame;
};

#endif
