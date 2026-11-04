#ifndef SHADHEATMAP_H
#define SHADHEATMAP_H

#include <QList>
#include <QStringList>
#include <QtGlobal>

#include "ShadChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 热力图：颜色从表面颜色（最小值）过渡到 colorRole（最大值），第一行在最上方
// appendRow 可用于瀑布图，GPU 绘制时只上传新增的行；实时数据建议固定数值范围，避免范围变化时整张重新着色
class ShadHeatmapPrivate;

class SHADCN_EXPORT ShadHeatmap : public ShadChart
{
    Q_OBJECT
    Q_PROPERTY(QStringList rowLabels READ getRowLabels WRITE setRowLabels)
    Q_PROPERTY(QStringList columnLabels READ getColumnLabels WRITE setColumnLabels)
    Q_PROPERTY(ShadThemeType::ThemeColor colorRole READ getColorRole WRITE setColorRole)
    Q_PROPERTY(bool xAxisVisible READ isXAxisVisible WRITE setXAxisVisible)
    Q_PROPERTY(bool yAxisVisible READ isYAxisVisible WRITE setYAxisVisible)
    Q_PROPERTY(bool tooltipVisible READ isTooltipVisible WRITE setTooltipVisible)

    public:
        explicit ShadHeatmap(QWidget* parent = nullptr);
        ~ShadHeatmap() override;

        // values 按行排列，缺少的按 0 处理
        void setData(int rowCount, int columnCount, const QList<qreal>& values);
        QList<qreal> getData() const;
        QList<qreal> getRow(int row) const;
        int getRowCount() const;
        int getColumnCount() const;
        // 按列数补 0 或截断，没有数据时以 values 的长度作为列数；maxRows 大于 0 时只保留最近 maxRows 行
        // 行标签不随行追加，数量与行数不同时不显示
        void appendRow(const QList<qreal>& values, int maxRows = 0);

        // 数量与行数、列数相同时才显示
        void setRowLabels(const QStringList& labels);
        QStringList getRowLabels() const;
        void setColumnLabels(const QStringList& labels);
        QStringList getColumnLabels() const;

        // 默认按数据自动计算；minimum 不小于 maximum 时忽略
        void setValueRange(qreal minimum, qreal maximum);
        void resetValueRange();
        bool hasValueRange() const;

        // 最大值的颜色，默认 chart-1
        void setColorRole(ShadThemeType::ThemeColor colorRole);
        ShadThemeType::ThemeColor getColorRole() const;

        // 列标签，默认显示
        void setXAxisVisible(bool visible);
        bool isXAxisVisible() const;
        // 行标签，默认显示
        void setYAxisVisible(bool visible);
        bool isYAxisVisible() const;
        // 悬停提示，默认开启
        void setTooltipVisible(bool visible);
        bool isTooltipVisible() const;

    private:
        Q_DISABLE_COPY(ShadHeatmap)
        ShadHeatmapPrivate* heatmapPrivate() const;
};

#endif // SHADHEATMAP_H
