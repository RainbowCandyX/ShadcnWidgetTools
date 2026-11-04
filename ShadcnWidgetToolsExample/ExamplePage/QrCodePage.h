#ifndef QRCODEPAGE_H
#define QRCODEPAGE_H

#include "ExampleBasePage.h"

// 二维码示例：输入文本实时生成、纠错等级、尺寸与颜色、内容过长
class QrCodePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit QrCodePage(QWidget* parent = nullptr);
        ~QrCodePage() override;
};

#endif // QRCODEPAGE_H
