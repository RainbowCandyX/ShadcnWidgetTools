#include "ChartHeatmapPage.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QRandomGenerator>
#include <QScreen>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtGlobal>

#include <cmath>
#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadHeatmap.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kDayCount = 7;
    constexpr int kHourCount = 24;
    constexpr int kWaterfallColumns = 128;
    constexpr int kWaterfallMaxRows = 120;
    constexpr int kWaterfallHeight = 320;
    // 刷新频率每秒统计一次
    constexpr qint64 kRateWindowMs = 1000;
    constexpr int kBarSpacing = 16;

    // 每天每小时的访问量，下午最高、周末较低
    ShadHeatmap* createWeekChart(QWidget* parent)
    {
        ShadHeatmap* chart = new ShadHeatmap(parent);
        QList<qreal> values;
        for (int day = 0; day < kDayCount; ++day)
        {
            const qreal dayScale = day < 5 ? 1.0 : 0.6;
            for (int hour = 0; hour < kHourCount; ++hour)
            {
                const qreal peak = std::exp(-(hour - 14.0) * (hour - 14.0) / 30.0);
                values.append(std::round(10.0 + 80.0 * peak * dayScale + (day * 7 + hour * 13) % 10));
            }
        }
        chart->setData(kDayCount, kHourCount, values);
        // 只替换行列标签，数据与显示状态不变
        ExampleI18n::bind(chart, [chart]()
        {
            chart->setRowLabels({QCoreApplication::translate("ChartHeatmapPage", "周一"), QCoreApplication::translate("ChartHeatmapPage", "周二"), QCoreApplication::translate("ChartHeatmapPage", "周三"), QCoreApplication::translate("ChartHeatmapPage", "周四"), QCoreApplication::translate("ChartHeatmapPage", "周五"), QCoreApplication::translate("ChartHeatmapPage", "周六"), QCoreApplication::translate("ChartHeatmapPage", "周日")});
            QStringList hours;
            for (int hour = 0; hour < kHourCount; ++hour)
            {
                hours.append(QCoreApplication::translate("ChartHeatmapPage", "%1时").arg(hour));
            }
            chart->setColumnLabels(hours);
        });
        return chart;
    }

    QString renderApiText(ShadChartType::RenderApi api)
    {
        switch (api)
        {
            case ShadChartType::Auto:
                return QCoreApplication::translate("ChartHeatmapPage", "自动");
            case ShadChartType::Software:
                return QStringLiteral("CPU");
            case ShadChartType::OpenGL:
                return QStringLiteral("OpenGL");
            case ShadChartType::Direct3D11:
                return QStringLiteral("Direct3D 11");
            case ShadChartType::Direct3D12:
                return QStringLiteral("Direct3D 12");
            case ShadChartType::Vulkan:
                return QStringLiteral("Vulkan");
            case ShadChartType::Metal:
                return QStringLiteral("Metal");
        }
        return QString();
    }

    // 瀑布图的状态，数据只保存在图表中
    struct WaterfallData
    {
        qreal time{0.0};
        bool isRunning{false};
        int frameCount{0};
        qreal framesPerSecond{0.0};
        QElapsedTimer rateTimer;
    };

    QString waterfallStatusText(const WaterfallData& data, const ShadHeatmap* chart)
    {
        const QScreen* screen = chart->screen();
        const qreal refreshRate = screen != nullptr ? screen->refreshRate() : 0.0;
        return QCoreApplication::translate("ChartHeatmapPage", "%1 绘制，行数 %2 / %3，实际刷新 %4 帧/秒，屏幕刷新率 %5 Hz").arg(renderApiText(chart->getRenderApi())).arg(chart->getRowCount()).arg(kWaterfallMaxRows).arg(data.framesPerSecond, 0, 'f', 1).arg(refreshRate, 0, 'f', 0);
    }

    // 一行频谱：两个移动的高斯峰加噪声
    void appendWaterfallRow(WaterfallData& data, ShadHeatmap* chart)
    {
        QRandomGenerator* random = QRandomGenerator::global();
        const qreal firstPeak = kWaterfallColumns * (0.5 + 0.3 * std::sin(data.time));
        const qreal secondPeak = kWaterfallColumns * (0.5 + 0.35 * std::cos(data.time * 0.7));
        QList<qreal> row;
        row.reserve(kWaterfallColumns);
        for (int column = 0; column < kWaterfallColumns; ++column)
        {
            const qreal first = 0.8 * std::exp(-(column - firstPeak) * (column - firstPeak) / 40.0);
            const qreal second = 0.5 * std::exp(-(column - secondPeak) * (column - secondPeak) / 10.0);
            row.append(qBound(0.0, 0.05 + first + second + random->generateDouble() * 0.15, 1.0));
        }
        chart->appendRow(row, kWaterfallMaxRows);
        data.time += 0.04;
    }
} // namespace

ChartHeatmapPage::ChartHeatmapPage(QWidget* parent)
    : ExampleBasePage([] { return tr("热力图"); }, [] { return tr("ShadHeatmap：GPU 以纹理绘制的热力图，可用作瀑布图，GPU 不可用时改用 CPU。"); }, parent)
{
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, []
    {
        return tr("颜色从表面颜色过渡到 chart-1，范围按数据自动计算；行列标签放不下时间隔显示");
    });
    addStretchItem(defaultCard, createWeekChart(this));

    ShadCard* rangeCard = addSection([] { return tr("固定范围与颜色"); }, []
    {
        return tr("setValueRange(0, 100) 固定范围，setColorRole(Chart2) 更换颜色");
    });
    ShadHeatmap* rangeChart = createWeekChart(this);
    rangeChart->setValueRange(0.0, 100.0);
    rangeChart->setColorRole(ShadThemeType::Chart2);
    addStretchItem(rangeCard, rangeChart);

    ShadCard* waterfallCard = addSection([] { return tr("瀑布图"); }, []
    {
        return tr("appendRow：每帧追加一行 128 个频点，只保留最近 120 行；GPU 绘制时只上传新增的行");
    });
    QWidget* waterfallBar = new QWidget(this);
    QHBoxLayout* waterfallLayout = new QHBoxLayout(waterfallBar);
    waterfallLayout->setContentsMargins(0, 0, 0, 0);
    waterfallLayout->setSpacing(kBarSpacing);
    ShadSwitch* waterfallSwitch = new ShadSwitch(waterfallBar);
    ExampleI18n::bind(waterfallSwitch, [waterfallSwitch]() { waterfallSwitch->setText(tr("开始滚动")); });
    ShadLabel* waterfallStatus = new ShadLabel(waterfallBar);
    waterfallStatus->setColorRole(ShadThemeType::MutedForeground);
    waterfallLayout->addWidget(waterfallSwitch);
    waterfallLayout->addWidget(waterfallStatus);
    waterfallLayout->addStretch(1);
    ShadHeatmap* waterfallChart = new ShadHeatmap(this);
    waterfallChart->setFixedHeight(kWaterfallHeight);
    // 固定范围，追加时不必整张重新着色
    waterfallChart->setValueRange(0.0, 1.0);
    waterfallChart->setColorRole(ShadThemeType::Chart3);
    std::shared_ptr<WaterfallData> waterfall = std::make_shared<WaterfallData>();
    // 状态文字按当前帧率与行数重新生成
    ExampleI18n::bind(waterfallStatus, [waterfall, waterfallChart, waterfallStatus]()
    {
        waterfallStatus->setText(waterfallStatusText(*waterfall, waterfallChart));
    });
    // 上一帧提交后才追加下一行，帧率上限即屏幕刷新率
    connect(waterfallChart, &ShadHeatmap::frameRendered, waterfallStatus, [waterfall, waterfallChart, waterfallStatus]()
    {
        if (!waterfall->isRunning)
        {
            return;
        }
        ++waterfall->frameCount;
        const qint64 elapsed = waterfall->rateTimer.elapsed();
        // 状态文字每秒更新一次，避免每帧重新布局
        if (elapsed >= kRateWindowMs)
        {
            waterfall->framesPerSecond = waterfall->frameCount * 1000.0 / elapsed;
            waterfall->frameCount = 0;
            waterfall->rateTimer.restart();
            waterfallStatus->setText(waterfallStatusText(*waterfall, waterfallChart));
        }
        appendWaterfallRow(*waterfall, waterfallChart);
    });
    connect(waterfallSwitch, &ShadSwitch::toggled, waterfallChart, [waterfall, waterfallChart](bool checked)
    {
        waterfall->isRunning = checked;
        if (!checked)
        {
            return;
        }
        waterfall->frameCount = 0;
        waterfall->rateTimer.start();
        appendWaterfallRow(*waterfall, waterfallChart);
    });
    addStretchItem(waterfallCard, waterfallBar);
    addStretchItem(waterfallCard, waterfallChart);
}

ChartHeatmapPage::~ChartHeatmapPage()
{
}
