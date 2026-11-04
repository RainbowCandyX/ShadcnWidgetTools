#ifndef SHADINPUTOTP_H
#define SHADINPUTOTP_H

#include <QList>
#include <QRegularExpression>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QInputMethodEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 验证码输入，对应 shadcn InputOTP（基于 input-otp）：一排格子，每格一个字符，格子按组相连，组之间显示减号分隔符
// 格子 size-9 border-input，同组相邻格子共用边框，组的首尾为圆角，整组 shadow-xs；获得焦点时当前格为 border-ring 加 3px ring，空格显示闪烁的假光标
// 输入：输入字符后跳到下一格；左右方向键回到已输入的格子后输入会替换该格；Backspace 删除前一个字符，Delete 删除当前格，Home / End 跳到首尾
// 值总是从第一格开始的连续字符串，不会出现中间空着的格子；Ctrl+V 从当前格开始粘贴，按 pattern 过滤并截断到总位数，Ctrl+C 复制整个值
// RTL 下格子从右向左排列，左右方向键对调
class ShadInputOtpPrivate;

class SHADCN_EXPORT ShadInputOtp : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadInputOtp)
    Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)
    Q_PROPERTY(bool upperCase READ isUpperCase WRITE setUpperCase)
    Q_PROPERTY(QSize slotSize READ getSlotSize WRITE setSlotSize)
    Q_PROPERTY(int slotTextPixelSize READ getSlotTextPixelSize WRITE setSlotTextPixelSize)

    public:
        explicit ShadInputOtp(QWidget* parent = nullptr);
        ~ShadInputOtp() override;

        // 每组的格子数，例如 {3, 3} 为 6 位、中间一个分隔符；默认 {3, 3}，小于 1 的组被忽略，全部无效时为 {1}
        // 总位数减少时当前值截断
        void setGroups(const QList<int>& groups);
        QList<int> getGroups() const;
        // 总位数，等于各组格子数之和，对应 maxLength
        int getMaxLength() const;

        // 设置值：超出总位数的部分截断，不符合 pattern 时忽略
        void setValue(const QString& value);
        QString getValue() const;
        void clear();

        // 整个值必须匹配的规则，对应 pattern；无效或空的规则表示接受任意字符，默认接受任意字符
        void setPattern(const QRegularExpression& pattern);
        QRegularExpression getPattern() const;
        // 对应 REGEXP_ONLY_DIGITS / REGEXP_ONLY_CHARS / REGEXP_ONLY_DIGITS_AND_CHARS
        static QRegularExpression onlyDigits();
        static QRegularExpression onlyChars();
        static QRegularExpression onlyDigitsAndChars();

        // 输入、粘贴与 setValue 的字母自动转为大写，默认开启；关闭后保留原样
        void setUpperCase(bool upperCase);
        bool isUpperCase() const;

        // 错误态，对应 InputOTPSlot 的 aria-invalid：格子边框为 destructive 色，整组外围显示 destructive 圆环
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 每个格子的尺寸，默认 36 x 36（size-9）
        void setSlotSize(const QSize& size);
        QSize getSlotSize() const;
        // 格子内文字的像素大小，默认 14（text-sm）
        void setSlotTextPixelSize(int pixelSize);
        int getSlotTextPixelSize() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

    Q_SIGNALS:
        void valueChanged(const QString& value);
        // 输入或粘贴后填满全部格子时发出，对应 onComplete
        void completed(const QString& value);
        void invalidChanged();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void inputMethodEvent(QInputMethodEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
};

#endif // SHADINPUTOTP_H
