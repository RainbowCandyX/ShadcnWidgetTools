#include "ShadTextareaPrivate.h"

#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QTextBlock>
#include <QTextDocument>
#include <QtGlobal>
#include <QtMath>

#include <initializer_list>

#include "ShadStyleHelper.h"
#include "ShadTextarea.h"
#include "ShadTheme.h"

ShadTextareaPrivate::ShadTextareaPrivate()
{
}

ShadTextareaPrivate::~ShadTextareaPrivate()
{
}

void ShadTextareaPrivate::updateAutoHeight()
{
    Q_Q(ShadTextarea);
    if (!_isAutoResize)
    {
        return;
    }
    // 累加各文本块在当前换行宽度下的高度，超过最大高度后无需继续累加；嵌入输入组时没有边框与焦点环边距
    const int margin = focusMargin();
    const int chromeHeight = 2 * frameInset() + 2 * paddingY() + qCeil(2 * q->document()->documentMargin());
    const qreal heightLimit = _maximumAutoHeight > 0 ? _maximumAutoHeight + 2 * margin - chromeHeight : -1.0;
    qreal textHeight = 0.0;
    for (QTextBlock block = q->document()->begin(); block.isValid(); block = block.next())
    {
        if (!block.isVisible())
        {
            continue;
        }
        textHeight += q->blockBoundingRect(block).height();
        if (heightLimit >= 0.0 && textHeight > heightLimit)
        {
            break;
        }
    }
    int visualHeight = chromeHeight + qCeil(textHeight) - 2 * margin;
    visualHeight = qMax(kMinVisualHeight, visualHeight);
    if (_maximumAutoHeight > 0)
    {
        visualHeight = qMin(qMax(kMinVisualHeight, _maximumAutoHeight), visualHeight);
    }
    const int widgetHeight = visualHeight + 2 * margin;
    if (widgetHeight == _autoHeight)
    {
        return;
    }
    _autoHeight = widgetHeight;
    q->updateGeometry();
}

void ShadTextareaPrivate::updateScrollBarPolicy()
{
    Q_Q(ShadTextarea);
    const bool isUnlimited = _isAutoResize && _maximumAutoHeight <= 0;
    q->setVerticalScrollBarPolicy(isUnlimited ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
}

void ShadTextareaPrivate::applyThemeColors()
{
    Q_Q(ShadTextarea);
    using ShadStyleHelper::kDisabledOpacity;
    using ShadStyleHelper::withAlpha;
    const QColor transparent(0, 0, 0, 0);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);

    QPalette palette = q->palette();
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        const qreal alpha = group == QPalette::Disabled ? kDisabledOpacity : 1.0;
        palette.setColor(group, QPalette::Base, transparent);
        palette.setColor(group, QPalette::Window, transparent);
        palette.setColor(group, QPalette::Text, withAlpha(foreground, alpha));
        palette.setColor(group, QPalette::PlaceholderText, withAlpha(placeholder, alpha));
        palette.setColor(group, QPalette::Highlight, selection);
        palette.setColor(group, QPalette::HighlightedText, selectionText);
    }
    q->setPalette(palette);
    q->update();
    q->viewport()->update();
}

bool ShadTextareaPrivate::isEmbedded() const
{
    Q_Q(const ShadTextarea);
    return ShadStyleHelper::isEmbedded(q);
}

int ShadTextareaPrivate::frameInset() const
{
    return isEmbedded() ? 0 : kFrameInset;
}

int ShadTextareaPrivate::paddingY() const
{
    // InputGroupTextarea 为 py-3
    return isEmbedded() ? kEmbeddedPaddingY : kPaddingY;
}

int ShadTextareaPrivate::focusMargin() const
{
    return isEmbedded() ? 0 : ShadStyleHelper::kFocusRingWidth;
}

void ShadTextareaPrivate::applyEmbedded()
{
    Q_Q(ShadTextarea);
    q->setLineWidth(frameInset());
    q->setViewportMargins(kPaddingX, paddingY(), kPaddingX, paddingY());
    _autoHeight = -1;
    updateAutoHeight();
    q->updateGeometry();
    q->update();
}

void ShadTextareaPrivate::paintFrame()
{
    Q_Q(ShadTextarea);
    using namespace ShadStyleHelper;
    // 嵌入输入组时外框与焦点环由输入组绘制
    if (isEmbedded())
    {
        return;
    }
    const QRectF bodyRect = insetRect(q->rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isFocused = q->hasFocus();
    QPainter painter(q);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(q->isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (isFocused || _isInvalid)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(_isInvalid));
    }
}
