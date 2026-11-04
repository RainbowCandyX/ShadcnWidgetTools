#include "ShadHeatmapPrivate.h"

#include <QCoreApplication>
#include <QFont>
#include <QImage>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>
#include <cstring>
#include <utility>

#include "ShadChart.h"
#include "ShadChartHelper.h"
#include "ShadChartOverlay.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 不限行数时纹理的最小行数
    constexpr int kMinGrowRows = 16;

    quint8 channelByte(qreal value)
    {
        return static_cast<quint8>(qBound(0, qRound(value * 255.0), 255));
    }

    QColor mixColor(const QColor& low, const QColor& high, qreal t)
    {
        return QColor::fromRgbF(static_cast<float>(low.redF() + (high.redF() - low.redF()) * t), static_cast<float>(low.greenF() + (high.greenF() - low.greenF()) * t), static_cast<float>(low.blueF() + (high.blueF() - low.blueF()) * t), static_cast<float>(low.alphaF() + (high.alphaF() - low.alphaF()) * t));
    }
} // namespace

ShadHeatmapPrivate::ShadHeatmapPrivate()
{
}

ShadHeatmapPrivate::~ShadHeatmapPrivate()
{
}

void ShadHeatmapPrivate::updateChart()
{
    _isUpdatePending = false;
    computeLayout();
    const QRect plot = physicalPlotRect();
    placePlotView(plot, !plot.isEmpty() && !_rows.isEmpty() && _columnCount > 0);
#if defined(SHADCN_CHART_GPU)
    if (_view != nullptr)
    {
        _view->setVertices(QList<ShadChartVertex>());
        syncImage();
    }
#endif
    if (_softwareView != nullptr)
    {
        // CPU 层每次整张着色
        _pendingRows = 0;
        const bool isColorChanged = _valueMin != _imageMin || _valueMax != _imageMax || _lowColor != _imageLow || _highColor != _imageHigh;
        if (_isImageDirty || isColorChanged)
        {
            const int rowCount = static_cast<int>(_rows.size());
            if (rowCount > 0 && _columnCount > 0)
            {
                _image = QImage(_columnCount, rowCount, QImage::Format_RGBA8888_Premultiplied);
                colorizeRows(_image, 0, 0, rowCount);
            }
            else
            {
                _image = QImage();
            }
            _imageMin = _valueMin;
            _imageMax = _valueMax;
            _imageLow = _lowColor;
            _imageHigh = _highColor;
            _isImageDirty = false;
        }
    }
    // 数据变化后按鼠标位置重新查找
    const bool hadHover = _hoverRow >= 0;
    if (hadHover && !cellAt(_hoverPosition, _hoverRow, _hoverColumn))
    {
        _hoverRow = -1;
        _hoverColumn = -1;
    }
    updateOverlay(plot, hadHover || _hoverRow >= 0);
}

void ShadHeatmapPrivate::paintPlot(QPainter* painter) const
{
    if (_image.isNull() || _plotRect.isEmpty())
    {
        return;
    }
    // 放大时不插值；RTL 时水平镜像
    painter->save();
    if (q_ptr->layoutDirection() == Qt::RightToLeft)
    {
        painter->translate(_plotRect.width(), 0.0);
        painter->scale(-1.0, 1.0);
    }
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawImage(QRectF(0.0, 0.0, _plotRect.width(), _plotRect.height()), _image);
    painter->restore();
}

void ShadHeatmapPrivate::paintOverlay(QPainter* painter)
{
    paintAxes(painter);
    if (_hoverRow < 0 || _hoverRow >= _rows.size() || _hoverColumn < 0 || _hoverColumn >= _columnCount)
    {
        return;
    }
    // 悬停的单元格画 1px 边框
    const QRectF rect = cellRect(_hoverRow, _hoverColumn);
    painter->save();
    painter->setPen(QPen(shadTheme->getColor(ShadThemeType::Foreground), 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
    painter->restore();
    const qreal value = _rows.at(_hoverRow).at(_hoverColumn);
    const QList<ShadChartHelper::TooltipRow> rows{{columnLabel(_hoverColumn), ShadChartHelper::formatValue(value), cellColor(value), ShadIconType::None}};
    ShadChartHelper::paintTooltip(painter, q_ptr, rect.center().x(), _hoverPosition.y(), rowLabel(_hoverRow), rows, ShadChartType::Dot);
}

void ShadHeatmapPrivate::updateHover(const QPointF& position)
{
    int row = -1;
    int column = -1;
    if (!cellAt(position, row, column))
    {
        clearHover();
        return;
    }
    _hoverRow = row;
    _hoverColumn = column;
    _hoverPosition = position;
    _overlay->update();
}

void ShadHeatmapPrivate::clearHover()
{
    if (_hoverRow < 0)
    {
        return;
    }
    _hoverRow = -1;
    _hoverColumn = -1;
    _overlay->update();
}

void ShadHeatmapPrivate::plotViewCreated()
{
    _isFullDirty = true;
    _isImageDirty = true;
}

void ShadHeatmapPrivate::computeLayout()
{
    const ShadChart* q = q_ptr;
    qreal minValue = _rangeMin;
    qreal maxValue = _rangeMax;
    if (!_hasValueRange)
    {
        bool hasValue = false;
        for (const QList<qreal>& row : std::as_const(_rows))
        {
            for (const qreal value : row)
            {
                if (std::isnan(value))
                {
                    continue;
                }
                minValue = hasValue ? qMin(minValue, value) : value;
                maxValue = hasValue ? qMax(maxValue, value) : value;
                hasValue = true;
            }
        }
        if (!hasValue)
        {
            minValue = 0.0;
            maxValue = 1.0;
        }
    }
    if (maxValue - minValue < 1e-12)
    {
        maxValue = minValue + 1.0;
    }
    _valueMin = minValue;
    _valueMax = maxValue;

    // 查找表按字节顺序 R、G、B、A 存放预乘后的颜色
    _lowColor = surfaceColor();
    _highColor = shadTheme->getColor(_colorRole);
    _colorTable.resize(kColorLevels);
    for (int level = 0; level < kColorLevels; ++level)
    {
        const QColor color = mixColor(_lowColor, _highColor, static_cast<qreal>(level) / (kColorLevels - 1));
        const qreal alpha = color.alphaF();
        const quint8 bytes[4] = {channelByte(color.redF() * alpha), channelByte(color.greenF() * alpha), channelByte(color.blueF() * alpha), channelByte(alpha)};
        quint32 packed = 0;
        std::memcpy(&packed, bytes, sizeof(packed));
        _colorTable[level] = packed;
    }

    // 绘图区：去掉外边距与标签占用的区域
    const QFont font = textFont();
    int left = ShadChartHelper::kHorizontalMargin;
    const int right = q->width() - ShadChartHelper::kHorizontalMargin;
    const int top = ShadChartHelper::kVerticalMargin;
    int bottom = q->height() - ShadChartHelper::kVerticalMargin;
    if (hasColumnLabels())
    {
        bottom -= ShadChartHelper::kXAxisHeight;
    }
    if (hasRowLabels())
    {
        int labelWidth = 0;
        for (const QString& label : std::as_const(_rowLabels))
        {
            labelWidth = qMax(labelWidth, ShadStyleHelper::textWidth(font, label));
        }
        left += labelWidth + ShadChartHelper::kTickMargin;
    }
    _plotRect = QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));

    _rowLabelIndices.clear();
    _columnLabelIndices.clear();
    const int rowCount = static_cast<int>(_rows.size());
    if (hasRowLabels() && _plotRect.height() > 0)
    {
        QList<qreal> centers;
        QList<qreal> heights;
        const qreal rowHeight = static_cast<qreal>(_plotRect.height()) / rowCount;
        for (int row = 0; row < rowCount; ++row)
        {
            centers.append(_plotRect.top() + (row + 0.5) * rowHeight);
            heights.append(ShadChartHelper::kTickTextHeight);
        }
        _rowLabelIndices = ShadChartHelper::spacedLabels(centers, heights, ShadChartHelper::kMinYTickGap);
    }
    if (hasColumnLabels() && _plotRect.width() > 0)
    {
        QList<qreal> centers;
        QList<qreal> widths;
        const qreal columnWidth = static_cast<qreal>(_plotRect.width()) / _columnCount;
        for (int column = 0; column < _columnCount; ++column)
        {
            centers.append(_plotRect.left() + (column + 0.5) * columnWidth);
            widths.append(ShadStyleHelper::textWidth(font, _columnLabels.at(column)));
        }
        _columnLabelIndices = ShadChartHelper::spacedLabels(centers, widths, ShadChartHelper::kMinTickGap);
    }
}

#if defined(SHADCN_CHART_GPU)
void ShadHeatmapPrivate::syncImage()
{
    const int rowCount = static_cast<int>(_rows.size());
    const int columns = _columnCount;
    const bool isMirrored = q_ptr->layoutDirection() == Qt::RightToLeft;
    bool isFull = _isFullDirty || columns != _gpuColumns || _valueMin != _gpuMin || _valueMax != _gpuMax || _lowColor != _gpuLow || _highColor != _gpuHigh;
    // _rows[firstRow, firstRow + count) 写到图像的 slot 行起
    const auto writeRows = [this, columns](int firstRow, int count, int slot)
    {
        QImage rows(columns, count, QImage::Format_RGBA8888_Premultiplied);
        colorizeRows(rows, 0, firstRow, count);
        _view->writeImageRows(slot, rows);
    };
    if (rowCount == 0 || columns == 0)
    {
        _view->resetImage(0, 0);
        _gpuHead = 0;
        _gpuCount = 0;
        _gpuCapacity = 0;
        isFull = false;
    }
    else if (!isFull && _pendingRows > 0)
    {
        const int appended = _pendingRows;
        const bool isRing = _ringMaxRows > 0 && _gpuCapacity == _ringMaxRows;
        if (isRing && appended < _gpuCapacity && appended <= rowCount)
        {
            // 接在最后一行之后，超出容量从开头继续，覆盖的正是被移除的行
            const int writeStart = (_gpuHead + _gpuCount) % _gpuCapacity;
            const int firstLength = qMin(appended, _gpuCapacity - writeStart);
            writeRows(rowCount - appended, firstLength, writeStart);
            if (appended > firstLength)
            {
                writeRows(rowCount - appended + firstLength, appended - firstLength, 0);
            }
            const int removed = _gpuCount + appended - rowCount;
            _gpuHead = (_gpuHead + removed) % _gpuCapacity;
            _gpuCount = rowCount;
        }
        else if (!isRing && _gpuHead == 0 && _gpuCount + appended == rowCount && rowCount <= _gpuCapacity)
        {
            // 不限行数且容量足够时接在末尾
            writeRows(_gpuCount, appended, _gpuCount);
            _gpuCount = rowCount;
        }
        else
        {
            isFull = true;
        }
    }
    if (isFull)
    {
        // 环形存放时容量为 maxRows；不限行数时预留一半余量，但不超过纹理尺寸上限
        int capacity = _ringMaxRows > 0 ? qMax(_ringMaxRows, rowCount) : qMax(kMinGrowRows, rowCount + rowCount / 2);
        const int maxSize = _view->maxTextureSize();
        if (_ringMaxRows == 0 && maxSize > 0)
        {
            capacity = qMin(capacity, qMax(rowCount, maxSize));
        }
        _view->resetImage(columns, capacity);
        writeRows(0, rowCount, 0);
        _gpuHead = 0;
        _gpuCount = rowCount;
        _gpuCapacity = capacity;
    }
    _view->setImageRange(_gpuHead, _gpuCount, isMirrored);
    _pendingRows = 0;
    _isFullDirty = false;
    _gpuColumns = columns;
    _gpuMin = _valueMin;
    _gpuMax = _valueMax;
    _gpuLow = _lowColor;
    _gpuHigh = _highColor;
}
#endif

void ShadHeatmapPrivate::colorizeRows(QImage& target, int targetRow, int firstRow, int count) const
{
    const qreal scale = (kColorLevels - 1) / (_valueMax - _valueMin);
    const quint32* table = _colorTable.constData();
    for (int row = 0; row < count; ++row)
    {
        // 每行按 4 字节对齐
        quint32* line = reinterpret_cast<quint32*>(target.scanLine(targetRow + row));
        const QList<qreal>& values = _rows.at(firstRow + row);
        for (int column = 0; column < _columnCount; ++column)
        {
            const qreal value = values.at(column);
            const int level = std::isnan(value) ? 0 : qBound(0, static_cast<int>(std::lround((value - _valueMin) * scale)), kColorLevels - 1);
            line[column] = table[level];
        }
    }
}

QColor ShadHeatmapPrivate::cellColor(qreal value) const
{
    const qreal t = std::isnan(value) ? 0.0 : qBound(0.0, (value - _valueMin) / (_valueMax - _valueMin), 1.0);
    return mixColor(_lowColor, _highColor, t);
}

bool ShadHeatmapPrivate::cellAt(const QPointF& position, int& row, int& column) const
{
    const QRect plot = physicalPlotRect();
    const int rowCount = static_cast<int>(_rows.size());
    if (!_isTooltipVisible || rowCount == 0 || _columnCount == 0 || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return false;
    }
    // 镜像是对合变换，同一函数可把实际 X 转回逻辑 X
    const qreal logicalX = physicalX(position.x());
    column = qBound(0, static_cast<int>(std::floor((logicalX - _plotRect.left()) / _plotRect.width() * _columnCount)), _columnCount - 1);
    row = qBound(0, static_cast<int>(std::floor((position.y() - _plotRect.top()) / _plotRect.height() * rowCount)), rowCount - 1);
    return true;
}

QRectF ShadHeatmapPrivate::cellRect(int row, int column) const
{
    const qreal columnWidth = static_cast<qreal>(_plotRect.width()) / _columnCount;
    const qreal rowHeight = static_cast<qreal>(_plotRect.height()) / _rows.size();
    const qreal x0 = physicalX(_plotRect.left() + column * columnWidth);
    const qreal x1 = physicalX(_plotRect.left() + (column + 1) * columnWidth);
    return QRectF(qMin(x0, x1), _plotRect.top() + row * rowHeight, qAbs(x1 - x0), rowHeight);
}

void ShadHeatmapPrivate::paintAxes(QPainter* painter) const
{
    const ShadChart* q = q_ptr;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRectF bounds(q->rect());
    const QFont font = textFont();
    painter->setFont(font);
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    const int rowCount = static_cast<int>(_rows.size());
    if (!_rowLabelIndices.isEmpty() && rowCount > 0)
    {
        const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter);
        const qreal rowHeight = static_cast<qreal>(_plotRect.height()) / rowCount;
        for (const int row : std::as_const(_rowLabelIndices))
        {
            const qreal center = _plotRect.top() + (row + 0.5) * rowHeight;
            const QRectF logicalRect(0.0, center - ShadChartHelper::kTickTextHeight / 2.0, _plotRect.left() - ShadChartHelper::kTickMargin, ShadChartHelper::kTickTextHeight);
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, logicalRect), alignment | Qt::TextDontClip, _rowLabels.value(row));
        }
    }
    if (!_columnLabelIndices.isEmpty() && _columnCount > 0)
    {
        const qreal textTop = _plotRect.top() + _plotRect.height() + ShadChartHelper::kTickMargin;
        const qreal columnWidth = static_cast<qreal>(_plotRect.width()) / _columnCount;
        for (const int column : std::as_const(_columnLabelIndices))
        {
            const QString text = _columnLabels.value(column);
            const qreal textWidth = ShadStyleHelper::textWidth(font, text);
            const qreal x = physicalX(_plotRect.left() + (column + 0.5) * columnWidth);
            painter->drawText(QRectF(x - textWidth / 2.0, textTop, textWidth, ShadChartHelper::kTickTextHeight), Qt::AlignCenter | Qt::TextDontClip, text);
        }
    }
}

QString ShadHeatmapPrivate::rowLabel(int row) const
{
    // 标签数量与行数不同时显示序号
    if (_rowLabels.size() == _rows.size())
    {
        return _rowLabels.at(row);
    }
    return QCoreApplication::translate("ShadHeatmap", "第 %1 行").arg(row + 1);
}

QString ShadHeatmapPrivate::columnLabel(int column) const
{
    if (_columnLabels.size() == _columnCount)
    {
        return _columnLabels.at(column);
    }
    return QCoreApplication::translate("ShadHeatmap", "第 %1 列").arg(column + 1);
}

bool ShadHeatmapPrivate::hasRowLabels() const
{
    return _isYAxisVisible && !_rows.isEmpty() && _rowLabels.size() == _rows.size();
}

bool ShadHeatmapPrivate::hasColumnLabels() const
{
    return _isXAxisVisible && _columnCount > 0 && _columnLabels.size() == _columnCount;
}

QFont ShadHeatmapPrivate::textFont() const
{
    return q_ptr->font();
}

qreal ShadHeatmapPrivate::physicalX(qreal logicalX) const
{
    return q_ptr->layoutDirection() == Qt::RightToLeft ? q_ptr->width() - logicalX : logicalX;
}

QRect ShadHeatmapPrivate::physicalPlotRect() const
{
    return ShadStyleHelper::visualRect(q_ptr->layoutDirection(), q_ptr->rect(), _plotRect);
}
