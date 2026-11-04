#include "ShadChartHelper.h"

#include <QFont>
#include <QIcon>
#include <QLineF>
#include <QLocale>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kTickSignificantDigits = 12;
    // 斜接长度上限为线宽一半的 2 倍，避免尖角过长
    constexpr qreal kMinMiterCos = 0.5;

    // 与 Recharts getFormatStep 相同：按位数把粗略步长向上取整到 0.05（或一位数时 0.1）的倍数，correction 每加 1 再放大一档
    qreal formatStep(qreal roughStep, int correction)
    {
        const int digitCount = static_cast<int>(std::floor(std::log10(roughStep))) + 1;
        const qreal digitValue = std::pow(10.0, digitCount);
        const qreal ratio = roughStep / digitValue;
        const qreal scale = digitCount != 1 ? 0.05 : 0.1;
        return (std::ceil(ratio / scale - 1e-9) + correction) * scale * digitValue;
    }

    quint8 colorByte(qreal value)
    {
        return static_cast<quint8>(qBound(0, qRound(value * 255.0), 255));
    }
} // namespace

namespace ShadChartHelper
{
    QList<qreal> niceTicks(qreal minValue, qreal maxValue, int count)
    {
        if (maxValue - minValue < 1e-12)
        {
            maxValue = minValue + 1.0;
        }
        QList<qreal> ticks;
        const qreal roughStep = (maxValue - minValue) / (count - 1);
        for (int correction = 0; correction < 64; ++correction)
        {
            const qreal step = formatStep(roughStep, correction);
            const qreal start = std::floor(minValue / step) * step;
            if (start + step * (count - 1) < maxValue - step * 1e-9)
            {
                continue;
            }
            for (int i = 0; i < count; ++i)
            {
                ticks.append(roundTick(start + step * i));
            }
            return ticks;
        }
        // 理论上不会到达：退化为等分
        for (int i = 0; i < count; ++i)
        {
            ticks.append(minValue + (maxValue - minValue) * i / (count - 1));
        }
        return ticks;
    }

    qreal roundTick(qreal value)
    {
        return QString::number(value, 'g', kTickSignificantDigits).toDouble();
    }

    QString formatValue(qreal value)
    {
        return QLocale().toString(value, 'f', QLocale::FloatingPointShortest);
    }

    QFont monospaceFont(const QFont& baseFont)
    {
        // 对应 ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas 等
        QFont font = baseFont;
        font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
        font.setStyleHint(QFont::Monospace);
        font.setWeight(QFont::Medium);
        return font;
    }

    QList<int> spacedLabels(const QList<qreal>& centers, const QList<qreal>& widths, qreal minGap)
    {
        QList<int> indices;
        qreal previousEnd = 0.0;
        for (qsizetype i = 0; i < centers.size() && i < widths.size(); ++i)
        {
            const qreal start = centers.at(i) - widths.at(i) / 2.0;
            if (!indices.isEmpty() && start < previousEnd + minGap)
            {
                continue;
            }
            indices.append(static_cast<int>(i));
            previousEnd = centers.at(i) + widths.at(i) / 2.0;
        }
        return indices;
    }

    void paintLegend(QPainter* painter, const QWidget* widget, const QList<LegendItem>& items)
    {
        if (items.isEmpty())
        {
            return;
        }
        const Qt::LayoutDirection direction = widget->layoutDirection();
        const QRectF bounds(widget->rect());
        const QFont font = widget->font();
        painter->setFont(font);

        QList<qreal> itemWidths;
        qreal totalWidth = 0.0;
        for (const LegendItem& item : items)
        {
            const int markerSize = item.icon != ShadIconType::None ? kLegendIconSize : kLegendSwatchSize;
            const qreal width = markerSize + kLegendInnerGap + ShadStyleHelper::textWidth(font, item.label);
            itemWidths.append(width);
            totalWidth += width;
        }
        totalWidth += kLegendItemGap * (items.size() - 1);

        // 按逻辑顺序从起始侧排列，RTL 时整体镜像
        qreal x = (widget->width() - totalWidth) / 2.0;
        const qreal y = widget->height() - kVerticalMargin - kLegendLineHeight;
        const Qt::Alignment textAlignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter);
        for (qsizetype index = 0; index < items.size(); ++index)
        {
            const LegendItem& item = items.at(index);
            int markerSize = kLegendSwatchSize;
            if (item.icon != ShadIconType::None)
            {
                // 图标为 text-muted-foreground
                markerSize = kLegendIconSize;
                const QRectF iconRect(x, y + (kLegendLineHeight - kLegendIconSize) / 2.0, kLegendIconSize, kLegendIconSize);
                ShadStyleHelper::drawIconText(painter, ShadStyleHelper::visualRect(direction, bounds, iconRect), item.icon, QIcon(), QString(), font, shadTheme->getColor(ShadThemeType::MutedForeground), 0.0, Qt::AlignCenter, kLegendIconSize);
            }
            else
            {
                const QRectF swatchRect(x, y + (kLegendLineHeight - kLegendSwatchSize) / 2.0, kLegendSwatchSize, kLegendSwatchSize);
                ShadStyleHelper::fillRoundedRect(painter, ShadStyleHelper::visualRect(direction, bounds, swatchRect), 2.0, item.color);
            }
            const qreal textX = x + markerSize + kLegendInnerGap;
            const QRectF textRect(textX, y, itemWidths.at(index) - (textX - x), kLegendLineHeight);
            painter->setFont(font);
            painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, textRect), textAlignment | Qt::TextDontClip, item.label);
            x += itemWidths.at(index) + kLegendItemGap;
        }
    }

    void paintTooltip(QPainter* painter, const QWidget* widget, qreal anchorX, qreal mouseY, const QString& title, const QList<TooltipRow>& rows, ShadChartType::TooltipIndicator indicator)
    {
        const Qt::LayoutDirection direction = widget->layoutDirection();
        QFont labelFont = widget->font();
        labelFont.setWeight(QFont::Medium);
        const QFont nameFont = widget->font();
        const QFont valueFont = monospaceFont(widget->font());

        // 颜色标记宽度：dot 与图标 10，line 4，dashed 3，没有标记为 0
        const auto indicatorWidth = [indicator](const TooltipRow& row)
        {
            if (row.icon != ShadIconType::None)
            {
                return 10;
            }
            if (!row.color.isValid())
            {
                return 0;
            }
            if (indicator == ShadChartType::Dot)
            {
                return 10;
            }
            return indicator == ShadChartType::Line ? 4 : 3;
        };
        const auto indicatorSpace = [&indicatorWidth](const TooltipRow& row)
        {
            const int width = indicatorWidth(row);
            return width > 0 ? width + kTooltipIndicatorGap : 0;
        };
        int contentWidth = ShadStyleHelper::textWidth(labelFont, title);
        for (const TooltipRow& row : rows)
        {
            const int rowWidth = indicatorSpace(row) + ShadStyleHelper::textWidth(nameFont, row.name) + kTooltipValueGap + ShadStyleHelper::textWidth(valueFont, row.value);
            contentWidth = qMax(contentWidth, rowWidth);
        }
        const int width = qMax(kTooltipMinWidth, contentWidth + 2 * kTooltipPaddingX);
        int height = 2 * kTooltipPaddingY + kTooltipLabelHeight;
        if (!rows.isEmpty())
        {
            const int rowCount = static_cast<int>(rows.size());
            height += kTooltipRowGap + rowCount * kTooltipRowHeight + (rowCount - 1) * kTooltipRowGap;
        }

        const bool isRtl = direction == Qt::RightToLeft;
        qreal left = isRtl ? anchorX - kTooltipOffset - width : anchorX + kTooltipOffset;
        if (isRtl ? left < 0.0 : left + width > widget->width())
        {
            left = isRtl ? anchorX + kTooltipOffset : anchorX - kTooltipOffset - width;
        }
        left = qBound(0.0, left, qMax(0.0, static_cast<qreal>(widget->width() - width)));
        qreal top = mouseY + kTooltipOffset;
        if (top + height > widget->height())
        {
            top = mouseY - kTooltipOffset - height;
        }
        top = qBound(0.0, top, qMax(0.0, static_cast<qreal>(widget->height() - height)));
        const QRectF cardRect(std::round(left), std::round(top), width, height);

        // 卡片：rounded-lg border-border/50 bg-background shadow-xl
        const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
        ShadStyleHelper::drawShadowLg(painter, cardRect, radius);
        // 不随窗口特效变透明
        ShadStyleHelper::fillRoundedRect(painter, cardRect, radius, shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background));
        ShadStyleHelper::strokeRoundedRect(painter, cardRect, radius, ShadStyleHelper::withAlpha(shadTheme->getColor(ShadThemeType::Border), 0.5), 1.0);

        // 内容按 LTR 逻辑坐标排列，在卡片内镜像
        const QRectF inner = cardRect.adjusted(kTooltipPaddingX, kTooltipPaddingY, -kTooltipPaddingX, -kTooltipPaddingY);
        const Qt::Alignment startAlignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter);
        const Qt::Alignment endAlignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter);
        painter->setFont(labelFont);
        painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter->drawText(ShadStyleHelper::visualRect(direction, cardRect, QRectF(inner.left(), inner.top(), inner.width(), kTooltipLabelHeight)), startAlignment | Qt::TextDontClip, title);

        qreal rowTop = inner.top() + kTooltipLabelHeight + kTooltipRowGap;
        for (const TooltipRow& row : rows)
        {
            const int markerWidth = indicatorWidth(row);
            if (row.icon != ShadIconType::None)
            {
                const QRectF iconRect(inner.left(), rowTop + (kTooltipRowHeight - 10) / 2.0, 10, 10);
                ShadStyleHelper::drawIconText(painter, ShadStyleHelper::visualRect(direction, cardRect, iconRect), row.icon, QIcon(), QString(), nameFont, shadTheme->getColor(ShadThemeType::MutedForeground), 0.0, Qt::AlignCenter, 10);
            }
            else if (markerWidth > 0 && indicator == ShadChartType::Dashed)
            {
                // border-[1.5px] border-dashed：宽 3px 的虚线框
                const QRectF dashedRect = ShadStyleHelper::visualRect(direction, cardRect, QRectF(inner.left(), rowTop, markerWidth, kTooltipRowHeight));
                painter->save();
                painter->setPen(QPen(row.color, 1.5, Qt::DashLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(dashedRect.adjusted(0.75, 0.75, -0.75, -0.75));
                painter->restore();
            }
            else if (markerWidth > 0)
            {
                // dot 为 10x10 圆角 2 的方块；line 为 4px 宽、与行同高
                const qreal markerHeight = indicator == ShadChartType::Dot ? 10.0 : kTooltipRowHeight;
                const QRectF markerRect(inner.left(), rowTop + (kTooltipRowHeight - markerHeight) / 2.0, markerWidth, markerHeight);
                ShadStyleHelper::fillRoundedRect(painter, ShadStyleHelper::visualRect(direction, cardRect, markerRect), 2.0, row.color);
            }
            const qreal nameLeft = inner.left() + indicatorSpace(row);
            const QRectF rowRect(nameLeft, rowTop, inner.right() - nameLeft, kTooltipRowHeight);
            painter->setFont(nameFont);
            painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
            painter->drawText(ShadStyleHelper::visualRect(direction, cardRect, rowRect), startAlignment | Qt::TextDontClip, row.name);
            painter->setFont(valueFont);
            painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
            painter->drawText(ShadStyleHelper::visualRect(direction, cardRect, rowRect), endAlignment | Qt::TextDontClip, row.value);
            rowTop += kTooltipRowHeight + kTooltipRowGap;
        }
    }

    ShadChartVertex makeVertex(const QPointF& point, const QColor& color, qreal alpha)
    {
        const qreal a = qBound<qreal>(0.0, color.alphaF() * alpha, 1.0);
        ShadChartVertex vertex;
        vertex.x = static_cast<float>(point.x());
        vertex.y = static_cast<float>(point.y());
        vertex.r = colorByte(color.redF() * a);
        vertex.g = colorByte(color.greenF() * a);
        vertex.b = colorByte(color.blueF() * a);
        vertex.a = colorByte(a);
        return vertex;
    }

    void beginStrip(QList<ShadChartVertex>& vertices, const ShadChartVertex& first)
    {
        if (vertices.isEmpty())
        {
            return;
        }
        const ShadChartVertex last = vertices.last();
        vertices.append(last);
        vertices.append(first);
    }

    void appendRect(QList<ShadChartVertex>& vertices, const QRectF& rect, const QColor& color)
    {
        const ShadChartVertex topLeft = makeVertex(rect.topLeft(), color, 1.0);
        const ShadChartVertex topRight = makeVertex(rect.topRight(), color, 1.0);
        const ShadChartVertex bottomLeft = makeVertex(rect.bottomLeft(), color, 1.0);
        const ShadChartVertex bottomRight = makeVertex(rect.bottomRight(), color, 1.0);
        beginStrip(vertices, topLeft);
        vertices.append(topLeft);
        vertices.append(bottomLeft);
        vertices.append(topRight);
        vertices.append(bottomRight);
    }

    void appendConvexPolygon(QList<ShadChartVertex>& vertices, const QPolygonF& polygon, const QColor& color)
    {
        const qsizetype count = polygon.size();
        if (count < 3)
        {
            return;
        }
        // 顺序为 0、1、n-1、2、n-2，依此类推
        vertices.reserve(vertices.size() + count + 2);
        beginStrip(vertices, makeVertex(polygon.first(), color, 1.0));
        qsizetype front = 0;
        qsizetype back = count - 1;
        bool takeFront = true;
        while (front <= back)
        {
            if (takeFront)
            {
                vertices.append(makeVertex(polygon.at(front), color, 1.0));
                ++front;
            }
            else
            {
                vertices.append(makeVertex(polygon.at(back), color, 1.0));
                --back;
            }
            takeFront = !takeFront;
        }
    }

    void appendPolyline(QList<ShadChartVertex>& vertices, const QList<QPointF>& sourcePoints, qreal width, const QColor& color)
    {
        QList<QPointF> points;
        points.reserve(sourcePoints.size());
        for (const QPointF& point : sourcePoints)
        {
            // 去掉重合的相邻点（阶梯曲线在水平段两端会产生），否则无法计算法线
            if (points.isEmpty() || QLineF(points.last(), point).length() > 1e-6)
            {
                points.append(point);
            }
        }
        const qsizetype count = points.size();
        if (count < 2)
        {
            return;
        }
        QList<QPointF> normals;
        normals.reserve(count - 1);
        for (qsizetype i = 0; i < count - 1; ++i)
        {
            const QPointF delta = points.at(i + 1) - points.at(i);
            const qreal length = std::hypot(delta.x(), delta.y());
            normals.append(QPointF(-delta.y() / length, delta.x() / length));
        }
        const qreal halfWidth = width / 2.0;
        QList<QPointF> offsets;
        offsets.reserve(count);
        for (qsizetype i = 0; i < count; ++i)
        {
            if (i == 0)
            {
                offsets.append(normals.first() * halfWidth);
                continue;
            }
            if (i == count - 1)
            {
                offsets.append(normals.last() * halfWidth);
                continue;
            }
            QPointF miter = normals.at(i - 1) + normals.at(i);
            const qreal miterLength = std::hypot(miter.x(), miter.y());
            if (miterLength < 1e-6)
            {
                offsets.append(normals.at(i) * halfWidth);
                continue;
            }
            miter /= miterLength;
            const qreal cosine = miter.x() * normals.at(i).x() + miter.y() * normals.at(i).y();
            offsets.append(miter * (halfWidth / qMax(cosine, kMinMiterCos)));
        }
        // 每个点两侧各一个顶点，依次组成三角形带
        vertices.reserve(vertices.size() + 2 * count + 2);
        beginStrip(vertices, makeVertex(points.first() + offsets.first(), color, 1.0));
        for (qsizetype i = 0; i < count; ++i)
        {
            vertices.append(makeVertex(points.at(i) + offsets.at(i), color, 1.0));
            vertices.append(makeVertex(points.at(i) - offsets.at(i), color, 1.0));
        }
    }
} // namespace ShadChartHelper
