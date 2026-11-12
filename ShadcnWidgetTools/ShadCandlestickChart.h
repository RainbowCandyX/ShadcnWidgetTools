#ifndef SHADCANDLESTICKCHART_H
#define SHADCANDLESTICKCHART_H

#include <QList>
#include <QString>
#include <QtGlobal>

#include "ShadChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 一根 K 线，label 用作 X 轴刻度与提示卡片标题
struct ShadCandle
{
    QString label;
    qreal open{0.0};
    qreal high{0.0};
    qreal low{0.0};
    qreal close{0.0};
    qreal volume{0.0};
};

// K 线图：收盘不低于开盘为上涨，价格轴在右侧（RTL 在左侧），可选均线与成交量副图
// K 线多于绘图区像素宽度时按像素列合并
class ShadCandlestickChartPrivate;

class SHADCN_EXPORT ShadCandlestickChart : public ShadChart
{
    Q_OBJECT
    Q_PROPERTY(ShadChartType::CandleColorScheme colorScheme READ getColorScheme WRITE setColorScheme)
    Q_PROPERTY(QList<int> movingAverages READ getMovingAverages WRITE setMovingAverages)
    Q_PROPERTY(bool volumeVisible READ isVolumeVisible WRITE setVolumeVisible)
    Q_PROPERTY(int pricePrecision READ getPricePrecision WRITE setPricePrecision)
    Q_PROPERTY(bool gridVisible READ isGridVisible WRITE setGridVisible)
    Q_PROPERTY(bool xAxisVisible READ isXAxisVisible WRITE setXAxisVisible)
    Q_PROPERTY(bool yAxisVisible READ isYAxisVisible WRITE setYAxisVisible)
    Q_PROPERTY(bool legendVisible READ isLegendVisible WRITE setLegendVisible)
    Q_PROPERTY(bool tooltipVisible READ isTooltipVisible WRITE setTooltipVisible)
    Q_PROPERTY(int yTickCount READ getYTickCount WRITE setYTickCount)
    Q_PROPERTY(bool zoomEnabled READ isZoomEnabled WRITE setZoomEnabled)

    public:
        explicit ShadCandlestickChart(QWidget* parent = nullptr);
        ~ShadCandlestickChart() override;

        void setCandles(const QList<ShadCandle>& candles);
        QList<ShadCandle> getCandles() const;
        int getCandleCount() const;
        // maxCount 大于 0 时只保留最近 maxCount 根
        void appendCandles(const QList<ShadCandle>& candles, int maxCount = 0);
        // 替换最后一根，没有 K 线时追加
        void updateLastCandle(const ShadCandle& candle);
        void clearCandles();

        // 默认红涨绿跌
        void setColorScheme(ShadChartType::CandleColorScheme scheme);
        ShadChartType::CandleColorScheme getColorScheme() const;

        // 均线周期，例如 {5, 10, 20}，按收盘价计算；小于 1 的周期忽略
        void setMovingAverages(const QList<int>& periods);
        QList<int> getMovingAverages() const;
        // 默认隐藏
        void setVolumeVisible(bool visible);
        bool isVolumeVisible() const;
        // 提示卡片中价格的小数位数，默认 2，范围 0 到 8
        void setPricePrecision(int decimals);
        int getPricePrecision() const;

        // 网格、X 轴、价格轴与提示默认显示，均线图例默认隐藏
        void setGridVisible(bool visible);
        bool isGridVisible() const;
        void setXAxisVisible(bool visible);
        bool isXAxisVisible() const;
        void setYAxisVisible(bool visible);
        bool isYAxisVisible() const;
        void setLegendVisible(bool visible);
        bool isLegendVisible() const;
        void setTooltipVisible(bool visible);
        bool isTooltipVisible() const;
        // 默认 5，最少 2
        void setYTickCount(int count);
        int getYTickCount() const;

        // 滚轮缩放、拖动平移、双击恢复，默认关闭；最多放大到可见 10 根
        void setZoomEnabled(bool enabled);
        bool isZoomEnabled() const;
        bool isZoomed() const;
        void resetZoom();

    private:
        Q_DISABLE_COPY(ShadCandlestickChart)
        ShadCandlestickChartPrivate* candlestickPrivate() const;
};

#endif // SHADCANDLESTICKCHART_H
