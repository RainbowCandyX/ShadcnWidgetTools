#include "ChartBarPage.h"

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
#include "ShadBarChart.h"
#include "ShadCard.h"
#include "ShadDef.h"

namespace
{
    // 缩放示例的天数
    constexpr int kDayCount = 91;

    QStringList monthCategories()
    {
        return {
            QCoreApplication::translate("ChartBarPage", "1月"), QCoreApplication::translate("ChartBarPage", "2月"),
            QCoreApplication::translate("ChartBarPage", "3月"), QCoreApplication::translate("ChartBarPage", "4月"),
            QCoreApplication::translate("ChartBarPage", "5月"), QCoreApplication::translate("ChartBarPage", "6月")
        };
    }

    // 1 月到 6 月的桌面端与移动端访问量；分类与系列名称随语言刷新，分类数量不变，缩放与数据不受影响
    ShadBarChart* createMonthChart(QWidget* parent, bool hasMobile)
    {
        ShadBarChart* chart = new ShadBarChart(parent);
        ExampleI18n::bind(chart, [chart]() { chart->setCategories(monthCategories()); });
        const int desktopIndex = chart->addSeries(QString(), {186, 305, 237, 73, 209, 214}, ShadThemeType::Chart1);
        const int mobileIndex = hasMobile ? chart->addSeries(QString(), {80, 200, 120, 190, 130, 140}, ShadThemeType::Chart2) : -1;
        ExampleI18n::bind(chart, [chart, desktopIndex, mobileIndex]()
        {
            chart->setSeriesLabel(desktopIndex, QCoreApplication::translate("ChartBarPage", "桌面端"));
            if (mobileIndex >= 0)
            {
                chart->setSeriesLabel(mobileIndex, QCoreApplication::translate("ChartBarPage", "移动端"));
            }
        });
        return chart;
    }
} // namespace

ChartBarPage::ChartBarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("柱状图"); }, [] { return tr("ShadBarChart：GPU 绘制的柱状图，GPU 不可用时改用 CPU。"); }, parent)
{
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, [] { return tr("setBarRadius(8)：柱子四个角为圆角，悬停只显示提示"); });
    ShadBarChart* defaultChart = createMonthChart(this, false);
    defaultChart->setBarRadius(8.0);
    addStretchItem(defaultCard, defaultChart);

    ShadCard* multipleCard = addSection([] { return tr("多系列"); }, [] { return tr("同一分类内各系列并排，间隔 4"); });
    ShadBarChart* multipleChart = createMonthChart(this, true);
    multipleChart->setTooltipIndicator(ShadChartType::Dashed);
    addStretchItem(multipleCard, multipleChart);

    ShadCard* stackedCard = addSection([] { return tr("堆叠"); }, []
    {
        return tr("setStackMode(Stacked)：只圆整根柱子的两端，setLegendVisible(true) 显示图例");
    });
    ShadBarChart* stackedChart = createMonthChart(this, true);
    stackedChart->setStackMode(ShadChartType::Stacked);
    stackedChart->setLegendVisible(true);
    addStretchItem(stackedCard, stackedChart);

    ShadCard* expandedCard = addSection([] { return tr("百分比堆叠"); }, []
    {
        return tr("setStackMode(Expanded)：每个分类按总和归一化为 0% 到 100%");
    });
    ShadBarChart* expandedChart = createMonthChart(this, true);
    const int otherIndex = expandedChart->addSeries(QString(), {45, 60, 52, 38, 70, 64}, ShadThemeType::Chart3);
    ExampleI18n::bind(expandedChart, [expandedChart, otherIndex]()
    {
        expandedChart->setSeriesLabel(otherIndex, tr("其他"));
    });
    expandedChart->setStackMode(ShadChartType::Expanded);
    expandedChart->setYAxisVisible(true);
    addStretchItem(expandedCard, expandedChart);

    ShadCard* squareCard = addSection([] { return tr("直角"); }, []
    {
        return tr("setBarRadius(0)：柱子为直角，setYAxisVisible(true) 显示 Y 轴刻度");
    });
    ShadBarChart* squareChart = createMonthChart(this, true);
    squareChart->setBarRadius(0.0);
    squareChart->setYAxisVisible(true);
    addStretchItem(squareCard, squareChart);

    ShadCard* zoomCard = addSection([] { return tr("缩放"); }, [] { return tr("setZoomEnabled(true)：滚轮缩放、拖动平移、双击恢复"); });
    ShadBarChart* zoomChart = new ShadBarChart(this);
    QList<qreal> desktop;
    const QDate firstDate(2024, 4, 1);
    for (int day = 0; day < kDayCount; ++day)
    {
        desktop.append(std::round(220.0 + 120.0 * std::sin(day * 0.37) + 80.0 * std::sin(day * 0.11 + 1.0) + (day * 37) % 50));
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
    ExampleI18n::bind(zoomChart, [zoomChart, zoomDesktopIndex]()
    {
        zoomChart->setSeriesLabel(zoomDesktopIndex, tr("桌面端"));
    });
    zoomChart->setBarRadius(2.0);
    zoomChart->setZoomEnabled(true);
    addStretchItem(zoomCard, zoomChart);

    ShadCard* stressCard = addSection([] { return tr("压力测试"); }, []
    {
        return tr("每帧追加 1000 条随机数据，只保留最近 50000 条；分类多于像素宽度时按像素列合并");
    });
    ShadBarChart* stressChart = new ShadBarChart(this);
    // 压力测试要求序号 0 为移动端、1 为桌面端
    const int stressMobileIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart2);
    const int stressDesktopIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart1);
    ExampleI18n::bind(stressChart, [stressChart, stressMobileIndex, stressDesktopIndex]()
    {
        stressChart->setSeriesLabel(stressMobileIndex, tr("移动端"));
        stressChart->setSeriesLabel(stressDesktopIndex, tr("桌面端"));
    });
    stressChart->setStackMode(ShadChartType::Stacked);
    stressChart->setZoomEnabled(true);
    addStretchItem(stressCard, ChartStressTest::createToolbar(this, stressChart));
    addStretchItem(stressCard, stressChart);
}

ChartBarPage::~ChartBarPage()
{
}
