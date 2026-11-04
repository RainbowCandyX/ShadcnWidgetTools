#include "ShadSelectBadges.h"

#include <QColor>
#include <QFontMetricsF>
#include <QPainter>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kBadgeFontSize = 12;
    constexpr int kBadgeHeight = 22;
    constexpr int kBadgePaddingX = 8;
    constexpr int kBadgeBorder = 1;
    constexpr int kBadgeGap = 4;
    // 第一个徽章截断后的最小宽度，再窄就只显示 +N
    constexpr int kMinBadgeWidth = 40;
} // namespace

namespace ShadSelectBadges
{
    void paint(QPainter* painter, const QRectF& rect, const QStringList& texts, const QFont& baseFont, Qt::LayoutDirection direction)
    {
        using namespace ShadStyleHelper;
        if (texts.isEmpty())
        {
            return;
        }
        QFont badgeFont = baseFont;
        badgeFont.setPixelSize(kBadgeFontSize);
        badgeFont.setWeight(QFont::Medium);
        const QFontMetricsF metrics(badgeFont);
        const qreal badgeHeight = qMin<qreal>(kBadgeHeight, rect.height());
        const qreal top = rect.center().y() - badgeHeight / 2.0;
        const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
        const QColor background = shadTheme->getColor(ShadThemeType::Secondary);
        const QColor textColor = shadTheme->getColor(ShadThemeType::SecondaryForeground);
        const auto badgeWidth = [&metrics](const QString& text) -> qreal { return metrics.horizontalAdvance(text) + 2 * (kBadgePaddingX + kBadgeBorder); };
        // x 按 LTR 计算，RTL 时镜像
        const auto drawBadge = [&](qreal x, qreal width, const QString& text)
        {
            const QRectF badgeRect = visualRect(direction, rect, QRectF(x, top, width, badgeHeight));
            fillRoundedRect(painter, badgeRect, radius, background);
            const QRectF textRect = badgeRect.adjusted(kBadgePaddingX + kBadgeBorder, 0, -(kBadgePaddingX + kBadgeBorder), 0);
            painter->setPen(textColor);
            painter->drawText(textRect, Qt::AlignCenter, metrics.elidedText(text, Qt::ElideRight, textRect.width()));
        };
        const auto overflowText = [](int count) { return QStringLiteral("+%1").arg(count); };

        painter->save();
        painter->setClipRect(rect);
        painter->setFont(badgeFont);
        const int count = int(texts.size());
        const qreal right = rect.right();
        qreal x = rect.left();
        int index = 0;
        for (; index < count; ++index)
        {
            // 后面还有未显示的项时，给 +N 徽章预留位置
            const qreal width = badgeWidth(texts.at(index));
            const int remaining = count - index - 1;
            const qreal reserve = remaining > 0 ? kBadgeGap + badgeWidth(overflowText(remaining)) : 0.0;
            if (x + width + reserve <= right)
            {
                drawBadge(x, width, texts.at(index));
                x += width + kBadgeGap;
                continue;
            }
            // 第一个徽章都放不下时截断文字，保证至少显示一个已选项
            if (index == 0)
            {
                const qreal elidedWidth = right - x - reserve;
                if (elidedWidth >= kMinBadgeWidth)
                {
                    drawBadge(x, elidedWidth, texts.at(0));
                    x += elidedWidth + kBadgeGap;
                    index = 1;
                }
            }
            break;
        }
        if (index < count)
        {
            const QString text = overflowText(count - index);
            drawBadge(x, badgeWidth(text), text);
        }
        painter->restore();
    }
} // namespace ShadSelectBadges
