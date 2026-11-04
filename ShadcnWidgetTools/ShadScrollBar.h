#ifndef SHADSCROLLBAR_H
#define SHADSCROLLBAR_H

#include <QScrollBar>
#include <QSize>

#include "ShadProperty.h"

class QContextMenuEvent;
class QEvent;
class QMouseEvent;
class QPaintEvent;

// shadcn ScrollArea 风格的细滚动条：无箭头按钮，10px 宽，圆角滑块
// 用于 QAbstractScrollArea 时通过 setVerticalScrollBar / setHorizontalScrollBar 替换原生滚动条
class ShadScrollBarPrivate;

class SHADCN_EXPORT ShadScrollBar : public QScrollBar
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadScrollBar)
    Q_PROPERTY(int trackInset READ getTrackInset WRITE setTrackInset)

    public:
        explicit ShadScrollBar(QWidget* parent = nullptr);
        explicit ShadScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadScrollBar() override;

        // 轨道两端额外留白，避免滑块进入圆角容器的圆角区域，默认 0
        void setTrackInset(int inset);
        int getTrackInset() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSCROLLBAR_H
