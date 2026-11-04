#ifndef SHADDOUBLESPINBOX_H
#define SHADDOUBLESPINBOX_H

#include <QDoubleSpinBox>
#include <QSize>

#include "ShadDef.h"
#include "ShadProperty.h"

class QContextMenuEvent;
class QWheelEvent;

// 小数输入框：滚轮与右键菜单行为和 ShadSpinBox 一致
class ShadDoubleSpinBoxPrivate;

class SHADCN_EXPORT ShadDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDoubleSpinBox)
    Q_PROPERTY(ShadSpinBoxType::ButtonLayout buttonLayout READ getButtonLayout WRITE setButtonLayout)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)

    public:
        explicit ShadDoubleSpinBox(QWidget* parent = nullptr);
        ~ShadDoubleSpinBox() override;

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

#endif // SHADDOUBLESPINBOX_H
