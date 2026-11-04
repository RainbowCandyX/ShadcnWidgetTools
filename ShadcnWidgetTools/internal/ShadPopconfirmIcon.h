#ifndef SHADPOPCONFIRMICON_H
#define SHADPOPCONFIRMICON_H

#include <QSize>
#include <QWidget>

#include "ShadIconType.h"

class QPaintEvent;

class ShadPopconfirmIcon : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadPopconfirmIcon(QWidget* parent = nullptr);
        ~ShadPopconfirmIcon() override;

        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        void setDestructive(bool destructive);

        void setLineHeight(int height);

        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadIconType::IconName _icon{ShadIconType::CircleAlert};
        bool _isDestructive{false};
        int _lineHeight{20};
};

#endif
