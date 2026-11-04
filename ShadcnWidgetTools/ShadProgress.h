#ifndef SHADPROGRESS_H
#define SHADPROGRESS_H

#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QPaintEvent;

// 进度条，取值范围 0 到 100，数值变化带过渡动画
class ShadProgressPrivate;

class SHADCN_EXPORT ShadProgress : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadProgress)
    Q_PROPERTY(int value READ getValue WRITE setValue NOTIFY valueChanged)

    public:
        explicit ShadProgress(QWidget* parent = nullptr);
        ~ShadProgress() override;

        // 超出范围的值会被限制到 0 到 100
        void setValue(int value);
        int getValue() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void valueChanged(int value);

    protected:
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADPROGRESS_H
