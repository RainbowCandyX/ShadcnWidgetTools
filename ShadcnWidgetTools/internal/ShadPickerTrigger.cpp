#include "ShadPickerTrigger.h"

#include <QColor>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadButtonPrivate.h"
#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 图标 size-4
    constexpr int kIconSize = 16;
} // namespace

namespace ShadPickerTrigger
{
    void paint(QPainter* painter, const QWidget* widget, const Options& options)
    {
        using namespace ShadStyleHelper;
        const ShadButtonPrivate::VariantColors colors = ShadButtonPrivate::variantColors(ShadButtonType::Outline);
        const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(ShadButtonType::Medium);
        const QRect bounds = widget->rect();
        const Qt::LayoutDirection direction = widget->layoutDirection();
        const QRectF bodyRect = insetRect(bounds, kFocusRingWidth);
        const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);

        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter->setOpacity(widget->isEnabled() ? 1.0 : kDisabledOpacity);
        drawShadowXs(painter, bodyRect, radius);
        fillRoundedRect(painter, bodyRect, radius, mix(colors.background, colors.hoverBackground, options.hoverProgress));
        strokeRoundedRect(painter, bodyRect, radius, options.showFocusRing ? shadTheme->getColor(ShadThemeType::Ring) : colors.border);
        if (options.showFocusRing)
        {
            drawFocusRing(painter, bodyRect, radius, focusRingColor(false));
        }

        const bool hasLeadingIcon = options.leadingIcon != ShadIconType::None;
        const int padding = (hasLeadingIcon || options.hasChevron) ? metrics.paddingXWithIcon : metrics.paddingX;
        const QRect contentRect = bodyRect.toAlignedRect().adjusted(padding, 0, -padding, 0);
        const int iconTop = contentRect.center().y() - kIconSize / 2 + 1;
        const QColor textColor = mix(colors.text, colors.hoverText, options.hoverProgress);
        const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
        int textLeft = contentRect.left();
        int textRight = contentRect.right() + 1;
        if (hasLeadingIcon)
        {
            const QRect iconRect(textLeft, iconTop, kIconSize, kIconSize);
            ShadIcon::paint(painter, QRectF(visualRect(direction, bounds, iconRect)), options.leadingIcon, textColor, kIconSize);
            textLeft += kIconSize + metrics.gap;
        }
        if (options.hasChevron)
        {
            const QRect chevronRect(textRight - kIconSize, iconTop, kIconSize, kIconSize);
            ShadIcon::paint(painter, QRectF(visualRect(direction, bounds, chevronRect)), ShadIconType::ChevronDown, mutedColor, kIconSize);
            textRight -= kIconSize + metrics.gap;
        }
        const bool isEmpty = options.text.isEmpty();
        const QRect textRect(textLeft, contentRect.top(), qMax(0, textRight - textLeft), contentRect.height());
        painter->setFont(widget->font());
        painter->setPen(isEmpty ? mutedColor : textColor);
        painter->drawText(visualRect(direction, bounds, textRect), visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(widget->font(), isEmpty ? options.placeholderText : options.text, textRect.width()));
        painter->restore();
    }

    QSize sizeHint(const QFont& font, const QStringList& texts, bool hasLeadingIcon, bool hasChevron)
    {
        const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(ShadButtonType::Medium);
        const int padding = (hasLeadingIcon || hasChevron) ? metrics.paddingXWithIcon : metrics.paddingX;
        int textWidth = 0;
        for (const QString& text : texts)
        {
            textWidth = qMax(textWidth, ShadStyleHelper::textWidth(font, text));
        }
        int width = 2 * padding + textWidth;
        if (hasLeadingIcon)
        {
            width += kIconSize + metrics.gap;
        }
        if (hasChevron)
        {
            width += kIconSize + metrics.gap;
        }
        const int focusMargin = 2 * ShadStyleHelper::kFocusRingWidth;
        return QSize(width + focusMargin, metrics.height + focusMargin);
    }

    QSize minimumSizeHint()
    {
        const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(ShadButtonType::Medium);
        const int focusMargin = 2 * ShadStyleHelper::kFocusRingWidth;
        return QSize(metrics.height + focusMargin, metrics.height + focusMargin);
    }
} // namespace ShadPickerTrigger
