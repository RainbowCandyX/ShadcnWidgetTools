#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPointF>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadAreaChart.h"
#include "ShadBarChart.h"
#include "ShadCandlestickChart.h"
#include "ShadCategoryChart.h"
#include "ShadDataTable.h"
#include "ShadDef.h"
#include "ShadHeatmap.h"
#include "ShadLineChart.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadPagination.h"
#include "ShadScatterChart.h"
#include "ShadTable.h"
#include "ShadTreeView.h"

namespace
{
    // 限制嵌套层数，避免自引用的表无限递归
    constexpr int kMaxTreeDepth = 32;

    int optionalInt(lua_State* L, int index, int defaultValue)
    {
        qint64 value = 0;
        return ShadLuaStack::getInteger(L, index, value) ? int(value) : defaultValue;
    }

    QString optionalString(lua_State* L, int index)
    {
        QString value;
        ShadLuaStack::getString(L, index, value);
        return value;
    }

    // index 必须是正的栈位置
    bool readNumbers(lua_State* L, int index, QList<qreal>& values)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        values.clear();
        values.reserve(length);
        for (int position = 1; position <= length; ++position)
        {
            double number = 0.0;
            lua_rawgeti(L, index, position);
            const bool isNumber = ShadLuaStack::getNumber(L, -1, number);
            lua_pop(L, 1);
            if (!isNumber)
            {
                return false;
            }
            values.append(number);
        }
        return true;
    }

    bool readNumberLists(lua_State* L, int index, QList<QList<qreal>>& lists)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        lists.clear();
        for (int position = 1; position <= length; ++position)
        {
            QList<qreal> values;
            lua_rawgeti(L, index, position);
            const bool isList = readNumbers(L, lua_gettop(L), values);
            lua_pop(L, 1);
            if (!isList)
            {
                return false;
            }
            lists.append(values);
        }
        return true;
    }

    bool readPoints(lua_State* L, int index, QList<QPointF>& points)
    {
        QList<QList<qreal>> lists;
        if (!readNumberLists(L, index, lists))
        {
            return false;
        }
        points.clear();
        points.reserve(lists.size());
        for (const QList<qreal>& pair : std::as_const(lists))
        {
            if (pair.size() < 2)
            {
                return false;
            }
            points.append(QPointF(pair.at(0), pair.at(1)));
        }
        return true;
    }

    void pushNumbers(lua_State* L, const QList<qreal>& values)
    {
        lua_createtable(L, int(values.size()), 0);
        for (int position = 0; position < values.size(); ++position)
        {
            lua_pushnumber(L, values.at(position));
            lua_rawseti(L, -2, position + 1);
        }
    }

    // 系列颜色参数，类型不正确时返回 false
    bool readSeriesColor(lua_State* L, int index, ShadThemeType::ThemeColor& colorRole, QColor& color)
    {
        colorRole = ShadThemeType::Chart1;
        color = QColor();
        if (lua_isnoneornil(L, index))
        {
            return true;
        }
        if (lua_type(L, index) == LUA_TNUMBER)
        {
            colorRole = static_cast<ShadThemeType::ThemeColor>(optionalInt(L, index, ShadThemeType::Chart1));
            return true;
        }
        return ShadLuaStack::getColor(L, index, color);
    }

    double fieldNumber(lua_State* L, int index, const char* name)
    {
        double value = 0.0;
        lua_getfield(L, index, name);
        ShadLuaStack::getNumber(L, -1, value);
        lua_pop(L, 1);
        return value;
    }

    bool readCandle(lua_State* L, int index, ShadCandle& candle)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        lua_getfield(L, index, "label");
        ShadLuaStack::getString(L, -1, candle.label);
        lua_pop(L, 1);
        candle.open = fieldNumber(L, index, "open");
        candle.high = fieldNumber(L, index, "high");
        candle.low = fieldNumber(L, index, "low");
        candle.close = fieldNumber(L, index, "close");
        candle.volume = fieldNumber(L, index, "volume");
        return true;
    }

    bool readCandles(lua_State* L, int index, QList<ShadCandle>& candles)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        candles.clear();
        candles.reserve(length);
        for (int position = 1; position <= length; ++position)
        {
            ShadCandle candle;
            lua_rawgeti(L, index, position);
            const bool isCandle = readCandle(L, lua_gettop(L), candle);
            lua_pop(L, 1);
            if (!isCandle)
            {
                return false;
            }
            candles.append(candle);
        }
        return true;
    }

    // 单元格不可编辑，数值按原类型保存
    QStandardItem* createItem(const QVariant& value)
    {
        QStandardItem* item = new QStandardItem();
        item->setData(value, Qt::DisplayRole);
        item->setEditable(false);
        return item;
    }

    bool readRow(lua_State* L, int index, QList<QStandardItem*>& items)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        for (int position = 1; position <= length; ++position)
        {
            lua_rawgeti(L, index, position);
            items.append(createItem(ShadLuaStack::getVariant(L, -1)));
            lua_pop(L, 1);
        }
        return true;
    }

    // 把节点数组追加到 parent 下，节点为文字或表
    void appendNodes(lua_State* L, int index, QStandardItem* parent, int depth)
    {
        // 递归前先确保栈空间
        if (lua_type(L, index) != LUA_TTABLE || depth >= kMaxTreeDepth || !lua_checkstack(L, 4))
        {
            return;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        for (int position = 1; position <= length; ++position)
        {
            lua_rawgeti(L, index, position);
            const int nodeIndex = lua_gettop(L);
            if (lua_type(L, nodeIndex) != LUA_TTABLE)
            {
                parent->appendRow(createItem(ShadLuaStack::getVariant(L, nodeIndex)));
                lua_pop(L, 1);
                continue;
            }
            lua_getfield(L, nodeIndex, "text");
            QStandardItem* item = createItem(ShadLuaStack::getVariant(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, nodeIndex, "icon");
            if (lua_type(L, -1) == LUA_TNUMBER)
            {
                item->setData(optionalInt(L, -1, 0), ShadTreeView::IconRole);
            }
            lua_pop(L, 1);
            lua_getfield(L, nodeIndex, "checked");
            if (lua_type(L, -1) == LUA_TBOOLEAN)
            {
                item->setCheckable(true);
                item->setAutoTristate(true);
                item->setCheckState(lua_toboolean(L, -1) != 0 ? Qt::Checked : Qt::Unchecked);
            }
            lua_pop(L, 1);
            parent->appendRow(item);
            lua_getfield(L, nodeIndex, "children");
            appendNodes(L, lua_gettop(L), item, depth + 1);
            lua_pop(L, 2);
        }
    }

    // 没有父对象的模型交给视图管理，避免被回收后悬空
    void adoptModel(QAbstractItemModel* model, QObject* owner)
    {
        if (model != nullptr && model->parent() == nullptr)
        {
            model->setParent(owner);
        }
    }

    // 面积图、折线图与柱状图共用的系列操作
    void registerCategoryCharts()
    {
        ShadLuaClass<ShadCategoryChart> chart;
        chart.method("setCategories", &ShadCategoryChart::setCategories);
        chart.method("setTickLabels", &ShadCategoryChart::setTickLabels);
        // addSeries(label, values, color)
        chart.rawMethod("addSeries", [](lua_State* L, ShadCategoryChart* self) -> int
        {
            QList<qreal> values;
            ShadThemeType::ThemeColor colorRole = ShadThemeType::Chart1;
            QColor color;
            if (!readNumbers(L, 3, values))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            if (!readSeriesColor(L, 4, colorRole, color))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            lua_pushinteger(L, color.isValid() ? self->addSeries(optionalString(L, 2), values, color) : self->addSeries(optionalString(L, 2), values, colorRole));
            return 1;
        });
        chart.rawMethod("setSeriesValues", [](lua_State* L, ShadCategoryChart* self) -> int
        {
            QList<qreal> values;
            if (!readNumbers(L, 3, values))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->setSeriesValues(optionalInt(L, 2, 0), values);
            return 0;
        });
        chart.rawMethod("getSeriesValues", [](lua_State* L, ShadCategoryChart* self) -> int
        {
            pushNumbers(L, self->getSeriesValues(optionalInt(L, 2, 0)));
            return 1;
        });
        // appendData(categories, seriesValues, maxCount)
        chart.rawMethod("appendData", [](lua_State* L, ShadCategoryChart* self) -> int
        {
            QStringList categories;
            QList<QList<qreal>> seriesValues;
            if (!ShadLuaStack::getStringList(L, 2, categories))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (!readNumberLists(L, 3, seriesValues))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->appendData(categories, seriesValues, optionalInt(L, 4, 0));
            return 0;
        });
        chart.method("setSeriesLabel", &ShadCategoryChart::setSeriesLabel);
        chart.method("getSeriesLabel", &ShadCategoryChart::getSeriesLabel);
        chart.method("setSeriesIcon", &ShadCategoryChart::setSeriesIcon);
        chart.method("clearSeries", &ShadCategoryChart::clearSeries);
        chart.method("getSeriesCount", &ShadCategoryChart::getSeriesCount);
        chart.method("isZoomed", &ShadCategoryChart::isZoomed);
        chart.method("resetZoom", &ShadCategoryChart::resetZoom);

        ShadLuaClass<ShadLineChart>("LineChart").constructor(&ShadLuaBind::newWithParent<ShadLineChart>);
        ShadLuaClass<ShadAreaChart>("AreaChart").constructor(&ShadLuaBind::newWithParent<ShadAreaChart>);
        ShadLuaClass<ShadBarChart>("BarChart").constructor(&ShadLuaBind::newWithParent<ShadBarChart>);
    }

    // 散点图、热力图与 K 线图
    void registerOtherCharts()
    {
        // addSeries(label, points, color)
        ShadLuaClass<ShadScatterChart> scatter("ScatterChart");
        scatter.constructor(&ShadLuaBind::newWithParent<ShadScatterChart>);
        scatter.rawMethod("addSeries", [](lua_State* L, ShadScatterChart* self) -> int
        {
            QList<QPointF> points;
            ShadThemeType::ThemeColor colorRole = ShadThemeType::Chart1;
            QColor color;
            if (!readPoints(L, 3, points))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            if (!readSeriesColor(L, 4, colorRole, color))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            lua_pushinteger(L, color.isValid() ? self->addSeries(optionalString(L, 2), points, color) : self->addSeries(optionalString(L, 2), points, colorRole));
            return 1;
        });
        scatter.rawMethod("setSeriesPoints", [](lua_State* L, ShadScatterChart* self) -> int
        {
            QList<QPointF> points;
            if (!readPoints(L, 3, points))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->setSeriesPoints(optionalInt(L, 2, 0), points);
            return 0;
        });
        // appendPoints(index, points, maxCount)
        scatter.rawMethod("appendPoints", [](lua_State* L, ShadScatterChart* self) -> int
        {
            QList<QPointF> points;
            if (!readPoints(L, 3, points))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->appendPoints(optionalInt(L, 2, 0), points, optionalInt(L, 4, 0));
            return 0;
        });
        scatter.method("setSeriesLabel", &ShadScatterChart::setSeriesLabel);
        scatter.method("clearSeries", &ShadScatterChart::clearSeries);
        scatter.method("getSeriesCount", &ShadScatterChart::getSeriesCount);

        // setData(rowCount, columnCount, values)；appendRow(values, maxRows)
        ShadLuaClass<ShadHeatmap> heatmap("Heatmap");
        heatmap.constructor(&ShadLuaBind::newWithParent<ShadHeatmap>);
        heatmap.rawMethod("setData", [](lua_State* L, ShadHeatmap* self) -> int
        {
            QList<qreal> values;
            if (!readNumbers(L, 4, values))
            {
                return ShadLuaStack::argumentError(L, 3);
            }
            self->setData(optionalInt(L, 2, 0), optionalInt(L, 3, 0), values);
            return 0;
        });
        heatmap.rawMethod("appendRow", [](lua_State* L, ShadHeatmap* self) -> int
        {
            QList<qreal> values;
            if (!readNumbers(L, 2, values))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->appendRow(values, optionalInt(L, 3, 0));
            return 0;
        });
        heatmap.method("setRowLabels", &ShadHeatmap::setRowLabels);
        heatmap.method("setColumnLabels", &ShadHeatmap::setColumnLabels);
        heatmap.method("setValueRange", &ShadHeatmap::setValueRange);
        heatmap.method("resetValueRange", &ShadHeatmap::resetValueRange);
        heatmap.method("getRowCount", &ShadHeatmap::getRowCount);
        heatmap.method("getColumnCount", &ShadHeatmap::getColumnCount);

        ShadLuaClass<ShadCandlestickChart> candlestick("CandlestickChart");
        candlestick.constructor(&ShadLuaBind::newWithParent<ShadCandlestickChart>);
        candlestick.rawMethod("setCandles", [](lua_State* L, ShadCandlestickChart* self) -> int
        {
            QList<ShadCandle> candles;
            if (!readCandles(L, 2, candles))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setCandles(candles);
            return 0;
        });
        // appendCandles(candles, maxCount)
        candlestick.rawMethod("appendCandles", [](lua_State* L, ShadCandlestickChart* self) -> int
        {
            QList<ShadCandle> candles;
            if (!readCandles(L, 2, candles))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->appendCandles(candles, optionalInt(L, 3, 0));
            return 0;
        });
        candlestick.rawMethod("updateLastCandle", [](lua_State* L, ShadCandlestickChart* self) -> int
        {
            ShadCandle candle;
            if (!readCandle(L, 2, candle))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->updateLastCandle(candle);
            return 0;
        });
        candlestick.rawMethod("setMovingAverages", [](lua_State* L, ShadCandlestickChart* self) -> int
        {
            QList<qreal> values;
            if (!readNumbers(L, 2, values))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            QList<int> periods;
            for (const qreal value : std::as_const(values))
            {
                periods.append(qRound(value));
            }
            self->setMovingAverages(periods);
            return 0;
        });
        candlestick.method("clearCandles", &ShadCandlestickChart::clearCandles);
        candlestick.method("getCandleCount", &ShadCandlestickChart::getCandleCount);
    }

    // 数据模型 shad.Model，表格与树形视图共用
    void registerModel()
    {
        ShadLuaClass<QStandardItemModel> model("Model");
        model.constructor([](lua_State* L) -> QStandardItemModel* { return new QStandardItemModel(ShadLuaObject::get(L, 1)); });
        model.method("setHeaders", &QStandardItemModel::setHorizontalHeaderLabels);
        // addRow({值, ...})
        model.rawMethod("addRow", [](lua_State* L, QStandardItemModel* self) -> int
        {
            QList<QStandardItem*> items;
            if (!readRow(L, 2, items))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->appendRow(items);
            lua_pushinteger(L, self->rowCount() - 1);
            return 1;
        });
        // setRows({{值, ...}, ...})
        model.rawMethod("setRows", [](lua_State* L, QStandardItemModel* self) -> int
        {
            if (lua_type(L, 2) != LUA_TTABLE)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->removeRows(0, self->rowCount());
            const int length = ShadLuaStack::tableLength(L, 2);
            for (int position = 1; position <= length; ++position)
            {
                QList<QStandardItem*> items;
                lua_rawgeti(L, 2, position);
                const bool isRow = readRow(L, lua_gettop(L), items);
                lua_pop(L, 1);
                if (isRow)
                {
                    self->appendRow(items);
                }
            }
            return 0;
        });
        // setTree({节点, ...})
        model.rawMethod("setTree", [](lua_State* L, QStandardItemModel* self) -> int
        {
            if (lua_type(L, 2) != LUA_TTABLE)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->removeRows(0, self->rowCount());
            appendNodes(L, 2, self->invisibleRootItem(), 0);
            return 0;
        });
        model.rawMethod("getValue", [](lua_State* L, QStandardItemModel* self) -> int
        {
            ShadLuaStack::pushVariant(L, self->index(optionalInt(L, 2, 0), optionalInt(L, 3, 0)).data());
            return 1;
        });
        model.rawMethod("setValue", [](lua_State* L, QStandardItemModel* self) -> int
        {
            const QModelIndex index = self->index(optionalInt(L, 2, 0), optionalInt(L, 3, 0));
            if (!index.isValid())
            {
                return ShadLuaStack::error(L, QStringLiteral("行号或列号超出范围"));
            }
            self->setData(index, ShadLuaStack::getVariant(L, 4), Qt::DisplayRole);
            return 0;
        });
        model.method("removeRow", +[](QStandardItemModel* self, int row) { self->removeRow(row); });
        model.method("clearRows", +[](QStandardItemModel* self) { self->removeRows(0, self->rowCount()); });
        model.method("rowCount", +[](QStandardItemModel* self) { return self->rowCount(); });
        model.method("columnCount", +[](QStandardItemModel* self) { return self->columnCount(); });
    }

    // 表格、数据表格与树形视图
    void registerViews()
    {
        // 表格与树形视图共用的方法
        ShadLuaClass<QAbstractItemView> view;
        view.rawMethod("setModel", [](lua_State* L, QAbstractItemView* self) -> int
        {
            QAbstractItemModel* model = ShadLuaObject::get<QAbstractItemModel>(L, 2);
            if (model == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            adoptModel(model, self);
            self->setModel(model);
            return 0;
        });
        view.method("currentRow", +[](QAbstractItemView* self) { return self->currentIndex().row(); });
        view.method("currentText", +[](QAbstractItemView* self) { return self->currentIndex().data().toString(); });
        // currentPath() 返回从根节点到当前项的文字数组
        view.rawMethod("currentPath", [](lua_State* L, QAbstractItemView* self) -> int
        {
            QStringList path;
            for (QModelIndex index = self->currentIndex(); index.isValid(); index = index.parent())
            {
                path.prepend(index.data().toString());
            }
            ShadLuaStack::pushStringList(L, path);
            return 1;
        });

        ShadLuaClass<ShadTable> table("Table");
        table.constructor(&ShadLuaBind::newWithParent<ShadTable>);
        table.method("setColumnFixedWidth", &ShadTable::setColumnFixedWidth);
        table.method("setColumnSortable", &ShadTable::setColumnSortable);
        table.method("setFooterText", +[](ShadTable* self, int column, QString text) { self->setFooterText(column, text); });
        table.method("clearFooter", &ShadTable::clearFooter);

        ShadLuaClass<ShadTreeView>("TreeView").constructor(&ShadLuaBind::newWithParent<ShadTreeView>);

        ShadLuaClass<ShadDataTable> dataTable("DataTable");
        dataTable.constructor(&ShadLuaBind::newWithParent<ShadDataTable>);
        dataTable.rawMethod("setSourceModel", [](lua_State* L, ShadDataTable* self) -> int
        {
            QAbstractItemModel* model = ShadLuaObject::get<QAbstractItemModel>(L, 2);
            if (model == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            adoptModel(model, self);
            self->setSourceModel(model);
            return 0;
        });
        // getSelectedRows()
        dataTable.rawMethod("getSelectedRows", [](lua_State* L, ShadDataTable* self) -> int
        {
            const QList<int> rows = self->getSelectedRows();
            lua_createtable(L, int(rows.size()), 0);
            for (int position = 0; position < rows.size(); ++position)
            {
                lua_pushinteger(L, rows.at(position));
                lua_rawseti(L, -2, position + 1);
            }
            return 1;
        });
        dataTable.method("setFilterColumn", &ShadDataTable::setFilterColumn);
        dataTable.method("setFilterPlaceholderText", &ShadDataTable::setFilterPlaceholderText);
        dataTable.method("setColumnSortable", &ShadDataTable::setColumnSortable);
        dataTable.method("setColumnHideable", &ShadDataTable::setColumnHideable);
        dataTable.method("setColumnVisible", &ShadDataTable::setColumnVisible);
        dataTable.method("isColumnVisible", &ShadDataTable::isColumnVisible);
        dataTable.method("setColumnFixedWidth", &ShadDataTable::setColumnFixedWidth);
        dataTable.method("setSelectionEnabled", &ShadDataTable::setSelectionEnabled);
        dataTable.method("isSelectionEnabled", &ShadDataTable::isSelectionEnabled);
        dataTable.method("clearSelection", &ShadDataTable::clearSelection);
        dataTable.method("setPageSize", &ShadDataTable::setPageSize);
        dataTable.method("getPageSize", &ShadDataTable::getPageSize);
        dataTable.method("setPageIndex", &ShadDataTable::setPageIndex);
        dataTable.method("getPageIndex", &ShadDataTable::getPageIndex);
        dataTable.method("getPageCount", &ShadDataTable::getPageCount);
        dataTable.method("setEmptyText", &ShadDataTable::setEmptyText);
        dataTable.method("getTable", &ShadDataTable::getTable);
        dataTable.method("getPagination", &ShadDataTable::getPagination);
    }
} // namespace

namespace ShadLuaBindings
{
    void registerDataComponents()
    {
        registerCategoryCharts();
        registerOtherCharts();
        registerModel();
        registerViews();
    }
} // namespace ShadLuaBindings
