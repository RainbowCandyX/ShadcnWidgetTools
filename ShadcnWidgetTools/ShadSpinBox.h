#ifndef SHADSPINBOX_H
#define SHADSPINBOX_H

#include <QSize>
#include <QSpinBox>

#include "ShadDef.h"
#include "ShadProperty.h"

class QContextMenuEvent;
class QWheelEvent;

// 整数输入框：滚轮只在获得焦点后改变数值
class ShadSpinBoxPrivate;

class SHADCN_EXPORT ShadSpinBox : public QSpinBox
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSpinBox)
    Q_PROPERTY(ShadSpinBoxType::ButtonLayout buttonLayout READ getButtonLayout WRITE setButtonLayout)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)

    public:
        explicit ShadSpinBox(QWidget* parent = nullptr);
        ~ShadSpinBox() override;

        void setButtonLayout(ShadSpinBoxType::ButtonLayout layout);
        ShadSpinBoxType::ButtonLayout getButtonLayout() const;

        void setInvalid(bool invalid);
        bool isInvalid() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        void contextMenuEvent(QContextMenuEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
};

#endif // SHADSPINBOX_H
