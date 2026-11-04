#ifndef EXAMPLEBASEPAGE_H
#define EXAMPLEBASEPAGE_H

#include <QList>
#include <QString>

#include "ExampleI18n.h"
#include "ShadScrollArea.h"

class QHBoxLayout;
class QVBoxLayout;
class QWidget;
class ShadCard;
class ShadLabel;

// 示例页基类：页面标题 + 说明 + 若干演示区块，可滚动
// 标题与说明以返回文字的函数传入（函数内调用 tr()），切换语言时由基类重新调用并刷新
class ExampleBasePage : public ShadScrollArea
{
    Q_OBJECT

    public:
        ExampleBasePage(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, QWidget* parent = nullptr);
        ~ExampleBasePage() override;

        // 当前语言的页面标题
        QString getTitle() const;

    Q_SIGNALS:
        // 切换语言后标题变化时发出，主窗口据此刷新侧栏按钮与面包屑
        void titleChanged(const QString& title);

    protected:
        // 添加一个演示区块，演示内容放入返回卡片的 contentLayout()；标题与说明随语言刷新
        ShadCard* addSection(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description);
        // 在区块中添加一行左对齐排列的组件，宽度不够时自动换行
        void addRow(ShadCard* card, const QList<QWidget*>& widgets);
        // 在区块中纵向追加一个左对齐的组件
        void addItem(ShadCard* card, QWidget* widget);
        // 在区块中纵向追加一个占满卡片宽度的组件，用于提示、代码块、列表项等整宽组件
        void addStretchItem(ShadCard* card, QWidget* widget);
        // 在页面说明后追加组件（说明改为单行显示）
        void addDescriptionWidget(QWidget* widget);

    private:
        QString _title;
        QVBoxLayout* _pageLayout{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        // 首次 addDescriptionWidget 时创建
        QHBoxLayout* _descriptionLayout{nullptr};
};

#endif // EXAMPLEBASEPAGE_H
