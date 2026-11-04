#ifndef SHADPASSKEYBUTTON_H
#define SHADPASSKEYBUTTON_H

#include <QScopedPointer>
#include <QString>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadPasskey.h"
#include "ShadProperty.h"

class QEvent;
class QWidget;
class ShadPasskeyButtonPrivate;

class SHADCN_EXPORT ShadPasskeyButton : public ShadButton
{
    Q_OBJECT

    public:
        explicit ShadPasskeyButton(QWidget* parent = nullptr);
        ~ShadPasskeyButton() override;

        ShadPasskey* passkey() const;

    Q_SIGNALS:
        void registrationFinished(const ShadPasskeyCredential& credential);
        void authenticationFinished(const ShadPasskeyAssertion& assertion);
        void failed(ShadPasskeyType::Error error, const QString& message);

    protected:
        void changeEvent(QEvent* event) override;

    private:
        void retranslate();
        QScopedPointer<ShadPasskeyButtonPrivate> _private;
};

#endif
