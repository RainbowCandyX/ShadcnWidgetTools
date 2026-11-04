#include "ShadAlertPrivate.h"

#include <QLabel>
#include <QMargins>
#include <QPalette>
#include <QRect>
#include <QWidget>
#include <QtGlobal>

#include "ShadAlert.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 1px 边框 + px-4 py-3
    constexpr int kBorderWidth = 1;
    constexpr int kPaddingX = 16;
    constexpr int kPaddingY = 12;
    // 图标 size-4，与文字间距 gap-x-2.5
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 10;
    // 有操作组件时右内边距 pr-18，操作组件位于 top-2.5 right-3
    constexpr int kActionPaddingRight = 72;
    constexpr int kActionTop = 10;
    constexpr int kActionRight = 12;
    // 标题与描述间距 gap-0.5
    constexpr int kTextGap = 2;
} // namespace

ShadAlertPrivate::ShadAlertPrivate()
{
}

ShadAlertPrivate::~ShadAlertPrivate()
{
}

const ShadAlertPrivate::CustomColors& ShadAlertPrivate::currentCustomColors() const
{
    return shadTheme->isDark() ? _darkColors : _lightColors;
}

QColor ShadAlertPrivate::backgroundColor() const
{
    // 两种变体都是 bg-card
    const QColor custom = currentCustomColors().background;
    return custom.isValid() ? custom : shadTheme->getColor(ShadThemeType::Card);
}

QColor ShadAlertPrivate::borderColor() const
{
    const QColor custom = currentCustomColors().border;
    return custom.isValid() ? custom : shadTheme->getColor(ShadThemeType::Border);
}

QColor ShadAlertPrivate::foregroundColor() const
{
    const QColor custom = currentCustomColors().foreground;
    if (custom.isValid())
    {
        return custom;
    }
    return shadTheme->getColor(_variant == ShadAlertType::Destructive ? ShadThemeType::Destructive : ShadThemeType::CardForeground);
}

QColor ShadAlertPrivate::descriptionColor() const
{
    // destructive 变体描述为 text-destructive/90，其余为 text-muted-foreground
    if (_variant == ShadAlertType::Destructive && !currentCustomColors().foreground.isValid())
    {
        return ShadStyleHelper::withAlpha(shadTheme->getColor(ShadThemeType::Destructive), 0.9);
    }
    return shadTheme->getColor(ShadThemeType::MutedForeground);
}

void ShadAlertPrivate::updateContent()
{
    Q_Q(ShadAlert);
    QPalette titlePalette = _titleLabel->palette();
    titlePalette.setColor(QPalette::WindowText, foregroundColor());
    _titleLabel->setPalette(titlePalette);
    QPalette descriptionPalette = _descriptionLabel->palette();
    descriptionPalette.setColor(QPalette::WindowText, descriptionColor());
    _descriptionLabel->setPalette(descriptionPalette);
    _titleLabel->setVisible(!_titleLabel->text().isEmpty());
    _descriptionLabel->setVisible(!_descriptionLabel->text().isEmpty());
    // 只有一行文字时，保证操作组件上下各留出 top-2.5 的距离而不超出卡片
    if (_action.isNull())
    {
        q->setMinimumHeight(0);
    }
    else
    {
        const int actionBodyHeight = _action->sizeHint().height() - 2 * ShadStyleHelper::kFocusRingWidth;
        q->setMinimumHeight(2 * (kBorderWidth + kActionTop) + actionBodyHeight);
    }
    layoutLabels();
    updateActionGeometry();
    q->updateGeometry();
    q->update();
}

QMargins ShadAlertPrivate::textMargins() const
{
    // 有图标时文字从图标右侧开始（grid-cols-[auto_1fr]）
    const int left = kBorderWidth + kPaddingX + (_icon != ShadIconType::None ? kIconSize + kIconGap : 0);
    const int right = kBorderWidth + (_action.isNull() ? kPaddingX : kActionPaddingRight);
    return QMargins(left, kBorderWidth + kPaddingY, right, kBorderWidth + kPaddingY);
}

int ShadAlertPrivate::textHeightForWidth(int textWidth) const
{
    const bool hasTitle = !_titleLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    int height = 0;
    if (hasTitle)
    {
        height += _titleLabel->heightForWidth(textWidth);
    }
    if (hasTitle && hasDescription)
    {
        height += kTextGap;
    }
    if (hasDescription)
    {
        height += _descriptionLabel->heightForWidth(textWidth);
    }
    return height;
}

void ShadAlertPrivate::layoutLabels()
{
    Q_Q(ShadAlert);
    // textMargins 为逻辑边距（left 为起始侧），RTL 时文字区域在卡片内镜像
    const QMargins margins = textMargins();
    const int textWidth = qMax(1, q->width() - margins.left() - margins.right());
    const Qt::LayoutDirection direction = q->layoutDirection();
    int top = margins.top();
    if (!_titleLabel->text().isEmpty())
    {
        const int titleHeight = _titleLabel->heightForWidth(textWidth);
        _titleLabel->setGeometry(ShadStyleHelper::visualRect(direction, q->rect(), QRect(margins.left(), top, textWidth, titleHeight)));
        top += titleHeight + kTextGap;
    }
    if (!_descriptionLabel->text().isEmpty())
    {
        _descriptionLabel->setGeometry(ShadStyleHelper::visualRect(direction, q->rect(), QRect(margins.left(), top, textWidth, _descriptionLabel->heightForWidth(textWidth))));
    }
}

void ShadAlertPrivate::updateActionGeometry()
{
    Q_Q(ShadAlert);
    if (_action.isNull())
    {
        return;
    }
    // 本库可聚焦组件四周预留了焦点环边距，按可视边缘定位
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    const QSize size = _action->sizeHint();
    // 位于结束侧顶角：LTR 为右上角，RTL 为左上角
    const int x = q->width() - kBorderWidth - kActionRight - size.width() + focusMargin;
    const int y = kBorderWidth + kActionTop - focusMargin;
    _action->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), QRect(x, y, size.width(), size.height())));
    _action->raise();
}
