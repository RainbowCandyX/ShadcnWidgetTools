#ifndef SHADEXPANDERITEM_H
#define SHADEXPANDERITEM_H

#include <QWidget>

class QPaintEvent;

// ShadExpander 的内容行
class ShadExpanderItem : public QWidget
{
    public:
        ShadExpanderItem(QWidget* content, QWidget* parent = nullptr);
        ~ShadExpanderItem() override;

        QWidget* content() const;
        // 不删除
        QWidget* takeContent();

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QWidget* _content{nullptr};
};

#endif // SHADEXPANDERITEM_H
