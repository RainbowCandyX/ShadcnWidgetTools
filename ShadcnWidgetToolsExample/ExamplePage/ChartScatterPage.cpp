#include "ChartScatterPage.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QRandomGenerator>
#include <QScreen>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include <cmath>
#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadScatterChart.h"
#include "ShadSwitch.h"

namespace
{
    // 固定种子，每次启动数据相同
    constexpr quint32 kSampleSeed = 20240401;
    constexpr int kClusterPointCount = 120;
    constexpr int kStressBatchCount = 2000;
    constexpr int kStressMaxCount = 200000;
    // 刷新频率每秒统计一次
    constexpr qint64 kStressRateWindowMs = 1000;
    constexpr int kStressBarSpacing = 16;
    constexpr qreal kPi = 3.14159265358979323846;

    // 正态分布随机数（Box-Muller）
    qreal gaussian(QRandomGenerator& random)
    {
        const qreal u = qMax(1e-12, random.generateDouble());
        const qreal v = random.generateDouble();
        return std::sqrt(-2.0 * std::log(u)) * std::cos(2.0 * kPi * v);
    }

    QList<QPointF> clusterPoints(QRandomGenerator& random, const QPointF& center, const QPointF& spread)
    {
        QList<QPointF> points;
        points.reserve(kClusterPointCount);
        for (int i = 0; i < kClusterPointCount; ++i)
        {
            points.append(QPointF(center.x() + gaussian(random) * spread.x(), center.y() + gaussian(random) * spread.y()));
        }
        return points;
    }

    // 身高（cm）与体重（kg）两组样本
    ShadScatterChart* createSampleChart(QWidget* parent)
    {
        QRandomGenerator random(kSampleSeed);
        ShadScatterChart* chart = new ShadScatterChart(parent);
        // 先 A 后 B 依次生成，保证随机样本与之前一致
        const int groupAIndex = chart->addSeries(QString(), clusterPoints(random, QPointF(165.0, 58.0), QPointF(6.0, 6.0)), ShadThemeType::Chart1);
        const int groupBIndex = chart->addSeries(QString(), clusterPoints(random, QPointF(178.0, 74.0), QPointF(7.0, 8.0)), ShadThemeType::Chart2);
        // 系列名称随语言刷新，数据点不重新上传
        ExampleI18n::bind(chart, [chart, groupAIndex, groupBIndex]()
        {
            chart->setSeriesLabel(groupAIndex, QCoreApplication::translate("ChartScatterPage", "A 组"));
            chart->setSeriesLabel(groupBIndex, QCoreApplication::translate("ChartScatterPage", "B 组"));
        });
        return chart;
    }

    QString renderApiText(ShadChartType::RenderApi api)
    {
        switch (api)
        {
            case ShadChartType::Auto:
                return QCoreApplication::translate("ChartScatterPage", "自动");
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

    // 压力测试的状态，数据只保存在图表中
    struct StressData
    {
        qreal phase{0.0};
        qsizetype count{0};
        bool isRunning{false};
        int frameCount{0};
        qreal framesPerSecond{0.0};
        QElapsedTimer rateTimer;
    };

    QString stressStatusText(const StressData& data, const ShadScatterChart* chart)
    {
        const QScreen* screen = chart->screen();
        const qreal refreshRate = screen != nullptr ? screen->refreshRate() : 0.0;
        return QCoreApplication::translate("ChartScatterPage", "%1 绘制，点数 %2 / %3，实际刷新 %4 帧/秒，屏幕刷新率 %5 Hz").arg(renderApiText(chart->getRenderApi())).arg(data.count).arg(2 * kStressMaxCount).arg(data.framesPerSecond, 0, 'f', 1).arg(refreshRate, 0, 'f', 0);
    }

    // X 均匀分布，Y 为随相位移动的正弦加噪声
    void appendStressPoints(StressData& data, ShadScatterChart* chart)
    {
        QRandomGenerator* random = QRandomGenerator::global();
        for (int series = 0; series < 2; ++series)
        {
            QList<QPointF> points;
            points.reserve(kStressBatchCount);
            const qreal offset = series == 0 ? 0.0 : kPi;
            for (int i = 0; i < kStressBatchCount; ++i)
            {
                const qreal x = random->generateDouble() * 100.0;
                const qreal y = 50.0 + 25.0 * std::sin(x / 12.0 + data.phase + offset) + (random->generateDouble() - 0.5) * 12.0;
                points.append(QPointF(x, y));
            }
            chart->appendPoints(series, points, kStressMaxCount);
        }
        data.phase += 0.05;
        data.count = qMin<qsizetype>(data.count + 2 * kStressBatchCount, 2 * kStressMaxCount);
    }
} // namespace

ChartScatterPage::ChartScatterPage(QWidget* parent)
    : ExampleBasePage([] { return tr("散点图"); }, [] { return tr("ShadScatterChart：GPU 按实例绘制的散点图，适合几十万个点，GPU 不可用时改用 CPU。"); }, parent)
{
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, [] { return tr("X、Y 都是数值轴，范围按数据自动取整齐刻度；悬停显示最近的点"); });
    addStretchItem(defaultCard, createSampleChart(this));

    ShadCard* legendCard = addSection([] { return tr("图例"); }, [] { return tr("setLegendVisible(true)：图表下方显示系列名称"); });
    ShadScatterChart* legendChart = createSampleChart(this);
    legendChart->setLegendVisible(true);
    addStretchItem(legendCard, legendChart);

    ShadCard* radiusCard = addSection([] { return tr("点半径"); }, []
    {
        return tr("setPointRadius(5)：放大圆点，setGridVisible(false) 隐藏网格线");
    });
    ShadScatterChart* radiusChart = createSampleChart(this);
    radiusChart->setPointRadius(5.0);
    radiusChart->setGridVisible(false);
    addStretchItem(radiusCard, radiusChart);

    ShadCard* stressCard = addSection([] { return tr("实时追加与压力测试"); }, []
    {
        return tr("appendPoints：每帧每个系列追加 2000 个点，各保留最近 200000 个；GPU 绘制时只上传新增的点");
    });
    QWidget* stressBar = new QWidget(this);
    QHBoxLayout* stressLayout = new QHBoxLayout(stressBar);
    stressLayout->setContentsMargins(0, 0, 0, 0);
    stressLayout->setSpacing(kStressBarSpacing);
    ShadSwitch* stressSwitch = new ShadSwitch(stressBar);
    ExampleI18n::bind(stressSwitch, [stressSwitch]() { stressSwitch->setText(tr("压力测试")); });
    ShadLabel* stressStatus = new ShadLabel(stressBar);
    stressStatus->setColorRole(ShadThemeType::MutedForeground);
    stressLayout->addWidget(stressSwitch);
    stressLayout->addWidget(stressStatus);
    stressLayout->addStretch(1);
    ShadScatterChart* stressChart = new ShadScatterChart(this);
    const int stressAIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart1);
    const int stressBIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart2);
    ExampleI18n::bind(stressChart, [stressChart, stressAIndex, stressBIndex]()
    {
        stressChart->setSeriesLabel(stressAIndex, tr("A 组"));
        stressChart->setSeriesLabel(stressBIndex, tr("B 组"));
    });
    stressChart->setPointRadius(1.5);
    std::shared_ptr<StressData> stress = std::make_shared<StressData>();
    // 按当前点数与帧率生成状态文字
    ExampleI18n::bind(stressStatus, [stress, stressChart, stressStatus]()
    {
        stressStatus->setText(stressStatusText(*stress, stressChart));
    });
    // 上一帧提交后才推送下一批，帧率上限即屏幕刷新率
    connect(stressChart, &ShadScatterChart::frameRendered, stressStatus, [stress, stressChart, stressStatus]()
    {
        if (!stress->isRunning)
        {
            return;
        }
        ++stress->frameCount;
        const qint64 elapsed = stress->rateTimer.elapsed();
        // 状态文字每秒更新一次，避免每帧重新布局
        if (elapsed >= kStressRateWindowMs)
        {
            stress->framesPerSecond = stress->frameCount * 1000.0 / elapsed;
            stress->frameCount = 0;
            stress->rateTimer.restart();
            stressStatus->setText(stressStatusText(*stress, stressChart));
        }
        appendStressPoints(*stress, stressChart);
    });
    connect(stressSwitch, &ShadSwitch::toggled, stressChart, [stress, stressChart](bool checked)
    {
        stress->isRunning = checked;
        if (!checked)
        {
            return;
        }
        stress->frameCount = 0;
        stress->rateTimer.start();
        appendStressPoints(*stress, stressChart);
    });
    addStretchItem(stressCard, stressBar);
    addStretchItem(stressCard, stressChart);
}

ChartScatterPage::~ChartScatterPage()
{
}
