#ifndef SHADCAROUSEL_H
#define SHADCAROUSEL_H

#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QKeyEvent;
class QResizeEvent;

// 轮播，对应 shadcn Carousel（基于 Embla Carousel）：幻灯片沿横向或纵向排列，可拖动、点击两侧按钮或用方向键切换
// 幻灯片的尺寸由组件决定：主轴方向按一屏张数与间距平分视口（对应 basis-* 与 -ml-4 / pl-4），交叉方向占满视口
// 横向时高度取幻灯片中最高的一张（幻灯片设置了 heightForWidth 时按幻灯片宽度计算）；纵向时高度默认为一屏张数乘以最高的一张加间距，可用 setFixedHeight 覆盖
// 上一张 / 下一张按钮为 outline、32px 的圆形按钮，放在内容区外侧 48px 处（-left-12 / -right-12，纵向为 -top-12 / -bottom-12）；
// Qt 的子组件不能绘制到父组件之外，组件会在两侧为按钮预留空间，隐藏按钮后不再预留
// 拖动：在幻灯片任意位置按下并移动超过 10px 后开始拖动，此时取消幻灯片内按钮等子组件的点击；松开后按拖动距离与速度停靠，最多移动一个停靠点
// 键盘：焦点在轮播内部时，横向按左右方向键（RTL 下对调）、纵向按上下方向键切换；点击幻灯片区域后轮播自身获得焦点（Qt::ClickFocus，不参与 Tab 顺序）；
// 焦点在按钮上时由轮播先处理（按钮默认用方向键移动焦点），其他子组件（例如输入框）没有处理的方向键才交给轮播
// RTL：横向时第一张在右侧，上一张按钮在右侧，拖动方向随之镜像
class ShadCarouselPrivate;

class SHADCN_EXPORT ShadCarousel : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCarousel)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(int itemsPerView READ getItemsPerView WRITE setItemsPerView)
    Q_PROPERTY(int spacing READ getSpacing WRITE setSpacing)
    Q_PROPERTY(ShadCarouselType::Align align READ getAlign WRITE setAlign)
    Q_PROPERTY(bool loop READ isLoop WRITE setLoop)
    Q_PROPERTY(int autoplayInterval READ getAutoplayInterval WRITE setAutoplayInterval)
    Q_PROPERTY(bool navigationVisible READ isNavigationVisible WRITE setNavigationVisible)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)

    public:
        explicit ShadCarousel(QWidget* parent = nullptr);
        ~ShadCarousel() override;

        // 添加幻灯片，轮播接管幻灯片的父子关系；返回幻灯片的位置
        int addItem(QWidget* item);
        // index 超出范围时添加到末尾
        int insertItem(int index, QWidget* item);
        // 移除幻灯片但不删除，幻灯片的父组件置空，由调用方负责释放
        void removeItem(int index);
        int count() const;
        QWidget* item(int index) const;
        // 幻灯片不属于轮播时返回 -1
        int indexOf(QWidget* item) const;

        // 默认 Qt::Horizontal，对应 orientation="horizontal" / "vertical"
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        // 一屏显示几张幻灯片，默认 1；对应 basis-full / basis-1/2 / basis-1/3
        void setItemsPerView(int itemsPerView);
        int getItemsPerView() const;

        // 相邻幻灯片的间距，默认 16，对应 CarouselContent 的 -ml-4 与 CarouselItem 的 pl-4
        void setSpacing(int spacing);
        int getSpacing() const;

        // 停靠时的对齐方式，默认 Center（与 Embla 一致）
        void setAlign(ShadCarouselType::Align align);
        ShadCarouselType::Align getAlign() const;

        // 首尾无缝循环，默认关闭；幻灯片数量不多于一屏张数时不足以循环，按不循环处理
        void setLoop(bool loop);
        bool isLoop() const;

        // 自动播放的间隔（毫秒），0 表示关闭（默认）；对应 Autoplay 插件的 delay
        // 鼠标悬停时暂停，离开后继续；手动切换或拖动后重新计时；不循环时播放到最后一个停靠点后回到第一个
        void setAutoplayInterval(int milliseconds);
        int getAutoplayInterval() const;

        // 是否显示上一张 / 下一张按钮，默认显示
        void setNavigationVisible(bool visible);
        bool isNavigationVisible() const;

        // 以下的位置均为停靠点序号：一屏多张且不循环时，末尾放不满一屏的停靠点会合并（Embla 的 containScroll="trimSnaps"），
        // 例如 5 张幻灯片、一屏 3 张、起始对齐时共有 3 个停靠点
        void scrollPrev();
        void scrollNext();
        // animated 为 false 时直接跳到停靠点，对应 scrollTo(index, jump)
        void scrollTo(int index, bool animated = true);
        bool canScrollPrev() const;
        bool canScrollNext() const;
        // 当前停靠点，没有幻灯片时为 -1
        int currentIndex() const;
        int snapCount() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;

    Q_SIGNALS:
        // 当前停靠点变化时发出，对应 Embla 的 select 事件：开始切换时即发出，不等待动画结束
        void currentChanged(int index);
        // 停靠点数量变化时发出（增删幻灯片、修改一屏张数或尺寸变化）
        void snapCountChanged(int count);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
};

#endif // SHADCAROUSEL_H
