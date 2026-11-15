#ifndef MARQUEEPAGE_H
#define MARQUEEPAGE_H

#include "ExampleBasePage.h"

// 跑马灯示例
class MarqueePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit MarqueePage(QWidget* parent = nullptr);
        ~MarqueePage() override;
};

#endif // MARQUEEPAGE_H
