#include "ShadCardPrivate.h"

#include <QColor>
#include <QPalette>
#include <QWidget>

#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadTheme.h"

ShadCardPrivate::ShadCardPrivate()
{
}

ShadCardPrivate::~ShadCardPrivate()
{
}

void ShadCardPrivate::applyThemeColors()
{
    Q_Q(ShadCard);
    // 卡片内未单独设置颜色的子组件继承 card-foreground
    const QColor foreground = shadTheme->getColor(ShadThemeType::CardForeground);
    QPalette palette = q->palette();
    palette.setColor(QPalette::WindowText, foreground);
    palette.setColor(QPalette::Text, foreground);
    q->setPalette(palette);
    q->update();
}

void ShadCardPrivate::updateHeaderVisibility()
{
    const bool hasTitle = !_titleLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    _titleLabel->setVisible(hasTitle);
    _descriptionLabel->setVisible(hasDescription);
    _headerWidget->setVisible(hasTitle || hasDescription || _headerAction != nullptr);
}
