#ifndef SKELETONPAGE_H
#define SKELETONPAGE_H

#include "ExampleBasePage.h"

// 骨架屏示例
class SkeletonPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SkeletonPage(QWidget* parent = nullptr);
        ~SkeletonPage() override;
};

#endif // SKELETONPAGE_H
