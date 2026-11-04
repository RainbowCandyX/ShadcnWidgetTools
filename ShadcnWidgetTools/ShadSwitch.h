#ifndef SHADSWITCH_H
#define SHADSWITCH_H

#include <QAbstractButton>
#include <QPoint>
#include <QSize>
#include <QString>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QPaintEvent;

// 开关，可选在右侧显示文字
class ShadSwitchPrivate;

class SHADCN_EXPORT ShadSwitch : public QAbstractButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSwitch)

    public:
        explicit ShadSwitch(QWidget* parent = nullptr);
        explicit ShadSwitch(const QString& text, QWidget* parent = nullptr);
        ~ShadSwitch() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool hitButton(const QPoint& pos) const override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSWITCH_H
