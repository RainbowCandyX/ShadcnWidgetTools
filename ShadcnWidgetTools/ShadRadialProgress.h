#ifndef SHADRADIALPROGRESS_H
#define SHADRADIALPROGRESS_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QPaintEvent;

class ShadRadialProgressPrivate;

class SHADCN_EXPORT ShadRadialProgress : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadRadialProgress)
    Q_PROPERTY(int value READ getValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(ShadRadialProgressType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(ShadRadialProgressType::Status status READ getStatus WRITE setStatus)
    Q_PROPERTY(int diameter READ getDiameter WRITE setDiameter)
    Q_PROPERTY(int thickness READ getThickness WRITE setThickness)
    Q_PROPERTY(bool textVisible READ isTextVisible WRITE setTextVisible)
    Q_PROPERTY(QString text READ getText WRITE setText)

    public:
        explicit ShadRadialProgress(QWidget* parent = nullptr);
        ~ShadRadialProgress() override;

        void setValue(int value);
        int getValue() const;

        void setVariant(ShadRadialProgressType::Variant variant);
        ShadRadialProgressType::Variant getVariant() const;

        void setStatus(ShadRadialProgressType::Status status);
        ShadRadialProgressType::Status getStatus() const;

        void setDiameter(int diameter);
        int getDiameter() const;

        void setThickness(int thickness);
        int getThickness() const;

        void setTextVisible(bool visible);
        bool isTextVisible() const;

        void setText(const QString& text);
        QString getText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void valueChanged(int value);

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif
