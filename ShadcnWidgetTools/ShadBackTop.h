#ifndef SHADBACKTOP_H
#define SHADBACKTOP_H

#include <QScopedPointer>

#include "ShadButton.h"
#include "ShadProperty.h"

class QAbstractScrollArea;
class QEvent;
class QObject;
class ShadBackTopPrivate;

// 回到顶部按钮，对应 Ant Design FloatButton.BackTop：圆形 Outline 图标按钮，悬浮在滚动区域可视区的结束侧下角，RTL 时位于左下角
// 向下滚动达到 visibilityHeight 时淡入，回到上方后淡出；点击后平滑滚动到顶部，动画期间手动滚动会中断动画
// 按钮成为滚动区域的子组件，随滚动区域一起销毁；外观可通过 ShadButton 的接口修改
class SHADCN_EXPORT ShadBackTop : public ShadButton
{
    Q_OBJECT
    Q_PROPERTY(int visibilityHeight READ getVisibilityHeight WRITE setVisibilityHeight)
    Q_PROPERTY(int offset READ getOffset WRITE setOffset)
    Q_PROPERTY(int duration READ getDuration WRITE setDuration)

    public:
        explicit ShadBackTop(QAbstractScrollArea* target = nullptr);
        ~ShadBackTop() override;

        // 绑定的滚动区域，按钮成为它的子组件；传 nullptr 解除绑定并隐藏按钮
        void setTarget(QAbstractScrollArea* target);
        QAbstractScrollArea* getTarget() const;

        // 显示按钮所需的最小滚动距离，默认 400
        void setVisibilityHeight(int height);
        int getVisibilityHeight() const;

        // 按钮与可视区底边、结束侧边缘的距离，默认 24
        void setOffset(int offset);
        int getOffset() const;

        // 滚动到顶部的动画时长（毫秒），默认 450，0 表示直接跳到顶部
        void setDuration(int durationMs);
        int getDuration() const;

    public Q_SLOTS:
        // 平滑滚动到顶部，点击按钮时调用
        void scrollToTop();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;

    private:
        QScopedPointer<ShadBackTopPrivate> _private;
};

#endif // SHADBACKTOP_H
