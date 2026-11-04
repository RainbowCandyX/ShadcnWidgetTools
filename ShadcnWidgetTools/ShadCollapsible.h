#ifndef SHADCOLLAPSIBLE_H
#define SHADCOLLAPSIBLE_H

#include <QWidget>

#include "ShadProperty.h"

class QAbstractButton;
class QVBoxLayout;

// 折叠面板，自身没有样式
class ShadCollapsiblePrivate;

class SHADCN_EXPORT ShadCollapsible : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCollapsible)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(int animationDuration READ getAnimationDuration WRITE setAnimationDuration)

    public:
        explicit ShadCollapsible(QWidget* parent = nullptr);
        ~ShadCollapsible() override;

        QVBoxLayout* headerLayout() const;
        QVBoxLayout* contentLayout() const;

        // 按钮不归本组件所有
        void setTrigger(QAbstractButton* button);
        QAbstractButton* getTrigger() const;

        void setOpen(bool open);
        bool isOpen() const;

        // 0 为不播放动画
        void setAnimationDuration(int msecs);
        int getAnimationDuration() const;

    public Q_SLOTS:
        void toggle();

    Q_SIGNALS:
        void openChanged(bool open);
};

#endif // SHADCOLLAPSIBLE_H
