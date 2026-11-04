#ifndef SHADHEATMAPPRIVATE_H
#define SHADHEATMAPPRIVATE_H

#include <QColor>
#include <QFont>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadChartPrivate.h"
#include "ShadDef.h"

class QPainter;

// 热力图私有数据；GPU 图像按行环形存放，颜色或范围变化时整张重新着色
class ShadHeatmapPrivate : public ShadChartPrivate
{
    public:
        ShadHeatmapPrivate();
        ~ShadHeatmapPrivate() override;

        static constexpr int kColorLevels = 256;

        void updateChart() override;
        void paintPlot(QPainter* painter) const override;
        void paintOverlay(QPainter* painter) override;
        void updateHover(const QPointF& position) override;
        void clearHover() override;

        // 每行的长度都是 _columnCount
        QList<QList<qreal>> _rows;
        int _columnCount{0};
        QStringList _rowLabels;
        QStringList _columnLabels;
        bool _hasValueRange{false};
        qreal _rangeMin{0.0};
        qreal _rangeMax{1.0};
        ShadThemeType::ThemeColor _colorRole{ShadThemeType::Chart1};
        bool _isXAxisVisible{true};
        bool _isYAxisVisible{true};
        bool _isTooltipVisible{true};
        // GPU 图像的同步状态
        int _pendingRows{0};
        int _ringMaxRows{0};
        bool _isFullDirty{true};
        // CPU 图像需要重新着色
        bool _isImageDirty{true};

    protected:
        void plotViewCreated() override;

    private:
        // 计算数值范围、颜色表、行列标签与绘图区
        void computeLayout();
#if defined(SHADCN_CHART_GPU)
        void syncImage();
#endif
        // _rows[firstRow, firstRow + count) 着色后写入 target 的 targetRow 行起
        void colorizeRows(QImage& target, int targetRow, int firstRow, int count) const;
        QColor cellColor(qreal value) const;
        // 返回是否在绘图区内
        bool cellAt(const QPointF& position, int& row, int& column) const;
        // 单元格的实际位置（RTL 已镜像）
        QRectF cellRect(int row, int column) const;
        void paintAxes(QPainter* painter) const;
        QString rowLabel(int row) const;
        QString columnLabel(int column) const;
        bool hasRowLabels() const;
        bool hasColumnLabels() const;
        QFont textFont() const;
        qreal physicalX(qreal logicalX) const;
        QRect physicalPlotRect() const;

        // 以下为 computeLayout 的结果，查找表为预乘 RGBA
        qreal _valueMin{0.0};
        qreal _valueMax{1.0};
        QColor _lowColor;
        QColor _highColor;
        QList<quint32> _colorTable;
        // 绘图区的逻辑位置（LTR）
        QRect _plotRect;
        // 显示的行列标签序号
        QList<int> _rowLabelIndices;
        QList<int> _columnLabelIndices;
        // GPU 图像的起点、行数、容量、列数与着色参数
        int _gpuHead{0};
        int _gpuCount{0};
        int _gpuCapacity{0};
        int _gpuColumns{0};
        qreal _gpuMin{0.0};
        qreal _gpuMax{0.0};
        QColor _gpuLow;
        QColor _gpuHigh;
        // CPU 图像与着色参数
        QImage _image;
        qreal _imageMin{0.0};
        qreal _imageMax{0.0};
        QColor _imageLow;
        QColor _imageHigh;
        // 悬停的单元格与鼠标位置，未悬停时为 -1
        int _hoverRow{-1};
        int _hoverColumn{-1};
        QPointF _hoverPosition;
};

#endif // SHADHEATMAPPRIVATE_H
