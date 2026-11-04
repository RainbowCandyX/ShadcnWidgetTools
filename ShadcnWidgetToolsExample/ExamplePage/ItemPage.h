#ifndef ITEMPAGE_H
#define ITEMPAGE_H

#include "ExampleBasePage.h"

// 列表项示例：三种样式、三种尺寸、图片与头像媒体、可点击、分组与分隔线、页眉与页脚、RTL
class ItemPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ItemPage(QWidget* parent = nullptr);
        ~ItemPage() override;
};

#endif // ITEMPAGE_H
