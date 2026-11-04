#include "ShadQrCodePrivate.h"

#include <QString>
#include <cstddef>

#include "ShadQrCode.h"
#include "ShadQrEncoder.h"

ShadQrCodePrivate::ShadQrCodePrivate()
{
}

ShadQrCodePrivate::~ShadQrCodePrivate()
{
}

void ShadQrCodePrivate::encode()
{
    Q_Q(ShadQrCode);
    const bool wasTooLong = _isTooLong;
    _symbol = ShadQrSymbol();
    _isTooLong = false;
    if (_text.isEmpty())
    {
        return;
    }
    if (ShadQrEncoder::encode(_text.toUtf8(), _level, &_symbol))
    {
        return;
    }
    // 不忽略失败：界面显示错误状态并通知调用方，不输出日志；持续输入时只在刚变为过长时通知一次
    _isTooLong = true;
    if (!wasTooLong)
    {
        Q_EMIT q->encodingFailed(ShadQrCode::tr("内容过长，当前纠错等级下无法生成二维码"));
    }
}

bool ShadQrCodePrivate::isDark(int x, int y) const
{
    return _symbol.modules[static_cast<std::size_t>(y) * static_cast<std::size_t>(_symbol.size) + static_cast<std::size_t>(x)];
}
