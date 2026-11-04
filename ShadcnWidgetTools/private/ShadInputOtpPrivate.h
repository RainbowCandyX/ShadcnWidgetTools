#ifndef SHADINPUTOTPPRIVATE_H
#define SHADINPUTOTPPRIVATE_H

#include <QElapsedTimer>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include "ShadProperty.h"

class QPainter;
class QTimer;
class ShadInputOtp;

class ShadInputOtpPrivate
{
    SHAD_D_CREATE(ShadInputOtp)

    public:
        ShadInputOtpPrivate();
        ~ShadInputOtpPrivate();

        // 组之间的间距 gap-2 与分隔符图标 size-4
        static constexpr int kGap = 8;
        static constexpr int kSeparatorSize = 16;
        // 假光标 h-4 w-px，闪烁周期 1s
        static constexpr int kCaretHeight = 16;
        static constexpr int kCaretPeriodMs = 1000;
        static constexpr int kCaretTickMs = 33;

        // 各格子、各组与各分隔符的区域（组件坐标），RTL 时已镜像
        struct Geometry
        {
            QList<QRect> slotRects;
            QList<QRect> groups;
            QList<QRect> separators;
        };

        Geometry geometry() const;

        int maxLength() const;
        // 格子是否为所在组的逻辑首格 / 尾格
        bool isGroupStart(int index) const;
        bool isGroupEnd(int index) const;
        // 获得焦点时的当前格，没有焦点时为 -1
        int activeIndex() const;
        // 光标所在的格子，不在格子上时返回 -1
        int slotAt(const QPoint& pos) const;
        // 值是否符合规则（空值总是有效）
        bool isAcceptable(const QString& value) const;

        // 从当前格开始写入文字：逐个字符替换或追加，不符合规则的字符跳过，到达总位数后停止；返回是否有变化
        bool insertText(const QString& text);
        void removeBefore();
        void removeAt();
        // 设置新值与光标，发出 valueChanged，填满时发出 completed
        void commit(const QString& value, int cursor, bool canComplete);
        void setCursor(int cursor);

        // 假光标的不透明度，对应 animate-caret-blink：0%、70%、100% 为 1，20% 到 50% 为 0，之间线性过渡
        qreal caretOpacity() const;
        // 当前格为空且有焦点时运行闪烁计时器
        void updateCaretTimer();
        void restartCaret();

        QList<int> _groups{3, 3};
        QString _value;
        // 插入位置，0 到值的长度
        int _cursor{0};
        QRegularExpression _pattern;
        bool _isInvalid{false};
        bool _isUpperCase{true};
        QSize _slotSize{36, 36};
        int _textPixelSize{14};
        QTimer* _caretTimer{nullptr};
        QElapsedTimer _caretClock;
};

#endif // SHADINPUTOTPPRIVATE_H
