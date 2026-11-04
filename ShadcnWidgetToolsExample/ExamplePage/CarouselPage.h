#ifndef CAROUSELPAGE_H
#define CAROUSELPAGE_H

#include "ExampleBasePage.h"

// 轮播示例：基础用法、尺寸、间距、方向、循环、API 与事件、自动播放与 RTL
class CarouselPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CarouselPage(QWidget* parent = nullptr);
        ~CarouselPage() override;
};

#endif // CAROUSELPAGE_H
