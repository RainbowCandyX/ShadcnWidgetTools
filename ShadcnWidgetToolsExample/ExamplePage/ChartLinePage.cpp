#include "ChartLinePage.h"

#include <QCoreApplication>
#include <QDate>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtGlobal>

#include <cmath>

#include "ChartStressTest.h"
#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLineChart.h"

namespace
{
    // 缩放示例的天数
    constexpr int kDayCount = 91;

    QStringList monthCategories()
    {
        return {
            QCoreApplication::translate("ChartLinePage", "1月"), QCoreApplication::translate("ChartLinePage", "2月"),
            QCoreApplication::translate("ChartLinePage", "3月"), QCoreApplication::translate("ChartLinePage", "4月"),
            QCoreApplication::translate("ChartLinePage", "5月"), QCoreApplication::translate("ChartLinePage", "6月")
        };
    }

    // 1 月到 6 月的桌面端与移动端访问量；分类与系列名称随语言刷新，分类数量不变，缩放与数据不受影响
    ShadLineChart* createMonthChart(QWidget* parent, bool hasMobile)
    {
        ShadLineChart* chart = new ShadLineChart(parent);
        ExampleI18n::bind(chart, [chart]() { chart->setCategories(monthCategories()); });
        const int desktopIndex = chart->addSeries(QString(), {186, 305, 237, 73, 209, 214}, ShadThemeType::Chart1);
        const int mobileIndex = hasMobile ? chart->addSeries(QString(), {80, 200, 120, 190, 130, 140}, ShadThemeType::Chart2) : -1;
        ExampleI18n::bind(chart, [chart, desktopIndex, mobileIndex]()
        {
            chart->setSeriesLabel(desktopIndex, QCoreApplication::translate("ChartLinePage", "桌面端"));
            if (mobileIndex >= 0)
            {
                chart->setSeriesLabel(mobileIndex, QCoreApplication::translate("ChartLinePage", "移动端"));
            }
        });
        return chart;
    }
} // namespace

ChartLinePage::ChartLinePage(QWidget* parent)
    : ExampleBasePage([] { return tr("折线图"); }, [] { return tr("ShadLineChart：GPU 绘制的折线图，GPU 不可用时改用 CPU。"); }, parent)
{
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, [] { return tr("自然曲线，线宽 2，悬停显示光标线与提示"); });
    addStretchItem(defaultCard, createMonthChart(this, false));

    ShadCard* linearCard = addSection([] { return tr("线性"); }, [] { return tr("setCurveType(Linear)：相邻数据点直线相连"); });
    ShadLineChart* linearChart = createMonthChart(this, false);
    linearChart->setCurveType(ShadChartType::Linear);
    addStretchItem(linearCard, linearChart);

    ShadCard* stepCard = addSection([] { return tr("阶梯"); }, [] { return tr("setCurveType(Step)：在相邻两点的中点处跳变"); });
    ShadLineChart* stepChart = createMonthChart(this, false);
    stepChart->setCurveType(ShadChartType::Step);
    addStretchItem(stepCard, stepChart);

    ShadCard* multipleCard = addSection([] { return tr("多系列"); }, []
    {
        return tr("每个系列一条线，setLegendVisible(true) 显示图例");
    });
    ShadLineChart* multipleChart = createMonthChart(this, true);
    multipleChart->setLegendVisible(true);
    multipleChart->setTooltipIndicator(ShadChartType::Line);
    addStretchItem(multipleCard, multipleChart);

    ShadCard* dotsCard = addSection([] { return tr("数据点"); }, []
    {
        return tr("setDotsVisible(true)：每个分类上显示数据点，由 GPU 按实例绘制");
    });
    ShadLineChart* dotsChart = createMonthChart(this, true);
    dotsChart->setDotsVisible(true);
    addStretchItem(dotsCard, dotsChart);

    ShadCard* axesCard = addSection([] { return tr("坐标轴"); }, []
    {
        return tr("setYAxisVisible(true)：显示 Y 轴刻度，setLineWidth(1) 改细线");
    });
    ShadLineChart* axesChart = createMonthChart(this, true);
    axesChart->setYAxisVisible(true);
    axesChart->setLineWidth(1.0);
    addStretchItem(axesCard, axesChart);

    ShadCard* zoomCard = addSection([] { return tr("缩放"); }, [] { return tr("setZoomEnabled(true)：滚轮缩放、拖动平移、双击恢复"); });
    ShadLineChart* zoomChart = new ShadLineChart(this);
    QList<qreal> desktop;
    QList<qreal> mobile;
    const QDate firstDate(2024, 4, 1);
    for (int day = 0; day < kDayCount; ++day)
    {
        desktop.append(std::round(220.0 + 120.0 * std::sin(day * 0.37) + 80.0 * std::sin(day * 0.11 + 1.0) + (day * 37) % 50));
        mobile.append(std::round(180.0 + 90.0 * std::sin(day * 0.29 + 2.0) + 60.0 * std::sin(day * 0.07) + (day * 53) % 40));
    }
    // 日期分类随语言刷新，分类数量不变，缩放范围保留
    ExampleI18n::bind(zoomChart, [zoomChart, firstDate]()
    {
        QStringList categories;
        QStringList tickLabels;
        for (int day = 0; day < kDayCount; ++day)
        {
            const QDate date = firstDate.addDays(day);
            categories.append(tr("%1年%2月%3日").arg(date.year()).arg(date.month()).arg(date.day()));
            tickLabels.append(tr("%1月%2日").arg(date.month()).arg(date.day()));
        }
        zoomChart->setCategories(categories);
        zoomChart->setTickLabels(tickLabels);
    });
    const int zoomDesktopIndex = zoomChart->addSeries(QString(), desktop, ShadThemeType::Chart1);
    const int zoomMobileIndex = zoomChart->addSeries(QString(), mobile, ShadThemeType::Chart2);
    ExampleI18n::bind(zoomChart, [zoomChart, zoomDesktopIndex, zoomMobileIndex]()
    {
        zoomChart->setSeriesLabel(zoomDesktopIndex, tr("桌面端"));
        zoomChart->setSeriesLabel(zoomMobileIndex, tr("移动端"));
    });
    zoomChart->setCurveType(ShadChartType::Linear);
    zoomChart->setZoomEnabled(true);
    addStretchItem(zoomCard, zoomChart);

    ShadCard* stressCard = addSection([] { return tr("压力测试"); }, []
    {
        return tr("每帧追加 1000 条随机数据，只保留最近 50000 条；数据过密时按像素列抽稀");
    });
    ShadLineChart* stressChart = new ShadLineChart(this);
    // 压力测试要求序号 0 为移动端、1 为桌面端
    const int stressMobileIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart2);
    const int stressDesktopIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart1);
    ExampleI18n::bind(stressChart, [stressChart, stressMobileIndex, stressDesktopIndex]()
    {
        stressChart->setSeriesLabel(stressMobileIndex, tr("移动端"));
        stressChart->setSeriesLabel(stressDesktopIndex, tr("桌面端"));
    });
    stressChart->setCurveType(ShadChartType::Linear);
    stressChart->setZoomEnabled(true);
    addStretchItem(stressCard, ChartStressTest::createToolbar(this, stressChart));
    addStretchItem(stressCard, stressChart);
}

ChartLinePage::~ChartLinePage()
{
}
