#ifndef SHADPOPCONFIRM_H
#define SHADPOPCONFIRM_H

#include <QScopedPointer>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadPopover.h"
#include "ShadProperty.h"

class QAbstractButton;
class QEvent;
class QHideEvent;
class QShowEvent;
class QWidget;
class ShadButton;
class ShadPopconfirmPrivate;

class SHADCN_EXPORT ShadPopconfirm : public ShadPopover
{
    Q_OBJECT
    Q_PROPERTY(bool destructive READ isDestructive WRITE setDestructive)

    public:
        explicit ShadPopconfirm(QWidget* parent = nullptr);
        ShadPopconfirm(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadPopconfirm() override;

        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        void setDestructive(bool destructive);
        bool isDestructive() const;

        ShadButton* confirmButton() const;
        ShadButton* cancelButton() const;

        void attach(QAbstractButton* anchor, ShadPlacementType::Side side = ShadPlacementType::Top, ShadPlacementType::Align align = ShadPlacementType::Center);

    Q_SIGNALS:
        void confirmed();
        void cancelled();

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        void retranslate();
        void resolve(bool isConfirmed);

        QScopedPointer<ShadPopconfirmPrivate> _private;
};

#endif
