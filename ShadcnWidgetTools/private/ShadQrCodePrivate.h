#ifndef SHADQRCODEPRIVATE_H
#define SHADQRCODEPRIVATE_H

#include <QColor>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"
#include "ShadQrEncoder.h"

class ShadQrCode;

class ShadQrCodePrivate
{
    SHAD_D_CREATE(ShadQrCode)

    public:
        ShadQrCodePrivate();
        ~ShadQrCodePrivate();

        static constexpr int kDefaultSize = 160;
        static constexpr int kDefaultQuietZone = 4;
        static constexpr int kBorderWidth = 1;
        // 错误状态文字与边框之间的内边距
        static constexpr int kErrorPadding = 12;

        // 按当前文本与纠错等级重新编码，更新格子数据与状态；编码失败时发出 encodingFailed
        void encode();
        bool isDark(int x, int y) const;

        QString _text;
        ShadQrCodeType::ErrorCorrection _level{ShadQrCodeType::Medium};
        int _size{kDefaultSize};
        int _quietZone{kDefaultQuietZone};
        QColor _foregroundColor{0, 0, 0};
        QColor _backgroundColor{255, 255, 255};
        // 当前符号，编码失败或文本为空时版本与尺寸为 0
        ShadQrSymbol _symbol;
        bool _isTooLong{false};
};

#endif // SHADQRCODEPRIVATE_H
