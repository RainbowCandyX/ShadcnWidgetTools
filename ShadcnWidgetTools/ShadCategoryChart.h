#ifndef SHADCATEGORYCHART_H
#define SHADCATEGORYCHART_H

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadChart.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QWidget;

// 分类图表基类，面积图、折线图与柱状图共用，不能直接创建；RTL 下 X 方向镜像，Y 轴在右侧
class ShadCategoryChartPrivate;

class SHADCN_EXPORT ShadCategoryChart : public ShadChart
{
    Q_OBJECT
    Q_PROPERTY(QStringList categories READ getCategories WRITE setCategories)
    Q_PROPERTY(QStringList tickLabels READ getTickLabels WRITE setTickLabels)
    Q_PROPERTY(ShadChartType::TooltipIndicator tooltipIndicator READ getTooltipIndicator WRITE setTooltipIndicator)
    Q_PROPERTY(bool gridVisible READ isGridVisible WRITE setGridVisible)
    Q_PROPERTY(bool xAxisVisible READ isXAxisVisible WRITE setXAxisVisible)
    Q_PROPERTY(bool yAxisVisible READ isYAxisVisible WRITE setYAxisVisible)
    Q_PROPERTY(bool legendVisible READ isLegendVisible WRITE setLegendVisible)
    Q_PROPERTY(bool tooltipVisible READ isTooltipVisible WRITE setTooltipVisible)
    Q_PROPERTY(int yTickCount READ getYTickCount WRITE setYTickCount)
    Q_PROPERTY(bool zoomEnabled READ isZoomEnabled WRITE setZoomEnabled)

    public:
        ~ShadCategoryChart() override;

        // X 轴分类，同时作为提示卡片的标题；系列的数值按分类顺序对应，缺少的数值按 0 处理
        void setCategories(const QStringList& categories);
        QStringList getCategories() const;
        // X 轴刻度文字，例如分类为完整日期、刻度只显示月日；为空或数量与分类不同时显示分类文字
        void setTickLabels(const QStringList& labels);
        QStringList getTickLabels() const;

        // 添加系列，返回系列序号；颜色为主题色（默认 chart-1，随明暗切换）或固定颜色
        int addSeries(const QString& label, const QList<qreal>& values, ShadThemeType::ThemeColor colorRole = ShadThemeType::Chart1);
        int addSeries(const QString& label, const QList<qreal>& values, const QColor& color);
        void setSeriesValues(int index, const QList<qreal>& values);
        QList<qreal> getSeriesValues(int index) const;
        // 系列名称（图例与提示卡片中显示），例如切换语言时修改；不影响数值、缩放与悬停
        void setSeriesLabel(int index, const QString& label);
        QString getSeriesLabel(int index) const;
        // 系列图标：设置后图例与提示卡片用图标代替颜色标记（对应 chartConfig 的 icon）
        void setSeriesIcon(int index, ShadIconType::IconName icon);
        void clearSeries();
        int getSeriesCount() const;

        // 追加数据，用于实时采集：在末尾追加分类与各系列的数值，seriesValues 按系列序号对应，缺少的数值按 0 处理
        // maxCount 大于 0 时只保留最近 maxCount 个分类，较早的分类、数值与刻度文字从开头移除；刻度文字不随分类追加，数量与分类不同时显示分类文字
        // 只修改开头与末尾，不复制整份数据，适合高频追加；放大后查看较早的数据时，可见范围随移除的分类前移，保持显示同一批数据
        void appendData(const QStringList& categories, const QList<QList<qreal>>& seriesValues, int maxCount = 0);

        // 提示卡片的颜色标记，默认 Dot
        void setTooltipIndicator(ShadChartType::TooltipIndicator indicator);
        ShadChartType::TooltipIndicator getTooltipIndicator() const;

        // 水平网格线，默认显示
        void setGridVisible(bool visible);
        bool isGridVisible() const;
        // X 轴刻度文字，默认显示；刻度之间至少留 32px，放不下时从末尾开始间隔显示
        void setXAxisVisible(bool visible);
        bool isXAxisVisible() const;
        // Y 轴刻度文字，默认隐藏；百分比堆叠时显示为百分数
        void setYAxisVisible(bool visible);
        bool isYAxisVisible() const;
        // 图例，默认隐藏；位于图表下方居中
        void setLegendVisible(bool visible);
        bool isLegendVisible() const;
        // 悬停提示，默认开启
        void setTooltipVisible(bool visible);
        bool isTooltipVisible() const;
        // Y 轴刻度数量（同时决定网格线数量），默认 5，最少 2
        void setYTickCount(int count);
        int getYTickCount() const;

        // 滚轮缩放与拖动平移，默认关闭；关闭时恢复显示全部分类
        // 开启后在绘图区内滚轮以鼠标位置为中心缩放 X 轴（最多放大到可见 10 个分类间隔），放大后按住左键拖动平移，双击恢复
        // 未放大时向下滚动交给父组件（例如滚动页面）；Y 轴范围按可见范围内的数据计算
        // 放大且可见范围贴住最后一个分类时，追加数据后继续显示最新的数据
        void setZoomEnabled(bool enabled);
        bool isZoomEnabled() const;
        bool isZoomed() const;
        void resetZoom();

    protected:
        ShadCategoryChart(ShadCategoryChartPrivate& dd, QWidget* parent);
        ShadCategoryChartPrivate* categoryPrivate() const;

    private:
        Q_DISABLE_COPY(ShadCategoryChart)
};

#endif // SHADCATEGORYCHART_H
