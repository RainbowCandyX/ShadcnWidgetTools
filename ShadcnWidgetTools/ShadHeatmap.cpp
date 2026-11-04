#include "ShadHeatmap.h"

#include <QList>
#include <QStringList>
#include <QtGlobal>

#include "ShadHeatmapPrivate.h"

ShadHeatmap::ShadHeatmap(QWidget* parent)
    : ShadChart(*new ShadHeatmapPrivate(), parent)
{
}

ShadHeatmap::~ShadHeatmap()
{
}

ShadHeatmapPrivate* ShadHeatmap::heatmapPrivate() const
{
    return static_cast<ShadHeatmapPrivate*>(d_ptr.data());
}

void ShadHeatmap::setData(int rowCount, int columnCount, const QList<qreal>& values)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    const int rows = qMax(0, rowCount);
    const int columns = rows > 0 ? qMax(0, columnCount) : 0;
    d->_rows.clear();
    d->_rows.reserve(rows);
    for (int row = 0; row < rows; ++row)
    {
        QList<qreal> line(columns, 0.0);
        for (int column = 0; column < columns; ++column)
        {
            line[column] = values.value(static_cast<qsizetype>(row) * columns + column, 0.0);
        }
        d->_rows.append(line);
    }
    d->_columnCount = columns;
    d->_pendingRows = 0;
    d->_isFullDirty = true;
    d->_isImageDirty = true;
    d->requestUpdate();
}

QList<qreal> ShadHeatmap::getData() const
{
    const ShadHeatmapPrivate* d = heatmapPrivate();
    QList<qreal> values;
    values.reserve(d->_rows.size() * d->_columnCount);
    for (const QList<qreal>& row : d->_rows)
    {
        values.append(row);
    }
    return values;
}

QList<qreal> ShadHeatmap::getRow(int row) const
{
    return heatmapPrivate()->_rows.value(row);
}

int ShadHeatmap::getRowCount() const
{
    return static_cast<int>(heatmapPrivate()->_rows.size());
}

int ShadHeatmap::getColumnCount() const
{
    return heatmapPrivate()->_columnCount;
}

void ShadHeatmap::appendRow(const QList<qreal>& values, int maxRows)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (d->_rows.isEmpty())
    {
        d->_columnCount = static_cast<int>(values.size());
    }
    if (d->_columnCount == 0)
    {
        return;
    }
    QList<qreal> row = values;
    row.resize(d->_columnCount, 0.0);
    d->_rows.append(row);
    ++d->_pendingRows;
    const int boundedMaxRows = qMax(0, maxRows);
    // maxRows 即 GPU 图像的行数，变化后整张重写
    if (d->_ringMaxRows != boundedMaxRows)
    {
        d->_ringMaxRows = boundedMaxRows;
        d->_isFullDirty = true;
    }
    if (boundedMaxRows > 0 && d->_rows.size() > boundedMaxRows)
    {
        const qsizetype removed = d->_rows.size() - boundedMaxRows;
        d->_rows.remove(0, removed);
        d->_rowLabels.remove(0, qMin(removed, d->_rowLabels.size()));
    }
    d->_isImageDirty = true;
    d->requestUpdate();
}

void ShadHeatmap::setRowLabels(const QStringList& labels)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    d->_rowLabels = labels;
    d->requestUpdate();
}

QStringList ShadHeatmap::getRowLabels() const
{
    return heatmapPrivate()->_rowLabels;
}

void ShadHeatmap::setColumnLabels(const QStringList& labels)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    d->_columnLabels = labels;
    d->requestUpdate();
}

QStringList ShadHeatmap::getColumnLabels() const
{
    return heatmapPrivate()->_columnLabels;
}

void ShadHeatmap::setValueRange(qreal minimum, qreal maximum)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (minimum >= maximum)
    {
        qWarning("热力图数值范围无效：%g 到 %g，已忽略", minimum, maximum);
        return;
    }
    d->_hasValueRange = true;
    d->_rangeMin = minimum;
    d->_rangeMax = maximum;
    d->requestUpdate();
}

void ShadHeatmap::resetValueRange()
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (!d->_hasValueRange)
    {
        return;
    }
    d->_hasValueRange = false;
    d->requestUpdate();
}

bool ShadHeatmap::hasValueRange() const
{
    return heatmapPrivate()->_hasValueRange;
}

void ShadHeatmap::setColorRole(ShadThemeType::ThemeColor colorRole)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (d->_colorRole == colorRole)
    {
        return;
    }
    d->_colorRole = colorRole;
    d->requestUpdate();
}

ShadThemeType::ThemeColor ShadHeatmap::getColorRole() const
{
    return heatmapPrivate()->_colorRole;
}

void ShadHeatmap::setXAxisVisible(bool visible)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (d->_isXAxisVisible == visible)
    {
        return;
    }
    d->_isXAxisVisible = visible;
    d->requestUpdate();
}

bool ShadHeatmap::isXAxisVisible() const
{
    return heatmapPrivate()->_isXAxisVisible;
}

void ShadHeatmap::setYAxisVisible(bool visible)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (d->_isYAxisVisible == visible)
    {
        return;
    }
    d->_isYAxisVisible = visible;
    d->requestUpdate();
}

bool ShadHeatmap::isYAxisVisible() const
{
    return heatmapPrivate()->_isYAxisVisible;
}

void ShadHeatmap::setTooltipVisible(bool visible)
{
    ShadHeatmapPrivate* d = heatmapPrivate();
    if (d->_isTooltipVisible == visible)
    {
        return;
    }
    d->_isTooltipVisible = visible;
    d->requestUpdate();
}

bool ShadHeatmap::isTooltipVisible() const
{
    return heatmapPrivate()->_isTooltipVisible;
}
