#ifndef SHADRESIZABLEHANDLE_H
#define SHADRESIZABLEHANDLE_H

#include <QSplitterHandle>

class QPaintEvent;
class QSplitter;

// ShadResizable 的拖动手柄：中间 1px bg-border 分隔线，可选 withHandle 抓手
class ShadResizableHandle : public QSplitterHandle
{
    Q_OBJECT

    public:
        ShadResizableHandle(Qt::Orientation orientation, QSplitter* parent);
        ~ShadResizableHandle() override;

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADRESIZABLEHANDLE_H
