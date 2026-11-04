#ifndef SHADTIMECOLUMN_H
#define SHADTIMECOLUMN_H

#include "ShadScrollArea.h"

class QFocusEvent;
class QKeyEvent;
class QVariantAnimation;
class ShadTimeColumnContent;

// ShadTimePicker 面板中的一列（时 / 分 / 秒），组件库内部使用
// 每格 h-8，悬停为 bg-accent，选中为 bg-primary；可见 7 格，底部留出空白使最后一格也能滚动到列顶
// 选择后选中格平滑滚动到列顶；上下方向键与 Home / End 改变选中值，左右方向键与回车交给面板处理
class ShadTimeColumn : public ShadScrollArea
{
    Q_OBJECT

    public:
        static constexpr int kCellHeight = 32;
        static constexpr int kColumnWidth = 56;
        static constexpr int kVisibleRows = 7;

        // count 为可选值的个数，例如小时为 24，分钟与秒为 60
        ShadTimeColumn(int count, QWidget* parent = nullptr);
        ~ShadTimeColumn() override;

        // 选中值，-1 表示未选择
        void setSelectedValue(int value);
        int selectedValue() const;
        // 把 value 所在的格滚动到列顶
        void scrollToValue(int value, bool animated);

        bool isFocusVisible() const;

    Q_SIGNALS:
        // 用户通过鼠标或键盘选择了 value
        void valueSelected(int value);
        // 左右方向键：direction 为 -1（左侧一列）或 1（右侧一列）
        void moveFocusRequested(int direction);
        // 回车
        void confirmRequested();

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    private:
        void selectByUser(int value);

        ShadTimeColumnContent* _content{nullptr};
        QVariantAnimation* _scrollAnimation{nullptr};
        int _count{0};
        int _selectedValue{-1};
        bool _isFocusVisible{false};

        friend class ShadTimeColumnContent;
};

#endif // SHADTIMECOLUMN_H
