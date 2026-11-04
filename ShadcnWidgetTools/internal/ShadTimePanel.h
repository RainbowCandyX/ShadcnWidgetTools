#ifndef SHADTIMEPANEL_H
#define SHADTIMEPANEL_H

#include <QList>
#include <QTime>
#include <QWidget>

class QEvent;
class QPaintEvent;
class ShadButton;
class ShadTimeColumn;

// ShadTimePicker 浮层中的面板，组件库内部使用：时、分、秒三列（秒可隐藏），列之间 1px Border 色分隔线，底部为"确定"按钮
// 列的顺序固定为"时 分 秒"从左到右，与时间文字的读法一致，RTL 下不镜像；页脚随布局方向镜像，确定按钮位于结束侧
// 选择任一列而其余列尚未选择时，其余列自动选择 00；三列都有值后确定按钮才可用
class ShadTimePanel : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadTimePanel(QWidget* parent = nullptr);
        ~ShadTimePanel() override;

        void setSecondsVisible(bool visible);
        bool isSecondsVisible() const;

        // 面板中待确定的时间，传无效时间清空选择
        void setTime(const QTime& time);
        // 任一可见列未选择时返回无效时间；隐藏秒时秒为 0
        QTime time() const;

        // 各列立即滚动到选中值，未选择的列滚动到 00
        void scrollToSelection();
        void focusFirstColumn();

    Q_SIGNALS:
        void confirmed(const QTime& time);

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 各列的无障碍名称与确定按钮文字，构造时与切换语言时设置
        void retranslate();
        QList<ShadTimeColumn*> visibleColumns() const;
        void onValueSelected();
        void moveFocus(ShadTimeColumn* column, int direction);
        void confirm();
        void updateConfirmButton();

        QWidget* _columnsArea{nullptr};
        QWidget* _footer{nullptr};
        ShadTimeColumn* _hourColumn{nullptr};
        ShadTimeColumn* _minuteColumn{nullptr};
        ShadTimeColumn* _secondColumn{nullptr};
        ShadButton* _confirmButton{nullptr};
};

#endif // SHADTIMEPANEL_H
