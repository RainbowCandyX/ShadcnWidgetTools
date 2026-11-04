#include "ChartStressTest.h"

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
#include "ShadCategoryChart.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kBatchCount = 1000;
    constexpr int kMaxCount = 50000;
    // 刷新频率每秒统计一次
    constexpr qint64 kRateWindowMs = 1000;
    constexpr int kBarSpacing = 16;

    // 数据只保存在图表中，这里只记录随机游走的状态与帧率统计
    struct StressData
    {
        qint64 nextIndex{1};
        qreal desktopValue{250.0};
        qreal mobileValue{150.0};
        qsizetype count{0};
        bool isRunning{false};
        int frameCount{0};
        qreal framesPerSecond{0.0};
        QElapsedTimer rateTimer;
    };

    QString renderApiText(ShadChartType::RenderApi api)
    {
        switch (api)
        {
            case ShadChartType::Auto:
                return QCoreApplication::translate("ChartStressTest", "自动");
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

    QString statusText(const StressData& data, const ShadCategoryChart* chart)
    {
        const QScreen* screen = chart->screen();
        const qreal refreshRate = screen != nullptr ? screen->refreshRate() : 0.0;
        return QCoreApplication::translate("ChartStressTest", "%1 绘制，数据点 %2 / %3，实际刷新 %4 帧/秒，屏幕刷新率 %5 Hz").arg(renderApiText(chart->getRenderApi())).arg(data.count).arg(kMaxCount).arg(data.framesPerSecond, 0, 'f', 1).arg(refreshRate, 0, 'f', 0);
    }

    // 随机游走：每条在上一条的基础上随机增减，限制在固定区间内
    void appendData(StressData& data, ShadCategoryChart* chart)
    {
        QRandomGenerator* random = QRandomGenerator::global();
        QStringList categories;
        QList<qreal> desktop;
        QList<qreal> mobile;
        categories.reserve(kBatchCount);
        desktop.reserve(kBatchCount);
        mobile.reserve(kBatchCount);
        // 每批只查一次翻译
        const QString categoryFormat = QCoreApplication::translate("ChartStressTest", "第 %1 条");
        for (int i = 0; i < kBatchCount; ++i)
        {
            data.desktopValue = qBound(50.0, data.desktopValue + random->generateDouble() * 40.0 - 20.0, 450.0);
            data.mobileValue = qBound(30.0, data.mobileValue + random->generateDouble() * 30.0 - 15.0, 300.0);
            categories.append(categoryFormat.arg(data.nextIndex));
            desktop.append(std::round(data.desktopValue));
            mobile.append(std::round(data.mobileValue));
            ++data.nextIndex;
        }
        chart->appendData(categories, {mobile, desktop}, kMaxCount);
        data.count = qMin<qsizetype>(data.count + kBatchCount, kMaxCount);
    }

    // 切换语言时按图表中保留的数据重新生成分类文字，数值与缩放范围不变
    void retranslateCategories(const StressData& data, ShadCategoryChart* chart)
    {
        const qsizetype count = chart->getCategories().size();
        if (count == 0)
        {
            return;
        }
        const QString categoryFormat = QCoreApplication::translate("ChartStressTest", "第 %1 条");
        QStringList categories;
        categories.reserve(count);
        for (qint64 index = data.nextIndex - count; index < data.nextIndex; ++index)
        {
            categories.append(categoryFormat.arg(index));
        }
        chart->setCategories(categories);
    }
} // namespace

namespace ChartStressTest
{
    QWidget* createToolbar(QWidget* parent, ShadCategoryChart* chart)
    {
        QWidget* toolbar = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(toolbar);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kBarSpacing);
        ShadSwitch* stressSwitch = new ShadSwitch(toolbar);
        ShadLabel* status = new ShadLabel(toolbar);
        status->setColorRole(ShadThemeType::MutedForeground);
        layout->addWidget(stressSwitch);
        layout->addWidget(status);
        layout->addStretch(1);
        std::shared_ptr<StressData> stress = std::make_shared<StressData>();
        ExampleI18n::bind(toolbar, [stressSwitch, status, stress, chart]()
        {
            stressSwitch->setText(QCoreApplication::translate("ChartStressTest", "压力测试"));
            status->setText(statusText(*stress, chart));
            retranslateCategories(*stress, chart);
        });
        // 上一帧提交后才推送下一批，帧率上限即屏幕刷新率；页面被切走时图表不绘制，推送随之暂停
        QObject::connect(chart, &ShadCategoryChart::frameRendered, status, [stress, chart, status]()
        {
            if (!stress->isRunning)
            {
                return;
            }
            ++stress->frameCount;
            const qint64 elapsed = stress->rateTimer.elapsed();
            // 状态文字每秒更新一次，避免每帧重新布局
            if (elapsed >= kRateWindowMs)
            {
                stress->framesPerSecond = stress->frameCount * 1000.0 / elapsed;
                stress->frameCount = 0;
                stress->rateTimer.restart();
                status->setText(statusText(*stress, chart));
            }
            appendData(*stress, chart);
        });
        QObject::connect(stressSwitch, &ShadSwitch::toggled, chart, [stress, chart](bool checked)
        {
            stress->isRunning = checked;
            if (!checked)
            {
                return;
            }
            stress->frameCount = 0;
            stress->rateTimer.start();
            appendData(*stress, chart);
        });
        return toolbar;
    }
} // namespace ChartStressTest
