#ifndef SHADMAPCOMPASSBUTTON_H
#define SHADMAPCOMPASSBUTTON_H

#include "ShadButton.h"

class QPaintEvent;
class QWidget;

class ShadMapCompassButton : public ShadButton
{
    public:
        explicit ShadMapCompassButton(QWidget* parent = nullptr);
        ~ShadMapCompassButton() override;

        void setBearing(double bearing);
        double getBearing() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        double _bearing{0.0};
};

#endif // SHADMAPCOMPASSBUTTON_H
