#include "ShadInputPrivate.h"

#include <QColor>
#include <QLineEdit>
#include <QPainter>
#include <QPalette>
#include <QString>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadInput.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 1px 边框 + 焦点环边距
    constexpr int kBodyInset = 1 + ShadStyleHelper::kFocusRingWidth;
    // 水平内边距，对应 px-3
    constexpr int kPaddingX = 12;
    // 前置图标 size-4，与文字间距 gap-2
    constexpr int kLeadingIconSize = 16;
    constexpr int kLeadingIconGap = 8;
    // 尾部按钮 size-6，图标 size-3.5，距右边框 6px，与文字间距 4px
    constexpr int kTrailingButtonSize = 24;
    constexpr int kTrailingIconSize = 14;
    constexpr int kTrailingRightGap = 6;
    constexpr int kTrailingTextGap = 4;
} // namespace

ShadInputPrivate::ShadInputPrivate()
{
}

ShadInputPrivate::~ShadInputPrivate()
{
}

void ShadInputPrivate::applyThemeColors()
{
    Q_Q(ShadInput);
    using ShadStyleHelper::withAlpha;
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    // 起始侧放前置图标，结束侧放尾部按钮；样式表的 padding 是物理方向，RTL 时左右对调
    // 嵌入输入组时没有边框与焦点环边距，水平内边距由输入组的附加区负责
    const int inset = bodyInset();
    const int paddingX = isEmbedded() ? 0 : kPaddingX;
    const int paddingStart = inset + paddingX + (_leadingIcon != ShadIconType::None ? kLeadingIconSize + kLeadingIconGap : 0);
    const int paddingEnd = _trailingAction != ShadInputType::NoAction ? inset + kTrailingRightGap + kTrailingButtonSize + kTrailingTextGap : inset + paddingX;
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    const int paddingLeft = isRightToLeft ? paddingEnd : paddingStart;
    const int paddingRight = isRightToLeft ? paddingStart : paddingEnd;

    // 背景、边框、图标由 paintEvent 自绘，样式表只负责清空原生外观并设置文字颜色与内边距
    const QString styleSheet = QStringLiteral("QLineEdit{background-color:transparent;border:none;padding-left:%1px;padding-right:%2px;color:%3;selection-background-color:%4;selection-color:%5;}QLineEdit:disabled{color:%6;}").arg(QString::number(paddingLeft), QString::number(paddingRight), foreground.name(QColor::HexArgb), selection.name(QColor::HexArgb), selectionText.name(QColor::HexArgb), withAlpha(foreground, ShadStyleHelper::kDisabledOpacity).name(QColor::HexArgb));
    q->setStyleSheet(styleSheet);

    // 占位符颜色样式表无法设置，通过调色板设置
    QPalette palette = q->palette();
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, withAlpha(placeholder, ShadStyleHelper::kDisabledOpacity));
    q->setPalette(palette);
    q->update();
}

QRectF ShadInputPrivate::leadingIconRect() const
{
    Q_Q(const ShadInput);
    // 按 LTR 计算，RTL 时镜像到右侧
    const QRectF logicalRect(bodyInset() + (isEmbedded() ? 0 : kPaddingX), (q->height() - kLeadingIconSize) / 2, kLeadingIconSize, kLeadingIconSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalRect);
}

QRectF ShadInputPrivate::trailingButtonRect() const
{
    Q_Q(const ShadInput);
    // 按 LTR 计算，RTL 时镜像到左侧
    const QRectF logicalRect(q->width() - bodyInset() - kTrailingRightGap - kTrailingButtonSize, (q->height() - kTrailingButtonSize) / 2, kTrailingButtonSize, kTrailingButtonSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalRect);
}

bool ShadInputPrivate::isEmbedded() const
{
    Q_Q(const ShadInput);
    return ShadStyleHelper::isEmbedded(q);
}

int ShadInputPrivate::bodyInset() const
{
    return isEmbedded() ? 0 : kBodyInset;
}

bool ShadInputPrivate::isTrailingVisible() const
{
    Q_Q(const ShadInput);
    switch (_trailingAction)
    {
        case ShadInputType::ClearAction:
            return !q->text().isEmpty() && q->isEnabled() && !q->isReadOnly();
        case ShadInputType::PasswordToggleAction:
            return true;
        case ShadInputType::NoAction:
            break;
    }
    return false;
}

ShadIconType::IconName ShadInputPrivate::trailingIcon() const
{
    switch (_trailingAction)
    {
        case ShadInputType::ClearAction:
            return ShadIconType::X;
        case ShadInputType::PasswordToggleAction:
            return _isPasswordVisible ? ShadIconType::EyeOff : ShadIconType::Eye;
        case ShadInputType::NoAction:
            break;
    }
    return ShadIconType::None;
}

QString ShadInputPrivate::trailingToolTip() const
{
    switch (_trailingAction)
    {
        case ShadInputType::ClearAction:
            return ShadInput::tr("清空");
        case ShadInputType::PasswordToggleAction:
            return _isPasswordVisible ? ShadInput::tr("隐藏密码") : ShadInput::tr("显示密码");
        case ShadInputType::NoAction:
            break;
    }
    return QString();
}

void ShadInputPrivate::triggerTrailingAction()
{
    Q_Q(ShadInput);
    if (!q->isEnabled())
    {
        return;
    }
    if (_trailingAction == ShadInputType::ClearAction)
    {
        q->clear();
    }
    else if (_trailingAction == ShadInputType::PasswordToggleAction)
    {
        _isPasswordVisible = !_isPasswordVisible;
        q->setEchoMode(_isPasswordVisible ? QLineEdit::Normal : QLineEdit::Password);
    }
    q->update();
}

void ShadInputPrivate::updateTrailingHover(const QPoint& pos)
{
    Q_Q(ShadInput);
    const bool isHovered = isTrailingVisible() && q->isEnabled() && trailingButtonRect().contains(pos);
    if (isHovered == _isTrailingHovered)
    {
        return;
    }
    _isTrailingHovered = isHovered;
    // 悬停在尾部按钮上时显示手型光标，其余区域保持文本光标
    q->setCursor(isHovered ? Qt::PointingHandCursor : Qt::IBeamCursor);
    q->update();
}

void ShadInputPrivate::paintDecorations(QPainter* painter) const
{
    using ShadStyleHelper::withAlpha;
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    if (_leadingIcon != ShadIconType::None)
    {
        ShadIcon::paint(painter, leadingIconRect(), _leadingIcon, mutedForeground, kLeadingIconSize);
    }
    if (!isTrailingVisible())
    {
        return;
    }
    const QRectF buttonRect = trailingButtonRect();
    // 对应 InputGroupButton：ghost 风格，hover:bg-accent（暗色 accent/50），圆角 radius - 5px
    if (_isTrailingHovered)
    {
        const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
        const qreal radius = qMax(0, shadTheme->getBaseRadius() - 5);
        ShadStyleHelper::fillRoundedRect(painter, buttonRect, radius, shadTheme->isDark() ? withAlpha(accent, 0.5) : accent);
    }
    const QColor iconColor = _isTrailingHovered ? shadTheme->getColor(ShadThemeType::Foreground) : mutedForeground;
    ShadIcon::paint(painter, buttonRect, trailingIcon(), iconColor, kTrailingIconSize);
}
