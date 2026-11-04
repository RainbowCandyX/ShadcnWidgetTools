#ifndef SHADDOCKHEADER_H
#define SHADDOCKHEADER_H

#include <QPoint>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"

class QEvent;
class QMouseEvent;
class QPaintEvent;
class ShadTransition;

class ShadDockHeader : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDockHeader(QWidget* parent = nullptr);
        ~ShadDockHeader() override;

        void setTitle(ShadIconType::IconName icon, const QString& title);
        void setCloseVisible(bool visible);
        void setDragEnabled(bool enabled);
        void setActive(bool active);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void closeClicked();
        void dragRequested(const QPoint& grabPos);

    protected:
        bool event(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QRectF closeRect() const;
        void setCloseHover(bool hover);

        ShadIconType::IconName _icon{ShadIconType::None};
        QString _title;
        bool _isCloseVisible{true};
        bool _isDragEnabled{true};
        bool _isActive{false};
        bool _isCloseHover{false};
        bool _isClosePressed{false};
        bool _isTitlePressed{false};
        QPoint _pressPos;
        ShadTransition* _activeTransition{nullptr};
};

#endif // SHADDOCKHEADER_H
