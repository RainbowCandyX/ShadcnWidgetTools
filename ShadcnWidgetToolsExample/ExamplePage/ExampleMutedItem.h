#ifndef EXAMPLEMUTEDITEM_H
#define EXAMPLEMUTEDITEM_H

#include <QWidget>

class QHBoxLayout;
class QPaintEvent;

// 示例用的横向条目容器，近似 shadcn Item variant="muted"：bg-muted/50 rounded-lg p-4 gap-4
// 组件库没有 Item 组件，只在示例页中使用；内容放入 itemLayout()
class ExampleMutedItem : public QWidget
{
    Q_OBJECT

    public:
        explicit ExampleMutedItem(QWidget* parent = nullptr);
        ~ExampleMutedItem() override;

        QHBoxLayout* itemLayout() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QHBoxLayout* _layout{nullptr};
};

#endif // EXAMPLEMUTEDITEM_H
