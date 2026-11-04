#ifndef RATINGPAGE_H
#define RATINGPAGE_H

#include "ExampleBasePage.h"

// 评分示例：基础用法、只读、数量与尺寸、自定义颜色、键盘操作与 RTL
class RatingPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit RatingPage(QWidget* parent = nullptr);
        ~RatingPage() override;
};

#endif // RATINGPAGE_H
