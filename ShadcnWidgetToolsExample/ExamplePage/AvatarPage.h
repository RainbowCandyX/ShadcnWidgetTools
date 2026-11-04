#ifndef AVATARPAGE_H
#define AVATARPAGE_H

#include "ExampleBasePage.h"

// 头像示例：基础用法、尺寸、徽章、头像组、计数、下拉菜单与 RTL
class AvatarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AvatarPage(QWidget* parent = nullptr);
        ~AvatarPage() override;
};

#endif // AVATARPAGE_H
