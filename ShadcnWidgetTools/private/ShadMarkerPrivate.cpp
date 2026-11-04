#include "ShadMarkerPrivate.h"

#include <QAbstractAnimation>
#include <QChar>
#include <QDesktopServices>
#include <QLatin1Char>
#include <QString>
#include <QTextLayout>
#include <QTextLine>
#include <QTextOption>
#include <QVariantAnimation>
#include <QWidget>
#include <QtMath>

#include "ShadMarker.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kMinimumRuleWidth = 24;
}

ShadMarkerPrivate::ShadMarkerPrivate()
{
}

ShadMarkerPrivate::~ShadMarkerPrivate()
{
}

bool ShadMarkerPrivate::hasIcon() const
{
    return _iconWidget != nullptr || _icon != ShadIconType::None;
}

QFont ShadMarkerPrivate::textFont() const
{
    Q_Q(const ShadMarker);
    QFont font = q->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Normal);
    font.setUnderline(!_url.isEmpty());
    return font;
}

QColor ShadMarkerPrivate::textColor() const
{
    return ShadStyleHelper::mix(shadTheme->getColor(ShadThemeType::MutedForeground), shadTheme->getColor(ShadThemeType::Foreground), _hoverTransition->value());
}

QStringList ShadMarkerPrivate::wrapText(qreal width) const
{
    QStringList lines;
    if (_text.isEmpty())
    {
        return lines;
    }
    QString text = _text;
    text.replace(QLatin1Char('\n'), QChar::LineSeparator);
    QTextLayout textLayout(text, textFont());
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(option);
    textLayout.beginLayout();
    while (true)
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
        {
            break;
        }
        line.setLineWidth(qMax<qreal>(1.0, width));
        lines.append(text.mid(line.textStart(), line.textLength()).trimmed());
    }
    textLayout.endLayout();
    return lines;
}

int ShadMarkerPrivate::naturalWidth() const
{
    const bool hasText = !_text.isEmpty();
    int width = hasIcon() ? kIconSize + (hasText ? kGap : 0) : 0;
    if (hasText)
    {
        int textWidth = 0;
        const QFont font = textFont();
        for (const QString& line : _text.split(QLatin1Char('\n')))
        {
            textWidth = qMax(textWidth, ShadStyleHelper::textWidth(font, line));
        }
        width += textWidth;
    }
    if (_variant == ShadMarkerType::Separator)
    {
        width += 2 * (kRuleGap + kMinimumRuleWidth);
    }
    return width + 2 * ShadStyleHelper::kFocusRingWidth;
}

ShadMarkerPrivate::Layout ShadMarkerPrivate::computeGeometry(int width) const
{
    Layout layout;
    const qreal margin = ShadStyleHelper::kFocusRingWidth;
    const qreal available = qMax<qreal>(0.0, width - 2.0 * margin);
    const bool showIcon = hasIcon();
    const bool hasText = !_text.isEmpty();
    if (_variant == ShadMarkerType::Separator)
    {
        const qreal centerY = margin + kLineHeight / 2.0;
        const QFont font = textFont();
        const qreal iconPart = showIcon ? kIconSize + (hasText ? kGap : 0) : 0.0;
        const qreal textNatural = hasText ? ShadStyleHelper::textWidth(font, _text) : 0.0;
        const qreal contentWidth = qMin(available, iconPart + textNatural);
        const qreal contentLeft = margin + (available - contentWidth) / 2.0;
        if (showIcon)
        {
            layout.icon = QRectF(contentLeft, centerY - kIconSize / 2.0, kIconSize, kIconSize);
        }
        if (hasText)
        {
            const qreal textWidth = qMax<qreal>(0.0, contentWidth - iconPart);
            layout.lines.append(ShadStyleHelper::elideText(font, _text, textWidth));
            layout.lineRects.append(QRectF(contentLeft + iconPart, margin, textWidth, kLineHeight));
        }
        const qreal ruleGap = showIcon || hasText ? kRuleGap : 0.0;
        const qreal ruleTop = qFloor(centerY);
        const qreal leadingWidth = contentLeft - ruleGap - margin;
        if (leadingWidth > 0.0)
        {
            layout.leadingRule = QRectF(margin, ruleTop, leadingWidth, kBorderWidth);
        }
        const qreal trailingLeft = contentLeft + contentWidth + ruleGap;
        const qreal trailingWidth = margin + available - trailingLeft;
        if (trailingWidth > 0.0)
        {
            layout.trailingRule = QRectF(trailingLeft, ruleTop, trailingWidth, kBorderWidth);
        }
        layout.height = qCeil(2.0 * margin + kLineHeight);
        return layout;
    }
    const qreal iconPart = showIcon ? kIconSize + kGap : 0.0;
    const qreal textLeft = margin + iconPart;
    const qreal textWidth = qMax<qreal>(0.0, available - iconPart);
    if (hasText)
    {
        layout.lines = wrapText(textWidth);
    }
    const qreal contentHeight = qMax<qreal>(kIconSize, layout.lines.size() * kLineHeight);
    for (int index = 0; index < layout.lines.size(); ++index)
    {
        layout.lineRects.append(QRectF(textLeft, margin + index * kLineHeight, textWidth, kLineHeight));
    }
    if (showIcon)
    {
        layout.icon = QRectF(margin, margin + (contentHeight - kIconSize) / 2.0, kIconSize, kIconSize);
    }
    qreal bottom = margin + contentHeight;
    if (_variant == ShadMarkerType::Border)
    {
        bottom += kBorderPadding;
        layout.border = QRectF(margin, qFloor(bottom), available, kBorderWidth);
        bottom += kBorderWidth;
    }
    layout.height = qCeil(bottom + margin);
    return layout;
}

void ShadMarkerPrivate::placeIconWidget()
{
    Q_Q(ShadMarker);
    if (_iconWidget == nullptr)
    {
        return;
    }
    const QRectF iconRect = computeGeometry(q->width()).icon;
    _iconWidget->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), iconRect).toAlignedRect());
}

void ShadMarkerPrivate::updateAnimations()
{
    Q_Q(ShadMarker);
    const bool isShown = q->isVisible();
    const bool shouldSpin = isShown && _iconWidget == nullptr && _icon == ShadIconType::LoaderCircle;
    if (shouldSpin && _spinAnimation->state() != QAbstractAnimation::Running)
    {
        _spinAnimation->start();
    }
    else if (!shouldSpin && _spinAnimation->state() == QAbstractAnimation::Running)
    {
        _spinAnimation->stop();
        _spinAngle = 0.0;
    }
    const bool shouldShimmer = isShown && _isShimmer && !_text.isEmpty();
    if (shouldShimmer && _shimmerAnimation->state() != QAbstractAnimation::Running)
    {
        _shimmerAnimation->start();
    }
    else if (!shouldShimmer && _shimmerAnimation->state() == QAbstractAnimation::Running)
    {
        _shimmerAnimation->stop();
        _shimmerProgress = 0.0;
    }
}

void ShadMarkerPrivate::activate()
{
    Q_Q(ShadMarker);
    if (!_url.isEmpty())
    {
        QDesktopServices::openUrl(_url);
    }
    Q_EMIT q->clicked();
}
