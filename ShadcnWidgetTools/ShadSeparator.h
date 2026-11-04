#ifndef SHADSEPARATOR_H
#define SHADSEPARATOR_H

#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QPaintEvent;

// 1px 分隔线，颜色为 border
class ShadSeparatorPrivate;

class SHADCN_EXPORT ShadSeparator : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSeparator)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)

    public:
        explicit ShadSeparator(QWidget* parent = nullptr);
        explicit ShadSeparator(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadSeparator() override;

        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSEPARATOR_H
