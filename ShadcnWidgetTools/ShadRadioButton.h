#ifndef SHADRADIOBUTTON_H
#define SHADRADIOBUTTON_H

#include <QPoint>
#include <QRadioButton>
#include <QSize>
#include <QString>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QPaintEvent;

// 单选项，通常通过 ShadRadioGroup 创建和管理
class ShadRadioButtonPrivate;

class SHADCN_EXPORT ShadRadioButton : public QRadioButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadRadioButton)

    public:
        explicit ShadRadioButton(QWidget* parent = nullptr);
        explicit ShadRadioButton(const QString& text, QWidget* parent = nullptr);
        ~ShadRadioButton() override;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool hitButton(const QPoint& pos) const override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADRADIOBUTTON_H
