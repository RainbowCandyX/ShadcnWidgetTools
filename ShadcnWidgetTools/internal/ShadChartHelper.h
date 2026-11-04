#ifndef SHADCHARTHELPER_H
#define SHADCHARTHELPER_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"

class QPainter;
class QWidget;

// 12 字节顶点：坐标与预乘透明度的 RGBA；三角形带中为逻辑像素，点管线中为相对原点的数据坐标
struct ShadChartVertex
{
    float x{0.0f};
    float y{0.0f};
    quint8 r{0};
    quint8 g{0};
    quint8 b{0};
    quint8 a{0};
};

namespace ShadChartHelper
{
    // 外边距：左右 12、上下 5（对应 margin={{ left: 12, right: 12 }} 与 Recharts 默认上下边距）
    constexpr int kHorizontalMargin = 12;
    constexpr int kVerticalMargin = 5;
    // X 轴区域高度 30，刻度文字与绘图区间距 8（tickMargin）
    constexpr int kXAxisHeight = 30;
    constexpr int kTickMargin = 8;
    constexpr int kTickTextHeight = 16;
    // X 轴刻度文字之间的最小间距（minTickGap）
    constexpr int kMinTickGap = 32;
    constexpr int kMinYTickGap = 4;
    // 图例：pt-3，行高 16，项间距 gap-4，色块 8x8 圆角 2，色块与文字 gap-1.5，图标 12
    constexpr int kLegendTopPadding = 12;
    constexpr int kLegendLineHeight = 16;
    constexpr int kLegendItemGap = 16;
    constexpr int kLegendSwatchSize = 8;
    constexpr int kLegendIconSize = 12;
    constexpr int kLegendInnerGap = 6;
    // 网格线宽度 1、悬停数据点半径 4
    constexpr qreal kGridLineWidth = 1.0;
    constexpr qreal kActiveDotRadius = 4.0;
    // 提示卡片：与光标偏移 10，min-w-[8rem]，px-2.5 py-1.5，行间距 gap-1.5，标题行高 16、数据行高 12（leading-none）
    constexpr int kTooltipOffset = 10;
    constexpr int kTooltipMinWidth = 128;
    constexpr int kTooltipPaddingX = 10;
    constexpr int kTooltipPaddingY = 6;
    constexpr int kTooltipRowGap = 6;
    constexpr int kTooltipLabelHeight = 16;
    constexpr int kTooltipRowHeight = 12;
    // 颜色标记与文字间距 gap-2，名称与数值的最小间距
    constexpr int kTooltipIndicatorGap = 8;
    constexpr int kTooltipValueGap = 8;
    // 默认尺寸：高度 250（h-[250px]）
    constexpr int kDefaultWidth = 400;
    constexpr int kDefaultHeight = 250;
    constexpr int kMinimumSize = 120;

    // icon 不为 None 时代替色块
    struct LegendItem
    {
        QString label;
        QColor color;
        ShadIconType::IconName icon{ShadIconType::None};
    };

    // icon 不为 None 时代替颜色标记；color 无效且没有图标时不显示颜色标记
    struct TooltipRow
    {
        QString name;
        QString value;
        QColor color;
        ShadIconType::IconName icon{ShadIconType::None};
    };

    // count 个覆盖 [minValue, maxValue] 的整齐刻度，与 Recharts 相同
    QList<qreal> niceTicks(qreal minValue, qreal maxValue, int count);
    // 保留 12 位有效数字，消除 0.1 + 0.2 之类的浮点误差
    qreal roundTick(qreal value);
    // 对应 toLocaleString：千分位分隔，小数位取最短表示
    QString formatValue(qreal value);
    // 等宽字体（font-mono）
    QFont monospaceFont(const QFont& baseFont);
    // 按顺序保留间隔不小于 minGap 的文字（centers 递增），返回序号
    QList<int> spacedLabels(const QList<qreal>& centers, const QList<qreal>& widths, qreal minGap);

    // 位于组件底部居中
    void paintLegend(QPainter* painter, const QWidget* widget, const QList<LegendItem>& items);
    // 超出组件时翻到另一侧
    void paintTooltip(QPainter* painter, const QWidget* widget, qreal anchorX, qreal mouseY, const QString& title, const QList<TooltipRow>& rows, ShadChartType::TooltipIndicator indicator);

    // 颜色预乘透明度
    ShadChartVertex makeVertex(const QPointF& point, const QColor& color, qreal alpha);
    // 所有图形画在同一条三角形带里：开始新的一段前重复上一段的最后一个顶点与本段的第一个顶点，中间产生的面积为 0 的三角形不会显示
    void beginStrip(QList<ShadChartVertex>& vertices, const ShadChartVertex& first);
    void appendRect(QList<ShadChartVertex>& vertices, const QRectF& rect, const QColor& color);
    // 凸多边形，从两端交替取点
    void appendConvexPolygon(QList<ShadChartVertex>& vertices, const QPolygonF& polygon, const QColor& color);
    // 折线扩展为三角形带：每个点沿两侧线段法线的平均方向偏移（斜接），首尾点沿所在线段的法线偏移
    void appendPolyline(QList<ShadChartVertex>& vertices, const QList<QPointF>& sourcePoints, qreal width, const QColor& color);
} // namespace ShadChartHelper

#endif // SHADCHARTHELPER_H
