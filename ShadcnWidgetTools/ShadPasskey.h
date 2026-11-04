#ifndef SHADPASSKEY_H
#define SHADPASSKEY_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

struct ShadPasskeyRegistrationOptions
{
    QString rpId;
    QString rpName;
    QByteArray challenge;
    QByteArray userId;
    QString userName;
    QString displayName;
    QList<QByteArray> excludeCredentials;
    int timeout{60000};
    ShadPasskeyType::UserVerification userVerification{ShadPasskeyType::Preferred};
};

struct ShadPasskeyAuthenticationOptions
{
    QString rpId;
    QByteArray challenge;
    QList<QByteArray> allowCredentials;
    int timeout{60000};
    ShadPasskeyType::UserVerification userVerification{ShadPasskeyType::Preferred};
};

struct ShadPasskeyCredential
{
    QByteArray credentialId;
    QByteArray clientDataJson;
    QByteArray attestationObject;
};

struct ShadPasskeyAssertion
{
    QByteArray credentialId;
    QByteArray clientDataJson;
    QByteArray authenticatorData;
    QByteArray signature;
    QByteArray userHandle;
};

class ShadPasskeyPrivate;

class SHADCN_EXPORT ShadPasskey : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadPasskey)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool simulated READ isSimulated WRITE setSimulated NOTIFY simulatedChanged)

    public:
        explicit ShadPasskey(QObject* parent = nullptr);
        ~ShadPasskey() override;

        static bool isSupported();
        static QString toBase64Url(const QByteArray& data);
        static QByteArray fromBase64Url(const QString& text);

        void setWindow(QWidget* window);
        QWidget* getWindow() const;

        bool isBusy() const;

        void setSimulated(bool simulated);
        bool isSimulated() const;

        bool registerCredential(const ShadPasskeyRegistrationOptions& options);
        bool authenticate(const ShadPasskeyAuthenticationOptions& options);
        void cancel();

    Q_SIGNALS:
        void registrationFinished(const ShadPasskeyCredential& credential);
        void authenticationFinished(const ShadPasskeyAssertion& assertion);
        void failed(ShadPasskeyType::Error error, const QString& message);
        void busyChanged(bool busy);
        void simulatedChanged(bool simulated);
};

#endif
