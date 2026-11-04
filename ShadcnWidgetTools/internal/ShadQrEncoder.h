#ifndef SHADQRENCODER_H
#define SHADQRENCODER_H

#include <QByteArray>
#include <vector>

#include "ShadDef.h"

// 二维码符号：版本、每边格子数与格子数据
struct ShadQrSymbol
{
    int version{0};
    int size{0};
    // 按行存储，true 为深色，长度为 size * size，不含静区
    std::vector<bool> modules;
};

// 二维码编码器，组件库内部使用，按 ISO/IEC 18004 QR Code Model 2 实现：
// 整段数据按数字、字母数字或字节模式之一编码（全部为数字时用数字模式，全部在字母数字字符集内时用字母数字模式，否则按字节），
// 字节模式下含有多字节 UTF-8 字符时在数据前加 UTF-8 的 ECI 声明（编号 26），扫码器据此按 UTF-8 解码，
// 自动选择能容纳数据的最小版本（1 到 40），按设置的纠错等级计算 Reed-Solomon 纠错码并交织，
// 填充功能图形、格式信息与版本信息，在 8 种掩码中选择罚分最低的一种
namespace ShadQrEncoder
{
    // 编码成功时写入 symbol 并返回 true；数据为空或超出该纠错等级下版本 40 的容量时返回 false，symbol 不变
    bool encode(const QByteArray& data, ShadQrCodeType::ErrorCorrection level, ShadQrSymbol* symbol);
} // namespace ShadQrEncoder

#endif // SHADQRENCODER_H
