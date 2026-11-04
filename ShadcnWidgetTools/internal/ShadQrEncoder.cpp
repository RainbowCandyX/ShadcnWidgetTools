#include "ShadQrEncoder.h"

#include <QByteArray>
#include <QtGlobal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace
{
    constexpr int kMaxVersion = 40;
    // 纠错等级在表格中的列序：Low、Medium、Quartile、High，与 ShadQrCodeType::ErrorCorrection 的数值一致
    constexpr int kLevelCount = 4;

    // 每个纠错块的纠错码字数与块数
    struct BlockLayout
    {
        int ecCodewordsPerBlock;
        int blockCount;
    };

    // 各版本的码字总数（数据码字 + 纠错码字），ISO/IEC 18004 表 1
    constexpr int kTotalCodewords[kMaxVersion] = {26, 44, 70, 100, 134, 172, 196, 242, 292, 346, 404, 466, 532, 581, 655, 733, 815, 901, 991, 1085, 1156, 1258, 1364, 1474, 1588, 1706, 1828, 1921, 2051, 2185, 2323, 2465, 2611, 2761, 2876, 3034, 3196, 3362, 3532, 3706};

    // 各版本、各纠错等级的分块方式，ISO/IEC 18004 表 9；每行依次为 Low、Medium、Quartile、High
    constexpr BlockLayout kBlockLayouts[kMaxVersion][kLevelCount] = {
        {{7, 1}, {10, 1}, {13, 1}, {17, 1}},
        {{10, 1}, {16, 1}, {22, 1}, {28, 1}},
        {{15, 1}, {26, 1}, {18, 2}, {22, 2}},
        {{20, 1}, {18, 2}, {26, 2}, {16, 4}},
        {{26, 1}, {24, 2}, {18, 4}, {22, 4}},
        {{18, 2}, {16, 4}, {24, 4}, {28, 4}},
        {{20, 2}, {18, 4}, {18, 6}, {26, 5}},
        {{24, 2}, {22, 4}, {22, 6}, {26, 6}},
        {{30, 2}, {22, 5}, {20, 8}, {24, 8}},
        {{18, 4}, {26, 5}, {24, 8}, {28, 8}},
        {{20, 4}, {30, 5}, {28, 8}, {24, 11}},
        {{24, 4}, {22, 8}, {26, 10}, {28, 11}},
        {{26, 4}, {22, 9}, {24, 12}, {22, 16}},
        {{30, 4}, {24, 9}, {20, 16}, {24, 16}},
        {{22, 6}, {24, 10}, {30, 12}, {24, 18}},
        {{24, 6}, {28, 10}, {24, 17}, {30, 16}},
        {{28, 6}, {28, 11}, {28, 16}, {28, 19}},
        {{30, 6}, {26, 13}, {28, 18}, {28, 21}},
        {{28, 7}, {26, 14}, {26, 21}, {26, 25}},
        {{28, 8}, {26, 16}, {30, 20}, {28, 25}},
        {{28, 8}, {26, 17}, {28, 23}, {30, 25}},
        {{28, 9}, {28, 17}, {30, 23}, {24, 34}},
        {{30, 9}, {28, 18}, {30, 25}, {30, 30}},
        {{30, 10}, {28, 20}, {30, 27}, {30, 32}},
        {{26, 12}, {28, 21}, {30, 29}, {30, 35}},
        {{28, 12}, {28, 23}, {28, 34}, {30, 37}},
        {{30, 12}, {28, 25}, {30, 34}, {30, 40}},
        {{30, 13}, {28, 26}, {30, 35}, {30, 42}},
        {{30, 14}, {28, 28}, {30, 38}, {30, 45}},
        {{30, 15}, {28, 29}, {30, 40}, {30, 48}},
        {{30, 16}, {28, 31}, {30, 43}, {30, 51}},
        {{30, 17}, {28, 33}, {30, 45}, {30, 54}},
        {{30, 18}, {28, 35}, {30, 48}, {30, 57}},
        {{30, 19}, {28, 37}, {30, 51}, {30, 60}},
        {{30, 19}, {28, 38}, {30, 53}, {30, 63}},
        {{30, 20}, {28, 40}, {30, 56}, {30, 66}},
        {{30, 21}, {28, 43}, {30, 59}, {30, 70}},
        {{30, 22}, {28, 45}, {30, 62}, {30, 74}},
        {{30, 24}, {28, 47}, {30, 65}, {30, 77}},
        {{30, 25}, {28, 49}, {30, 68}, {30, 81}},
    };

    // 对齐图形中心坐标，ISO/IEC 18004 附录 E；每行第一个数为坐标个数，行列坐标取同一组值
    constexpr int kAlignmentPositions[kMaxVersion][8] = {
        {0},
        {2, 6, 18},
        {2, 6, 22},
        {2, 6, 26},
        {2, 6, 30},
        {2, 6, 34},
        {3, 6, 22, 38},
        {3, 6, 24, 42},
        {3, 6, 26, 46},
        {3, 6, 28, 50},
        {3, 6, 30, 54},
        {3, 6, 32, 58},
        {3, 6, 34, 62},
        {4, 6, 26, 46, 66},
        {4, 6, 26, 48, 70},
        {4, 6, 26, 50, 74},
        {4, 6, 30, 54, 78},
        {4, 6, 30, 56, 82},
        {4, 6, 30, 58, 86},
        {4, 6, 34, 62, 90},
        {5, 6, 28, 50, 72, 94},
        {5, 6, 26, 50, 74, 98},
        {5, 6, 30, 54, 78, 102},
        {5, 6, 28, 54, 80, 106},
        {5, 6, 32, 58, 84, 110},
        {5, 6, 30, 58, 86, 114},
        {5, 6, 34, 62, 90, 118},
        {6, 6, 26, 50, 74, 98, 122},
        {6, 6, 30, 54, 78, 102, 126},
        {6, 6, 26, 52, 78, 104, 130},
        {6, 6, 30, 56, 82, 108, 134},
        {6, 6, 34, 60, 86, 112, 138},
        {6, 6, 30, 58, 86, 114, 142},
        {6, 6, 34, 62, 90, 118, 146},
        {7, 6, 30, 54, 78, 102, 126, 150},
        {7, 6, 24, 50, 76, 102, 128, 154},
        {7, 6, 28, 54, 80, 106, 132, 158},
        {7, 6, 32, 58, 84, 110, 136, 162},
        {7, 6, 26, 54, 82, 110, 138, 166},
        {7, 6, 30, 58, 86, 114, 142, 170},
    };

    // 格式信息中纠错等级的 2 位编码：Low 01、Medium 00、Quartile 11、High 10
    constexpr int kFormatLevelBits[kLevelCount] = {1, 0, 3, 2};
    // BCH(15,5) 生成多项式 x^10 + x^8 + x^5 + x^4 + x^2 + x + 1 与格式信息掩码
    constexpr int kFormatGenerator = 0x537;
    constexpr int kFormatMask = 0x5412;
    // BCH(18,6) 生成多项式 x^12 + x^11 + x^10 + x^9 + x^8 + x^5 + x^2 + 1
    constexpr int kVersionGenerator = 0x1F25;
    // GF(256) 的本原多项式 x^8 + x^4 + x^3 + x^2 + 1
    constexpr int kFieldPolynomial = 0x11D;
    // 填充码字
    constexpr std::uint32_t kPadCodewords[2] = {0xEC, 0x11};
    // ECI 模式指示符 0111 与 UTF-8 的 ECI 编号 26（小于 128，指示符为 1 字节），共 12 位
    constexpr std::uint32_t kEciModeIndicator = 0x7;
    constexpr std::uint32_t kUtf8EciAssignment = 26;
    constexpr int kUtf8EciBits = 4 + 8;
    // 罚分权重，ISO/IEC 18004 7.8.3
    constexpr int kPenaltySameRun = 3;
    constexpr int kPenaltyBlock = 3;
    constexpr int kPenaltyFinderLike = 40;
    constexpr int kPenaltyBalance = 10;

    enum class SegmentMode
    {
        Numeric,
        Alphanumeric,
        Byte,
    };

    // 字母数字模式的字符值，不在字符集内时返回 -1
    int alphanumericValue(char character)
    {
        if (character >= '0' && character <= '9')
        {
            return character - '0';
        }
        if (character >= 'A' && character <= 'Z')
        {
            return character - 'A' + 10;
        }
        switch (character)
        {
            case ' ':
                return 36;
            case '$':
                return 37;
            case '%':
                return 38;
            case '*':
                return 39;
            case '+':
                return 40;
            case '-':
                return 41;
            case '.':
                return 42;
            case '/':
                return 43;
            case ':':
                return 44;
            default:
                return -1;
        }
    }

    SegmentMode chooseMode(const QByteArray& data)
    {
        bool isNumeric = true;
        bool isAlphanumeric = true;
        for (const char character : data)
        {
            if (character < '0' || character > '9')
            {
                isNumeric = false;
            }
            if (alphanumericValue(character) < 0)
            {
                isAlphanumeric = false;
            }
        }
        if (isNumeric)
        {
            return SegmentMode::Numeric;
        }
        return isAlphanumeric ? SegmentMode::Alphanumeric : SegmentMode::Byte;
    }

    // 含有 0x80 及以上的字节（UTF-8 多字节字符）时返回 true
    bool hasNonAsciiByte(const QByteArray& data)
    {
        for (const char character : data)
        {
            if ((static_cast<unsigned char>(character) & 0x80U) != 0)
            {
                return true;
            }
        }
        return false;
    }

    std::uint32_t modeIndicator(SegmentMode mode)
    {
        switch (mode)
        {
            case SegmentMode::Numeric:
                return 0x1;
            case SegmentMode::Alphanumeric:
                return 0x2;
            case SegmentMode::Byte:
            default:
                return 0x4;
        }
    }

    // 字符计数指示符的位数，按版本 1-9、10-26、27-40 三档
    int characterCountBits(SegmentMode mode, int version)
    {
        const int range = version <= 9 ? 0 : (version <= 26 ? 1 : 2);
        constexpr int kNumericBits[3] = {10, 12, 14};
        constexpr int kAlphanumericBits[3] = {9, 11, 13};
        constexpr int kByteBits[3] = {8, 16, 16};
        switch (mode)
        {
            case SegmentMode::Numeric:
                return kNumericBits[range];
            case SegmentMode::Alphanumeric:
                return kAlphanumericBits[range];
            case SegmentMode::Byte:
            default:
                return kByteBits[range];
        }
    }

    // 数据部分（不含模式与计数指示符）的位数
    int payloadBits(SegmentMode mode, int count)
    {
        switch (mode)
        {
            case SegmentMode::Numeric:
                {
                    // 每 3 位数字 10 位，余 2 位数字 7 位，余 1 位数字 4 位
                    constexpr int kRemainderBits[3] = {0, 4, 7};
                    return count / 3 * 10 + kRemainderBits[count % 3];
                }
            case SegmentMode::Alphanumeric:
                // 每 2 个字符 11 位，余 1 个字符 6 位
                return count / 2 * 11 + count % 2 * 6;
            case SegmentMode::Byte:
            default:
                return count * 8;
        }
    }

    const BlockLayout& blockLayout(int version, int levelIndex)
    {
        return kBlockLayouts[version - 1][levelIndex];
    }

    int dataCodewordCount(int version, int levelIndex)
    {
        const BlockLayout& layout = blockLayout(version, levelIndex);
        return kTotalCodewords[version - 1] - layout.ecCodewordsPerBlock * layout.blockCount;
    }

    // 按位追加的缓冲区，高位在前
    class BitWriter
    {
        public:
            void append(std::uint32_t value, int bitCount)
            {
                for (int bit = bitCount - 1; bit >= 0; --bit)
                {
                    _bits.push_back(((value >> bit) & 1U) != 0);
                }
            }

            int size() const
            {
                return static_cast<int>(_bits.size());
            }

            std::vector<std::uint8_t> toBytes() const
            {
                std::vector<std::uint8_t> bytes(_bits.size() / 8, 0);
                for (std::size_t index = 0; index < _bits.size(); ++index)
                {
                    if (_bits[index])
                    {
                        bytes[index / 8] = static_cast<std::uint8_t>(bytes[index / 8] | (0x80U >> (index % 8)));
                    }
                }
                return bytes;
            }

        private:
            std::vector<bool> _bits;
    };

    // 按所选模式写入整段数据：可选的 UTF-8 ECI 声明、模式指示符、字符计数、数据、终止符、补齐到字节、填充码字，直到填满数据码字
    std::vector<std::uint8_t> buildDataCodewords(const QByteArray& data, SegmentMode mode, bool hasUtf8Eci, int version, int capacityBits)
    {
        BitWriter writer;
        const int count = static_cast<int>(data.size());
        // 字节模式默认按 ISO-8859-1 解释，部分扫码器不会自动识别 UTF-8，中文会显示为乱码；ECI 声明后续字节为 UTF-8
        if (hasUtf8Eci)
        {
            writer.append(kEciModeIndicator, 4);
            writer.append(kUtf8EciAssignment, 8);
        }
        writer.append(modeIndicator(mode), 4);
        writer.append(static_cast<std::uint32_t>(count), characterCountBits(mode, version));
        if (mode == SegmentMode::Numeric)
        {
            for (int index = 0; index < count; index += 3)
            {
                const int length = qMin(3, count - index);
                std::uint32_t value = 0;
                for (int offset = 0; offset < length; ++offset)
                {
                    value = value * 10 + static_cast<std::uint32_t>(data.at(index + offset) - '0');
                }
                writer.append(value, length * 3 + 1);
            }
        }
        else if (mode == SegmentMode::Alphanumeric)
        {
            for (int index = 0; index < count; index += 2)
            {
                if (index + 1 < count)
                {
                    const int value = alphanumericValue(data.at(index)) * 45 + alphanumericValue(data.at(index + 1));
                    writer.append(static_cast<std::uint32_t>(value), 11);
                }
                else
                {
                    writer.append(static_cast<std::uint32_t>(alphanumericValue(data.at(index))), 6);
                }
            }
        }
        else
        {
            for (const char character : data)
            {
                writer.append(static_cast<std::uint8_t>(character), 8);
            }
        }
        // 终止符最多 4 个 0，再补 0 到整字节，最后交替填充 0xEC、0x11
        writer.append(0, qMin(4, capacityBits - writer.size()));
        writer.append(0, (8 - writer.size() % 8) % 8);
        for (int padIndex = 0; writer.size() < capacityBits; ++padIndex)
        {
            writer.append(kPadCodewords[padIndex % 2], 8);
        }
        return writer.toBytes();
    }

    // GF(256) 的指数表与对数表，乘法用对数相加实现
    class GaloisField
    {
        public:
            static const GaloisField& instance()
            {
                static const GaloisField field;
                return field;
            }

            std::uint8_t power(int exponent) const
            {
                return _exp[exponent % 255];
            }

            std::uint8_t multiply(std::uint8_t left, std::uint8_t right) const
            {
                if (left == 0 || right == 0)
                {
                    return 0;
                }
                return _exp[_log[left] + _log[right]];
            }

        private:
            GaloisField()
            {
                int value = 1;
                for (int exponent = 0; exponent < 255; ++exponent)
                {
                    _exp[exponent] = static_cast<std::uint8_t>(value);
                    _log[value] = exponent;
                    value <<= 1;
                    if ((value & 0x100) != 0)
                    {
                        value ^= kFieldPolynomial;
                    }
                }
                // 指数表重复一遍，两个对数相加（最大 508）时不必再取模
                for (int exponent = 255; exponent < 512; ++exponent)
                {
                    _exp[exponent] = _exp[exponent - 255];
                }
            }

            std::uint8_t _exp[512]{};
            int _log[256]{};
    };

    // 纠错码生成多项式 (x + a^0)(x + a^1)...(x + a^(degree-1))，系数从最高次到常数项，首项为 1
    std::vector<std::uint8_t> generatorPolynomial(int degree)
    {
        const GaloisField& field = GaloisField::instance();
        std::vector<std::uint8_t> polynomial{1};
        for (int index = 0; index < degree; ++index)
        {
            const std::uint8_t root = field.power(index);
            std::vector<std::uint8_t> product(polynomial.size() + 1, 0);
            for (std::size_t term = 0; term < polynomial.size(); ++term)
            {
                product[term] = static_cast<std::uint8_t>(product[term] ^ polynomial[term]);
                product[term + 1] = static_cast<std::uint8_t>(product[term + 1] ^ field.multiply(polynomial[term], root));
            }
            polynomial = product;
        }
        return polynomial;
    }

    // 数据码字乘以 x^degree 后除以生成多项式，余式即纠错码字
    std::vector<std::uint8_t> errorCorrectionCodewords(const std::vector<std::uint8_t>& data, const std::vector<std::uint8_t>& generator)
    {
        const GaloisField& field = GaloisField::instance();
        const std::size_t degree = generator.size() - 1;
        std::vector<std::uint8_t> remainder(data);
        remainder.resize(data.size() + degree, 0);
        for (std::size_t index = 0; index < data.size(); ++index)
        {
            const std::uint8_t factor = remainder[index];
            if (factor == 0)
            {
                continue;
            }
            for (std::size_t term = 1; term <= degree; ++term)
            {
                remainder[index + term] = static_cast<std::uint8_t>(remainder[index + term] ^ field.multiply(generator[term], factor));
            }
        }
        return std::vector<std::uint8_t>(remainder.begin() + static_cast<std::ptrdiff_t>(data.size()), remainder.end());
    }

    // 数据码字分块并计算每块的纠错码字，再按列交织：先依次取各块数据码字，再依次取各块纠错码字
    // 码字总数不能被块数整除时，前面的块比后面的块少一个数据码字
    std::vector<std::uint8_t> interleaveCodewords(const std::vector<std::uint8_t>& dataCodewords, int version, int levelIndex)
    {
        const BlockLayout& layout = blockLayout(version, levelIndex);
        const int totalCodewords = kTotalCodewords[version - 1];
        const int blockCount = layout.blockCount;
        const int longBlockCount = totalCodewords % blockCount;
        const int shortDataLength = totalCodewords / blockCount - layout.ecCodewordsPerBlock;
        const std::vector<std::uint8_t> generator = generatorPolynomial(layout.ecCodewordsPerBlock);

        std::vector<std::vector<std::uint8_t>> dataBlocks;
        std::vector<std::vector<std::uint8_t>> ecBlocks;
        std::size_t offset = 0;
        for (int block = 0; block < blockCount; ++block)
        {
            const bool isLong = block >= blockCount - longBlockCount;
            const std::size_t length = static_cast<std::size_t>(shortDataLength + (isLong ? 1 : 0));
            std::vector<std::uint8_t> blockData(dataCodewords.begin() + static_cast<std::ptrdiff_t>(offset), dataCodewords.begin() + static_cast<std::ptrdiff_t>(offset + length));
            offset += length;
            ecBlocks.push_back(errorCorrectionCodewords(blockData, generator));
            dataBlocks.push_back(blockData);
        }

        std::vector<std::uint8_t> result;
        result.reserve(static_cast<std::size_t>(totalCodewords));
        for (int column = 0; column <= shortDataLength; ++column)
        {
            for (const std::vector<std::uint8_t>& blockData : dataBlocks)
            {
                if (column < static_cast<int>(blockData.size()))
                {
                    result.push_back(blockData[static_cast<std::size_t>(column)]);
                }
            }
        }
        for (int column = 0; column < layout.ecCodewordsPerBlock; ++column)
        {
            for (const std::vector<std::uint8_t>& ecBlock : ecBlocks)
            {
                result.push_back(ecBlock[static_cast<std::size_t>(column)]);
            }
        }
        return result;
    }

    // 15 位格式信息：纠错等级 2 位 + 掩码 3 位，附加 BCH 校验后与固定掩码异或
    int formatInformation(int levelIndex, int mask)
    {
        const int data = kFormatLevelBits[levelIndex] << 3 | mask;
        int remainder = data << 10;
        for (int bit = 14; bit >= 10; --bit)
        {
            if (((remainder >> bit) & 1) != 0)
            {
                remainder ^= kFormatGenerator << (bit - 10);
            }
        }
        return ((data << 10) | remainder) ^ kFormatMask;
    }

    // 18 位版本信息：版本号 6 位，附加 BCH 校验，只用于版本 7 及以上
    int versionInformation(int version)
    {
        int remainder = version << 12;
        for (int bit = 17; bit >= 12; --bit)
        {
            if (((remainder >> bit) & 1) != 0)
            {
                remainder ^= kVersionGenerator << (bit - 12);
            }
        }
        return (version << 12) | remainder;
    }

    // 掩码条件，row 为行号，column 为列号，ISO/IEC 18004 表 10
    bool maskCondition(int mask, int row, int column)
    {
        switch (mask)
        {
            case 0:
                return (row + column) % 2 == 0;
            case 1:
                return row % 2 == 0;
            case 2:
                return column % 3 == 0;
            case 3:
                return (row + column) % 3 == 0;
            case 4:
                return (row / 2 + column / 3) % 2 == 0;
            case 5:
                return (row * column) % 2 + (row * column) % 3 == 0;
            case 6:
                return ((row * column) % 2 + (row * column) % 3) % 2 == 0;
            case 7:
            default:
                return ((row + column) % 2 + (row * column) % 3) % 2 == 0;
        }
    }

    // 符号矩阵：记录每个格子的颜色，以及是否属于功能图形（功能图形不参与数据填充与掩码）
    class SymbolMatrix
    {
        public:
            SymbolMatrix(int version, int levelIndex)
                : _version(version), _levelIndex(levelIndex), _size(version * 4 + 17)
            {
                const std::size_t cellCount = static_cast<std::size_t>(_size) * static_cast<std::size_t>(_size);
                _dark.assign(cellCount, false);
                _function.assign(cellCount, false);
            }

            int size() const
            {
                return _size;
            }

            const std::vector<bool>& modules() const
            {
                return _dark;
            }

            void drawFunctionPatterns()
            {
                // 三个角的定位图形，含一圈浅色分隔符
                drawFinderPattern(0, 0);
                drawFinderPattern(_size - 7, 0);
                drawFinderPattern(0, _size - 7);
                // 第 6 行与第 6 列的时序图形，深浅交替
                for (int index = 8; index < _size - 8; ++index)
                {
                    setFunction(index, 6, index % 2 == 0);
                    setFunction(6, index, index % 2 == 0);
                }
                drawAlignmentPatterns();
                // 先按掩码 0 写入格式信息以占用这些位置，选定掩码后再覆盖
                drawFormatInformation(0);
                drawVersionInformation();
            }

            // 从右下角开始，每两列为一组上下往返填充，跳过第 6 列（垂直时序图形）与功能图形；剩余位（不足一个码字的格子）保持浅色
            void drawCodewords(const std::vector<std::uint8_t>& codewords)
            {
                const std::size_t totalBits = codewords.size() * 8;
                std::size_t bitIndex = 0;
                bool isUpward = true;
                for (int right = _size - 1; right >= 1; right -= 2)
                {
                    if (right == 6)
                    {
                        right = 5;
                    }
                    for (int step = 0; step < _size; ++step)
                    {
                        const int row = isUpward ? _size - 1 - step : step;
                        for (int offset = 0; offset < 2; ++offset)
                        {
                            const int column = right - offset;
                            if (isFunction(column, row))
                            {
                                continue;
                            }
                            bool isDarkBit = false;
                            if (bitIndex < totalBits)
                            {
                                isDarkBit = ((codewords[bitIndex / 8] >> (7 - bitIndex % 8)) & 1U) != 0;
                                ++bitIndex;
                            }
                            setDark(column, row, isDarkBit);
                        }
                    }
                    isUpward = !isUpward;
                }
            }

            // 依次尝试 8 种掩码，保留罚分最低的一种并写入对应的格式信息
            void applyBestMask()
            {
                const std::vector<bool> unmasked = _dark;
                int bestMask = 0;
                long bestPenalty = -1;
                for (int mask = 0; mask < 8; ++mask)
                {
                    _dark = unmasked;
                    applyMask(mask);
                    drawFormatInformation(mask);
                    const long penalty = penaltyScore();
                    if (bestPenalty < 0 || penalty < bestPenalty)
                    {
                        bestPenalty = penalty;
                        bestMask = mask;
                    }
                }
                _dark = unmasked;
                applyMask(bestMask);
                drawFormatInformation(bestMask);
            }

        private:
            std::size_t indexOf(int column, int row) const
            {
                return static_cast<std::size_t>(row) * static_cast<std::size_t>(_size) + static_cast<std::size_t>(column);
            }

            bool isDark(int column, int row) const
            {
                return _dark[indexOf(column, row)];
            }

            bool isFunction(int column, int row) const
            {
                return _function[indexOf(column, row)];
            }

            void setDark(int column, int row, bool dark)
            {
                _dark[indexOf(column, row)] = dark;
            }

            void setFunction(int column, int row, bool dark)
            {
                _dark[indexOf(column, row)] = dark;
                _function[indexOf(column, row)] = true;
            }

            // 7×7 定位图形：外圈深色、内圈浅色、中心 3×3 深色；四周超出 7×7 的一圈为浅色分隔符，超出符号范围的部分忽略
            void drawFinderPattern(int left, int top)
            {
                for (int dy = -1; dy <= 7; ++dy)
                {
                    for (int dx = -1; dx <= 7; ++dx)
                    {
                        const int column = left + dx;
                        const int row = top + dy;
                        if (column < 0 || column >= _size || row < 0 || row >= _size)
                        {
                            continue;
                        }
                        bool dark = false;
                        if (dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6)
                        {
                            const bool isBorder = dx == 0 || dx == 6 || dy == 0 || dy == 6;
                            const bool isCenter = dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4;
                            dark = isBorder || isCenter;
                        }
                        setFunction(column, row, dark);
                    }
                }
            }

            // 5×5 对齐图形：外圈深色、内圈浅色、中心深色；与三个定位图形重叠的位置不绘制
            void drawAlignmentPatterns()
            {
                const int* positions = kAlignmentPositions[_version - 1];
                const int count = positions[0];
                for (int rowIndex = 0; rowIndex < count; ++rowIndex)
                {
                    for (int columnIndex = 0; columnIndex < count; ++columnIndex)
                    {
                        const bool isFirstRow = rowIndex == 0;
                        const bool isFirstColumn = columnIndex == 0;
                        const bool isLastRow = rowIndex == count - 1;
                        const bool isLastColumn = columnIndex == count - 1;
                        if ((isFirstRow && isFirstColumn) || (isFirstRow && isLastColumn) || (isLastRow && isFirstColumn))
                        {
                            continue;
                        }
                        const int centerRow = positions[rowIndex + 1];
                        const int centerColumn = positions[columnIndex + 1];
                        for (int dy = -2; dy <= 2; ++dy)
                        {
                            for (int dx = -2; dx <= 2; ++dx)
                            {
                                const int distance = qMax(std::abs(dx), std::abs(dy));
                                setFunction(centerColumn + dx, centerRow + dy, distance != 1);
                            }
                        }
                    }
                }
            }

            // 格式信息两份：一份围绕左上角定位图形，一份分在右上角（第 8 行）与左下角（第 8 列）；bit 0 为最低位
            void drawFormatInformation(int mask)
            {
                const int bits = formatInformation(_levelIndex, mask);
                for (int bit = 0; bit < 15; ++bit)
                {
                    const bool dark = ((bits >> bit) & 1) != 0;
                    // 第一份：第 8 列自上而下（跳过第 6 行时序图形），再沿第 8 行从右向左（跳过第 6 列）
                    if (bit < 6)
                    {
                        setFunction(8, bit, dark);
                    }
                    else if (bit < 8)
                    {
                        setFunction(8, bit + 1, dark);
                    }
                    else if (bit == 8)
                    {
                        setFunction(7, 8, dark);
                    }
                    else
                    {
                        setFunction(14 - bit, 8, dark);
                    }
                    // 第二份：低 8 位沿第 8 行从右边缘向左，高 7 位沿第 8 列从下方第 7 格向下
                    if (bit < 8)
                    {
                        setFunction(_size - 1 - bit, 8, dark);
                    }
                    else
                    {
                        setFunction(8, _size - 15 + bit, dark);
                    }
                }
                // 左下角格式信息上方固定为深色的格子
                setFunction(8, _size - 8, true);
            }

            // 版本信息两份 6×3：左下角（第 0-5 列，倒数第 11-9 行）与右上角（倒数第 11-9 列，第 0-5 行），bit 0 为最低位
            void drawVersionInformation()
            {
                if (_version < 7)
                {
                    return;
                }
                const int bits = versionInformation(_version);
                for (int bit = 0; bit < 18; ++bit)
                {
                    const bool dark = ((bits >> bit) & 1) != 0;
                    const int along = bit / 3;
                    const int across = _size - 11 + bit % 3;
                    setFunction(along, across, dark);
                    setFunction(across, along, dark);
                }
            }

            void applyMask(int mask)
            {
                for (int row = 0; row < _size; ++row)
                {
                    for (int column = 0; column < _size; ++column)
                    {
                        if (!isFunction(column, row) && maskCondition(mask, row, column))
                        {
                            setDark(column, row, !isDark(column, row));
                        }
                    }
                }
            }

            // 行（isRow 为 true）或列方向上 [from, to) 范围内都是浅色时返回 true，超出符号范围的部分视为浅色
            bool isLightRange(int line, int from, int to, bool isRow) const
            {
                const int start = qMax(0, from);
                const int end = qMin(_size, to);
                for (int position = start; position < end; ++position)
                {
                    if (isRow ? isDark(position, line) : isDark(line, position))
                    {
                        return false;
                    }
                }
                return true;
            }

            // 罚分，ISO/IEC 18004 7.8.3：同色连续、2×2 同色块、类似定位图形的 1:1:3:1:1 序列、深色比例偏离 50%
            long penaltyScore() const
            {
                long penalty = 0;
                for (int line = 0; line < _size; ++line)
                {
                    for (int direction = 0; direction < 2; ++direction)
                    {
                        const bool isRow = direction == 0;
                        // 同色连续 5 格及以上：3 + (长度 - 5)
                        int runLength = 0;
                        bool runDark = false;
                        for (int position = 0; position < _size; ++position)
                        {
                            const bool dark = isRow ? isDark(position, line) : isDark(line, position);
                            if (position > 0 && dark == runDark)
                            {
                                ++runLength;
                                continue;
                            }
                            if (runLength >= 5)
                            {
                                penalty += kPenaltySameRun + (runLength - 5);
                            }
                            runDark = dark;
                            runLength = 1;
                        }
                        if (runLength >= 5)
                        {
                            penalty += kPenaltySameRun + (runLength - 5);
                        }
                        // 深浅深深深浅深，且一侧有 4 格浅色
                        for (int position = 0; position + 6 < _size; ++position)
                        {
                            const auto darkAt = [this, line, isRow](int offset) { return isRow ? isDark(offset, line) : isDark(line, offset); };
                            const bool isPattern = darkAt(position) && !darkAt(position + 1) && darkAt(position + 2) && darkAt(position + 3) && darkAt(position + 4) && !darkAt(position + 5) && darkAt(position + 6);
                            if (isPattern && (isLightRange(line, position - 4, position, isRow) || isLightRange(line, position + 7, position + 11, isRow)))
                            {
                                penalty += kPenaltyFinderLike;
                            }
                        }
                    }
                }
                // 2×2 同色块
                for (int row = 0; row + 1 < _size; ++row)
                {
                    for (int column = 0; column + 1 < _size; ++column)
                    {
                        const bool dark = isDark(column, row);
                        if (dark == isDark(column + 1, row) && dark == isDark(column, row + 1) && dark == isDark(column + 1, row + 1))
                        {
                            penalty += kPenaltyBlock;
                        }
                    }
                }
                // 深色比例每偏离 50% 达 5%，加 10 分
                long darkCount = 0;
                for (const bool dark : _dark)
                {
                    if (dark)
                    {
                        ++darkCount;
                    }
                }
                const long totalCount = static_cast<long>(_size) * static_cast<long>(_size);
                penalty += std::labs(darkCount * 2 - totalCount) * 10 / totalCount * kPenaltyBalance;
                return penalty;
            }

            int _version{1};
            int _levelIndex{0};
            int _size{21};
            std::vector<bool> _dark;
            std::vector<bool> _function;
    };
} // namespace

namespace ShadQrEncoder
{
    bool encode(const QByteArray& data, ShadQrCodeType::ErrorCorrection level, ShadQrSymbol* symbol)
    {
        if (data.isEmpty() || symbol == nullptr)
        {
            return false;
        }
        const int levelIndex = qBound(0, static_cast<int>(level), kLevelCount - 1);
        const SegmentMode mode = chooseMode(data);
        const int count = static_cast<int>(data.size());
        // 只有含多字节字符时才声明 UTF-8，纯 ASCII 内容与 ISO-8859-1 一致，不增加长度
        const bool hasUtf8Eci = mode == SegmentMode::Byte && hasNonAsciiByte(data);
        const int eciBits = hasUtf8Eci ? kUtf8EciBits : 0;
        // 选择能容纳数据的最小版本；字符数超出计数指示符的表示范围时该版本不可用
        int version = 0;
        for (int candidate = 1; candidate <= kMaxVersion; ++candidate)
        {
            const int countBits = characterCountBits(mode, candidate);
            if (count >= (1 << countBits))
            {
                continue;
            }
            const int requiredBits = eciBits + 4 + countBits + payloadBits(mode, count);
            if (requiredBits <= dataCodewordCount(candidate, levelIndex) * 8)
            {
                version = candidate;
                break;
            }
        }
        if (version == 0)
        {
            return false;
        }
        const std::vector<std::uint8_t> dataCodewords = buildDataCodewords(data, mode, hasUtf8Eci, version, dataCodewordCount(version, levelIndex) * 8);
        const std::vector<std::uint8_t> codewords = interleaveCodewords(dataCodewords, version, levelIndex);
        SymbolMatrix matrix(version, levelIndex);
        matrix.drawFunctionPatterns();
        matrix.drawCodewords(codewords);
        matrix.applyBestMask();
        symbol->version = version;
        symbol->size = matrix.size();
        symbol->modules = matrix.modules();
        return true;
    }
} // namespace ShadQrEncoder
