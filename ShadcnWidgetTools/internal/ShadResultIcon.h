#ifndef SHADRESULTICON_H
#define SHADRESULTICON_H

#include <QColor>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"

class QPaintEvent;

class ShadResultIcon : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadResultIcon(QWidget* parent = nullptr);
        ~ShadResultIcon() override;

        void setStatus(ShadResultType::Status status);
        ShadResultType::Status getStatus() const;

        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadResultType::Status _status{ShadResultType::Info};
        ShadIconType::IconName _icon{ShadIconType::None};
        QColor _color;
};

#endif // SHADRESULTICON_H
