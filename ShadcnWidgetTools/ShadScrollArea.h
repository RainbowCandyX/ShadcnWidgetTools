#ifndef SHADSCROLLAREA_H
#define SHADSCROLLAREA_H

#include <QScrollArea>

#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QObject;
class QResizeEvent;
class ShadScrollBar;

// shadcn ScrollArea：无边框，内容区透明并自动撑满宽度（widgetResizable），细滚动条覆盖在内容上方，不占用内容宽度
// 滚动条在鼠标悬停时显示，离开 600ms 后隐藏（对应 Radix ScrollArea 默认的 type="hover"）；滚轮与键盘滚动不受影响
// 原生滚动条固定为 ScrollBarAlwaysOff 并在内部维护滚动范围，不要再修改两个方向的 ScrollBarPolicy
class ShadScrollAreaPrivate;

class SHADCN_EXPORT ShadScrollArea : public QScrollArea
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadScrollArea)

    public:
        explicit ShadScrollArea(QWidget* parent = nullptr);
        ~ShadScrollArea() override;

        // 覆盖在内容上方的滚动条，可用于设置 trackInset 等外观
        ShadScrollBar* getOverlayScrollBar(Qt::Orientation orientation) const;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADSCROLLAREA_H
