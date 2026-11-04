#include "ChartAreaPage.h"

#include <QCoreApplication>
#include <QDate>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QRandomGenerator>
#include <QScreen>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>
#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadAreaChart.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"
#include "ShadWidget.h"

namespace
{
    // 交互示例：2024 年 4 月 1 日到 6 月 30 日，共 91 天
    constexpr int kDayCount = 91;
    // 时间范围下拉框的可视宽度（w-[160px]）
    constexpr int kRangeComboWidth = 160;

    QStringList monthCategories()
    {
        return {
            QCoreApplication::translate("ChartAreaPage", "1月"), QCoreApplication::translate("ChartAreaPage", "2月"),
            QCoreApplication::translate("ChartAreaPage", "3月"), QCoreApplication::translate("ChartAreaPage", "4月"),
            QCoreApplication::translate("ChartAreaPage", "5月"), QCoreApplication::translate("ChartAreaPage", "6月")
        };
    }

    QList<qreal> desktopValues()
    {
        return {186, 305, 237, 73, 209, 214};
    }

    QList<qreal> mobileValues()
    {
        return {80, 200, 120, 190, 130, 140};
    }

    // 1 月到 6 月的桌面端与移动端访问量
    ShadAreaChart* createMonthChart(QWidget* parent, bool hasMobile)
    {
        ShadAreaChart* chart = new ShadAreaChart(parent);
        // 只替换分类文字，数值、缩放与悬停状态不变
        ExampleI18n::bind(chart, [chart]() { chart->setCategories(monthCategories()); });
        // 没有移动端系列时序号为 -1
        const int mobileIndex = hasMobile ? chart->addSeries(QString(), mobileValues(), ShadThemeType::Chart2) : -1;
        const int desktopIndex = chart->addSeries(QString(), desktopValues(), ShadThemeType::Chart1);
        // 只替换系列名称，数值与悬停状态不变
        ExampleI18n::bind(chart, [chart, mobileIndex, desktopIndex]()
        {
            if (mobileIndex >= 0)
            {
                chart->setSeriesLabel(mobileIndex, QCoreApplication::translate("ChartAreaPage", "移动端"));
            }
            chart->setSeriesLabel(desktopIndex, QCoreApplication::translate("ChartAreaPage", "桌面端"));
        });
        return chart;
    }

    // 每日访问量：由几条正弦波叠加生成，数值固定，每次启动相同
    struct DailyData
    {
        QStringList categories;
        QStringList tickLabels;
        QList<qreal> desktop;
        QList<qreal> mobile;
    };

    DailyData dailyData(int dayCount)
    {
        DailyData data;
        const QDate firstDate(2024, 4, 1);
        for (int day = kDayCount - dayCount; day < kDayCount; ++day)
        {
            const QDate date = firstDate.addDays(day);
            data.categories.append(QCoreApplication::translate("ChartAreaPage", "%1年%2月%3日").arg(date.year()).arg(date.month()).arg(date.day()));
            data.tickLabels.append(QCoreApplication::translate("ChartAreaPage", "%1月%2日").arg(date.month()).arg(date.day()));
            data.desktop.append(std::round(220.0 + 120.0 * std::sin(day * 0.37) + 80.0 * std::sin(day * 0.11 + 1.0) + (day * 37) % 50));
            data.mobile.append(std::round(180.0 + 90.0 * std::sin(day * 0.29 + 2.0) + 60.0 * std::sin(day * 0.07) + (day * 53) % 40));
        }
        return data;
    }

    // 压力测试：图表每提交一帧追加 1000 条随机数据，只保留最近 50000 条
    constexpr int kStressBatchCount = 1000;
    constexpr int kStressMaxCount = 50000;
    // 刷新频率每秒统计一次
    constexpr qint64 kStressRateWindowMs = 1000;
    // 开关与状态文字的间距
    constexpr int kStressBarSpacing = 16;

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

    QString stressCategoryText(qint64 index)
    {
        return QCoreApplication::translate("ChartAreaPage", "第 %1 条").arg(index);
    }

    // 按当前语言重新生成压力测试的分类文字：图表中保存最近的 n 条，序号为 nextIndex - n 到 nextIndex - 1；只替换文字，数值与缩放不变
    void relabelStressData(const StressData& data, ShadAreaChart* chart)
    {
        const qsizetype count = chart->getCategories().size();
        QStringList categories;
        categories.reserve(count);
        for (qint64 index = data.nextIndex - count; index < data.nextIndex; ++index)
        {
            categories.append(stressCategoryText(index));
        }
        chart->setCategories(categories);
    }

    // 随机游走：每条在上一条的基础上随机增减，限制在固定区间内；通过 appendData 追加，只保留最近 kStressMaxCount 条
    void appendStressData(StressData& data, ShadAreaChart* chart)
    {
        QRandomGenerator* random = QRandomGenerator::global();
        QStringList categories;
        QList<qreal> desktop;
        QList<qreal> mobile;
        categories.reserve(kStressBatchCount);
        desktop.reserve(kStressBatchCount);
        mobile.reserve(kStressBatchCount);
        for (int i = 0; i < kStressBatchCount; ++i)
        {
            data.desktopValue = qBound(50.0, data.desktopValue + random->generateDouble() * 40.0 - 20.0, 450.0);
            data.mobileValue = qBound(30.0, data.mobileValue + random->generateDouble() * 30.0 - 15.0, 300.0);
            categories.append(stressCategoryText(data.nextIndex));
            desktop.append(std::round(data.desktopValue));
            mobile.append(std::round(data.mobileValue));
            ++data.nextIndex;
        }
        // 系列顺序：0 为移动端，1 为桌面端
        chart->appendData(categories, {mobile, desktop}, kStressMaxCount);
        data.count = qMin<qsizetype>(data.count + kStressBatchCount, kStressMaxCount);
    }

    QString renderApiText(ShadChartType::RenderApi api)
    {
        switch (api)
        {
            case ShadChartType::Auto:
                return QCoreApplication::translate("ChartAreaPage", "自动");
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

    // 屏幕刷新率取图表当前所在屏幕，窗口移到另一块屏幕后随之变化
    QString stressStatusText(const StressData& data, const ShadAreaChart* chart)
    {
        const QScreen* screen = chart->screen();
        const qreal refreshRate = screen != nullptr ? screen->refreshRate() : 0.0;
        return QCoreApplication::translate("ChartAreaPage", "%1 绘制，数据点 %2 / %3，实际刷新 %4 帧/秒，屏幕刷新率 %5 Hz").arg(renderApiText(chart->getRenderApi())).arg(data.count).arg(kStressMaxCount).arg(data.framesPerSecond, 0, 'f', 1).arg(refreshRate, 0, 'f', 0);
    }

    // 压力测试的工具栏与图表，页面与新窗口共用
    struct StressDemo
    {
        QWidget* bar{nullptr};
        ShadAreaChart* chart{nullptr};
    };

    // 工具栏：API 下拉框（hasApiSwitch 为 true 时）、压力测试开关与状态文字；打开开关后持续追加随机数据
    StressDemo createStressDemo(QWidget* parent, bool hasApiSwitch)
    {
        QWidget* stressBar = new QWidget(parent);
        QHBoxLayout* stressLayout = new QHBoxLayout(stressBar);
        stressLayout->setContentsMargins(0, 0, 0, 0);
        stressLayout->setSpacing(kStressBarSpacing);
        ShadSwitch* stressSwitch = new ShadSwitch(stressBar);
        std::shared_ptr<StressData> stress = std::make_shared<StressData>();
        ShadLabel* stressStatus = new ShadLabel(stressBar);
        stressStatus->setColorRole(ShadThemeType::MutedForeground);
        stressLayout->addWidget(stressSwitch);
        stressLayout->addWidget(stressStatus);
        stressLayout->addStretch(1);
        ShadAreaChart* stressChart = new ShadAreaChart(parent);
        const int stressMobileIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart2);
        const int stressDesktopIndex = stressChart->addSeries(QString(), {}, ShadThemeType::Chart1);
        stressChart->setStackMode(ShadChartType::Stacked);
        stressChart->setZoomEnabled(true);
        // 状态文字按当前统计重新生成
        ExampleI18n::bind(stressBar, [stress, stressSwitch, stressStatus, stressChart]()
        {
            stressSwitch->setText(QCoreApplication::translate("ChartAreaPage", "压力测试"));
            stressStatus->setText(stressStatusText(*stress, stressChart));
        });
        ExampleI18n::bind(stressChart, [stress, stressChart, stressMobileIndex, stressDesktopIndex]()
        {
            stressChart->setSeriesLabel(stressMobileIndex, QCoreApplication::translate("ChartAreaPage", "移动端"));
            stressChart->setSeriesLabel(stressDesktopIndex, QCoreApplication::translate("ChartAreaPage", "桌面端"));
            relabelStressData(*stress, stressChart);
        });
        if (hasApiSwitch)
        {
            // 同一窗口只能用一种 GPU API，可选项为窗口的 GPU API 与 CPU；GPU 不可用时只有 CPU
            QList<ShadChartType::RenderApi> apis;
            if (stressChart->getRenderApi() != ShadChartType::Software)
            {
                apis.append(stressChart->getRenderApi());
            }
            apis.append(ShadChartType::Software);
            ShadComboBox* apiComboBox = new ShadComboBox(stressBar);
            apiComboBox->setFixedWidth(kRangeComboWidth + 2 * ExampleLayout::kFocusRingWidth);
            for (int index = 0; index < apis.size(); ++index)
            {
                apiComboBox->addItem(QString());
            }
            // 按序号刷新选项文字，当前选项不变
            ExampleI18n::bind(apiComboBox, [apiComboBox, apis]()
            {
                for (int index = 0; index < apis.size(); ++index)
                {
                    apiComboBox->setItemText(index, renderApiText(apis.at(index)));
                }
            });
            QObject::connect(apiComboBox, &ShadComboBox::currentIndexChanged, stressChart, [stress, stressChart, stressStatus, apis](int index)
            {
                stressChart->setRenderApi(apis.value(index, ShadChartType::Software));
                stressStatus->setText(stressStatusText(*stress, stressChart));
            });
            stressLayout->insertWidget(0, apiComboBox);
        }
        // 推送一批数据：appendData 只在末尾追加、开头移除，不复制已有的 5 万条数据
        const auto pushStressFrame = [stress, stressChart]() { appendStressData(*stress, stressChart); };
        // 按帧驱动：上一帧提交后才推送下一批，帧的呈现受垂直同步限制，帧率上限即屏幕刷新率，不受固定定时器间隔限制
        // 页面被切走时图表不绘制，推送随之暂停，回到页面后自动继续
        QObject::connect(stressChart, &ShadAreaChart::frameRendered, stressStatus, [stress, stressChart, stressStatus, pushStressFrame]()
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
            pushStressFrame();
        });
        QObject::connect(stressSwitch, &ShadSwitch::toggled, stressChart, [stress, pushStressFrame](bool checked)
        {
            stress->isRunning = checked;
            if (!checked)
            {
                return;
            }
            stress->frameCount = 0;
            stress->rateTimer.start();
            pushStressFrame();
        });
        return {stressBar, stressChart};
    }

    // 新窗口的尺寸与间距
    constexpr int kApiWindowWidth = 720;
    constexpr int kApiWindowHeight = 420;
    constexpr int kApiWindowPadding = 24;
    constexpr int kApiWindowGap = 16;

    // 新窗口：压力测试图表使用 api 绘制
    ShadWidget* createRenderApiWindow(ShadChartType::RenderApi api)
    {
        ShadWidget* window = new ShadWidget();
        ExampleI18n::bind(window, [window, api]()
        {
            window->setWindowTitle(QCoreApplication::translate("ChartAreaPage", "面积图：%1").arg(renderApiText(api)));
        });
        window->resize(kApiWindowWidth, kApiWindowHeight);
        ExampleLayout::addStayOnTopButton(window);
        // 不阻止主窗口退出
        window->setAttribute(Qt::WA_QuitOnClose, false);
        QVBoxLayout* layout = new QVBoxLayout(window);
        layout->setContentsMargins(kApiWindowPadding, kApiWindowPadding, kApiWindowPadding, kApiWindowPadding);
        layout->setSpacing(kApiWindowGap);
        // 只对本窗口的图表生效，创建后恢复原值
        const ShadChartType::RenderApi previousApi = ShadAreaChart::getDefaultRenderApi();
        ShadAreaChart::setDefaultRenderApi(api);
        // 新窗口固定使用所选 API，不提供切换
        const StressDemo demo = createStressDemo(window, false);
        ShadAreaChart::setDefaultRenderApi(previousApi);
        // 图表在窗口背景上
        demo.chart->setSurfaceColor(ShadThemeType::Background);
        layout->addWidget(demo.bar);
        layout->addWidget(demo.chart, 1);
        return window;
    }
} // namespace

ChartAreaPage::ChartAreaPage(QWidget* parent)
    : ExampleBasePage([] { return tr("面积图"); }, [] { return tr("ShadAreaChart：GPU 绘制的面积图，GPU 不可用时改用 CPU。"); }, parent)
{
    // 默认：自然曲线
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, [] { return tr("自然曲线，悬停显示光标线与提示"); });
    ShadAreaChart* defaultChart = createMonthChart(this, false);
    defaultChart->setTooltipIndicator(ShadChartType::Line);
    addStretchItem(defaultCard, defaultChart);

    // 线性
    ShadCard* linearCard = addSection([] { return tr("线性"); }, [] { return tr("setCurveType(Linear)：相邻数据点直线相连"); });
    ShadAreaChart* linearChart = createMonthChart(this, false);
    linearChart->setCurveType(ShadChartType::Linear);
    addStretchItem(linearCard, linearChart);

    // 阶梯
    ShadCard* stepCard = addSection([] { return tr("阶梯"); }, [] { return tr("setCurveType(Step)：在相邻两点的中点处跳变"); });
    ShadAreaChart* stepChart = createMonthChart(this, false);
    stepChart->setCurveType(ShadChartType::Step);
    addStretchItem(stepCard, stepChart);

    // 图例
    ShadCard* legendCard = addSection([] { return tr("图例"); }, [] { return tr("setLegendVisible(true)：图表下方显示系列名称"); });
    ShadAreaChart* legendChart = createMonthChart(this, true);
    legendChart->setStackMode(ShadChartType::Stacked);
    legendChart->setLegendVisible(true);
    legendChart->setTooltipIndicator(ShadChartType::Line);
    addStretchItem(legendCard, legendChart);

    // 堆叠
    ShadCard* stackedCard = addSection([] { return tr("堆叠"); }, [] { return tr("setStackMode(Stacked)：后一个系列叠加在前一个之上"); });
    ShadAreaChart* stackedChart = createMonthChart(this, true);
    stackedChart->setStackMode(ShadChartType::Stacked);
    addStretchItem(stackedCard, stackedChart);

    // 百分比堆叠
    ShadCard* expandedCard = addSection([] { return tr("百分比堆叠"); }, []
    {
        return tr("setStackMode(Expanded)：每个分类按总和归一化为 0% 到 100%");
    });
    ShadAreaChart* expandedChart = createMonthChart(this, true);
    const int otherIndex = expandedChart->addSeries(QString(), {45, 60, 52, 38, 70, 64}, ShadThemeType::Chart3);
    ExampleI18n::bind(expandedChart, [expandedChart, otherIndex]()
    {
        expandedChart->setSeriesLabel(otherIndex, tr("其他"));
    });
    expandedChart->setStackMode(ShadChartType::Expanded);
    expandedChart->setYAxisVisible(true);
    expandedChart->setTooltipIndicator(ShadChartType::Line);
    addStretchItem(expandedCard, expandedChart);

    // 图标
    ShadCard* iconCard = addSection([] { return tr("图标"); }, [] { return tr("setSeriesIcon：图例与提示用图标代替颜色标记"); });
    ShadAreaChart* iconChart = createMonthChart(this, true);
    iconChart->setSeriesIcon(0, ShadIconType::TrendingUp);
    iconChart->setSeriesIcon(1, ShadIconType::TrendingDown);
    iconChart->setStackMode(ShadChartType::Stacked);
    iconChart->setLegendVisible(true);
    iconChart->setTooltipIndicator(ShadChartType::Line);
    addStretchItem(iconCard, iconChart);

    // 渐变
    ShadCard* gradientCard = addSection([] { return tr("渐变"); }, [] { return tr("setFillStyle(Gradient)：填充从上到下渐隐"); });
    ShadAreaChart* gradientChart = createMonthChart(this, true);
    gradientChart->setStackMode(ShadChartType::Stacked);
    gradientChart->setFillStyle(ShadChartType::Gradient);
    addStretchItem(gradientCard, gradientChart);

    // 坐标轴
    ShadCard* axesCard = addSection([] { return tr("坐标轴"); }, []
    {
        return tr("setYAxisVisible(true)：显示 Y 轴刻度，setYTickCount(3) 减少刻度");
    });
    ShadAreaChart* axesChart = createMonthChart(this, true);
    axesChart->setStackMode(ShadChartType::Stacked);
    axesChart->setYAxisVisible(true);
    axesChart->setYTickCount(3);
    addStretchItem(axesCard, axesChart);

    // 交互：切换时间范围
    ShadCard* interactiveCard = addSection([] { return tr("交互"); }, [] { return tr("切换时间范围后更新数据，X 轴刻度过密时自动间隔显示"); });
    ShadComboBox* rangeComboBox = new ShadComboBox(this);
    rangeComboBox->addItems({QString(), QString(), QString()});
    rangeComboBox->setFixedWidth(kRangeComboWidth + 2 * ExampleLayout::kFocusRingWidth);
    // 按序号刷新选项文字，当前选项不变
    ExampleI18n::bind(rangeComboBox, [rangeComboBox]()
    {
        rangeComboBox->setItemText(0, tr("最近 3 个月"));
        rangeComboBox->setItemText(1, tr("最近 30 天"));
        rangeComboBox->setItemText(2, tr("最近 7 天"));
    });
    ShadAreaChart* interactiveChart = new ShadAreaChart(this);
    const int interactiveMobileIndex = interactiveChart->addSeries(QString(), {}, ShadThemeType::Chart2);
    const int interactiveDesktopIndex = interactiveChart->addSeries(QString(), {}, ShadThemeType::Chart1);
    ExampleI18n::bind(interactiveChart, [interactiveChart, interactiveMobileIndex, interactiveDesktopIndex]()
    {
        interactiveChart->setSeriesLabel(interactiveMobileIndex, tr("移动端"));
        interactiveChart->setSeriesLabel(interactiveDesktopIndex, tr("桌面端"));
    });
    interactiveChart->setStackMode(ShadChartType::Stacked);
    interactiveChart->setFillStyle(ShadChartType::Gradient);
    interactiveChart->setLegendVisible(true);
    const auto rangeDayCount = [](int index)
    {
        static const int dayCounts[] = {kDayCount, 30, 7};
        return dayCounts[qBound(0, index, 2)];
    };
    const auto applyRange = [interactiveChart, rangeDayCount](int index)
    {
        const DailyData data = dailyData(rangeDayCount(index));
        interactiveChart->setCategories(data.categories);
        interactiveChart->setTickLabels(data.tickLabels);
        interactiveChart->setSeriesValues(0, data.mobile);
        interactiveChart->setSeriesValues(1, data.desktop);
    };
    connect(rangeComboBox, &ShadComboBox::currentIndexChanged, interactiveChart, applyRange);
    applyRange(0);
    // 切换语言时只按当前时间范围替换分类与刻度文字，数值不变
    ExampleI18n::bind(interactiveChart, [interactiveChart, rangeComboBox, rangeDayCount]()
    {
        const DailyData data = dailyData(rangeDayCount(rangeComboBox->currentIndex()));
        interactiveChart->setCategories(data.categories);
        interactiveChart->setTickLabels(data.tickLabels);
    });
    addItem(interactiveCard, rangeComboBox);
    addStretchItem(interactiveCard, interactiveChart);

    // 绘制 API 与压力测试：切换绘图层后对比帧率；打开开关后持续追加随机数据，状态文字显示绘制 API、数据量与帧率
    ShadCard* stressCard = addSection([] { return tr("绘制 API 与压力测试"); }, []
    {
        return tr("setRenderApi：在窗口的 GPU API 与 CPU 之间切换，其他 API 在新窗口中打开；每帧追加 1000 条随机数据，只保留最近 50000 条");
    });
    const StressDemo stressDemo = createStressDemo(this, true);
    addStretchItem(stressCard, stressDemo.bar);
    addStretchItem(stressCard, stressDemo.chart);

    // 其他 GPU API 在新窗口中打开：新窗口是另一个顶层窗口，可以用与主窗口不同的 API
    const QList<ShadChartType::RenderApi> windowApis = ShadAreaChart::availableRenderApis();
    ShadComboBox* windowApiComboBox = new ShadComboBox(this);
    for (int index = 0; index < windowApis.size(); ++index)
    {
        windowApiComboBox->addItem(QString());
    }
    ExampleI18n::bind(windowApiComboBox, [windowApiComboBox, windowApis]()
    {
        for (int index = 0; index < windowApis.size(); ++index)
        {
            windowApiComboBox->setItemText(index, renderApiText(windowApis.at(index)));
        }
    });
    windowApiComboBox->setFixedWidth(kRangeComboWidth + 2 * ExampleLayout::kFocusRingWidth);
    ShadButton* openButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(openButton, [openButton]() { openButton->setText(tr("在新窗口中打开")); });
    connect(openButton, &ShadButton::clicked, this, [this, windowApiComboBox, windowApis]()
    {
        const ShadChartType::RenderApi api = windowApis.value(windowApiComboBox->currentIndex(), ShadChartType::Auto);
        // 窗口的 API 显示后固定，换 API 时重建窗口
        if (!_renderApiWindow.isNull() && _renderApiWindowApi != api)
        {
            delete _renderApiWindow.data();
        }
        _renderApiWindowApi = api;
        ExampleLayout::presentTransientWindow(_renderApiWindow, [api]() { return createRenderApiWindow(api); });
    });
    addRow(stressCard, {windowApiComboBox, openButton});
}

ChartAreaPage::~ChartAreaPage()
{
    // 新窗口无父组件，随页面销毁
    delete _renderApiWindow.data();
}
