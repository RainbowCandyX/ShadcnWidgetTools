#ifndef SHADEXPANDER_H
#define SHADEXPANDER_H

#include <QList>
#include <QWidget>

#include "ShadProperty.h"

class QHBoxLayout;
class QPaintEvent;

// 抽屉面板
class ShadExpanderPrivate;

class SHADCN_EXPORT ShadExpander : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadExpander)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(int animationDuration READ getAnimationDuration WRITE setAnimationDuration)

    public:
        explicit ShadExpander(QWidget* parent = nullptr);
        ~ShadExpander() override;

        QHBoxLayout* headerLayout() const;

        void addItem(QWidget* widget);
        // 只移除，不删除
        void removeItem(QWidget* widget);
        QList<QWidget*> getItems() const;

        void setOpen(bool open);
        bool isOpen() const;

        // 0 为不播放动画
        void setAnimationDuration(int msecs);
        int getAnimationDuration() const;

    public Q_SLOTS:
        void toggle();

    Q_SIGNALS:
        void openChanged(bool open);

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADEXPANDER_H
