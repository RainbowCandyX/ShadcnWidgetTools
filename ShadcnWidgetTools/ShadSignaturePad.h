#ifndef SHADSIGNATUREPAD_H
#define SHADSIGNATUREPAD_H

#include <QColor>
#include <QImage>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QTabletEvent;

class ShadSignaturePadPrivate;

class SHADCN_EXPORT ShadSignaturePad : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSignaturePad)
    Q_PROPERTY(QColor penColor READ getPenColor WRITE setPenColor)
    Q_PROPERTY(qreal minPenWidth READ getMinPenWidth WRITE setMinPenWidth)
    Q_PROPERTY(qreal maxPenWidth READ getMaxPenWidth WRITE setMaxPenWidth)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)

    public:
        explicit ShadSignaturePad(QWidget* parent = nullptr);
        ~ShadSignaturePad() override;

        void setPenColor(const QColor& color);
        QColor getPenColor() const;

        void setMinPenWidth(qreal width);
        qreal getMinPenWidth() const;
        void setMaxPenWidth(qreal width);
        qreal getMaxPenWidth() const;

        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        bool isEmpty() const;
        int strokeCount() const;

        QImage toImage(bool trimmed = true, const QColor& background = Qt::transparent) const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        void undo();
        void clear();

    Q_SIGNALS:
        void strokeStarted();
        void strokeFinished();
        void changed();

    protected:
        void changeEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void tabletEvent(QTabletEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif
