#ifndef SHADDOCKFLOATINGWINDOW_H
#define SHADDOCKFLOATINGWINDOW_H

#include <QByteArray>
#include <QPoint>
#include <QWidget>
#include <QtGlobal>

#include "ShadWidget.h"

class QEvent;
class QObject;
class ShadDockContainer;
class ShadTitleBar;

class ShadDockFloatingWindow : public ShadWidget
{
    Q_OBJECT

    public:
        explicit ShadDockFloatingWindow(QWidget* parent = nullptr);
        ~ShadDockFloatingWindow() override;

        ShadDockContainer* container() const;

    Q_SIGNALS:
        void activated();
        // 仅 Windows
        void systemMoving();
        void systemMoveFinished();
        // 仅 macOS 与 Linux
        void titleBarDragRequested(const QPoint& pressGlobalPos);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    private:
        ShadDockContainer* _container{nullptr};
        ShadTitleBar* _titleBar{nullptr};
        bool _isTitlePressed{false};
        QPoint _titlePressGlobalPos;
};

#endif // SHADDOCKFLOATINGWINDOW_H
