#ifndef CHARTSTRESSTEST_H
#define CHARTSTRESSTEST_H

class QWidget;
class ShadCategoryChart;

// 折线图与柱状图共用的压力测试
namespace ChartStressTest
{
    // 工具栏：开关与状态文字；打开后每帧追加 1000 条随机数据，只保留最近 50000 条
    // chart 需要两个系列，序号 0 为移动端、1 为桌面端
    QWidget* createToolbar(QWidget* parent, ShadCategoryChart* chart);
} // namespace ChartStressTest

#endif // CHARTSTRESSTEST_H
