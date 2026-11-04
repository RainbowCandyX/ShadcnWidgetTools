#ifndef SHADQRCODE_H
#define SHADQRCODE_H

#include <QColor>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;

// 二维码：文本变化后自动按 UTF-8 编码（含非 ASCII 字符时附带 UTF-8 的 ECI 声明），自动选择数字 / 字母数字 / 字节模式、版本（1 到 40）与掩码，符合 ISO/IEC 18004
// 纠错等级按设置的档位编码，不会自动提升；超出当前纠错等级的最大容量时显示错误状态，并发出 encodingFailed
// 外观：圆角背景加 1px Border 色边框，四周保留静区；默认白底黑格，不随深色主题反转（浅色格子在深色背景上的二维码部分扫码器无法识别）
// 每个格子按整数像素绘制，保证边缘清晰；RTL 下不做镜像
class ShadQrCodePrivate;

class SHADCN_EXPORT ShadQrCode : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadQrCode)
    Q_PROPERTY(QString text READ getText WRITE setText NOTIFY textChanged)
    Q_PROPERTY(ShadQrCodeType::ErrorCorrection errorCorrectionLevel READ getErrorCorrectionLevel WRITE setErrorCorrectionLevel)
    Q_PROPERTY(int qrSize READ getQrSize WRITE setQrSize)
    Q_PROPERTY(int quietZone READ getQuietZone WRITE setQuietZone)
    Q_PROPERTY(QColor foregroundColor READ getForegroundColor WRITE setForegroundColor)
    Q_PROPERTY(QColor backgroundColor READ getBackgroundColor WRITE setBackgroundColor)

    public:
        explicit ShadQrCode(QWidget* parent = nullptr);
        explicit ShadQrCode(const QString& text, QWidget* parent = nullptr);
        ~ShadQrCode() override;

        // 二维码内容，为空时只绘制背景
        void setText(const QString& text);
        QString getText() const;

        // 纠错等级，默认 Medium（约 15%）
        void setErrorCorrectionLevel(ShadQrCodeType::ErrorCorrection level);
        ShadQrCodeType::ErrorCorrection getErrorCorrectionLevel() const;

        // 最近一次编码成功时返回 true；文本为空或内容过长时返回 false
        bool isValid() const;
        // 当前二维码的版本（1 到 40），版本越大格子越多；没有有效二维码时返回 0
        int getVersion() const;
        // 每边的格子数（版本 × 4 + 17），不含静区；没有有效二维码时返回 0
        int getModuleCount() const;

        // 控件边长（像素），默认 160，控件尺寸固定为该值
        void setQrSize(int size);
        int getQrSize() const;

        // 四周留白的格子数，默认 4（标准规定的最小静区）
        void setQuietZone(int modules);
        int getQuietZone() const;

        // 深色格子颜色，默认黑色
        void setForegroundColor(const QColor& color);
        QColor getForegroundColor() const;
        // 背景与浅色格子颜色，默认白色
        void setBackgroundColor(const QColor& color);
        QColor getBackgroundColor() const;

        QSize sizeHint() const override;

    Q_SIGNALS:
        void textChanged();
        // 从可编码变为内容过长时发出一次，reason 为中文原因；保持过长期间继续修改文本不会重复发出
        void encodingFailed(const QString& reason);

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADQRCODE_H
