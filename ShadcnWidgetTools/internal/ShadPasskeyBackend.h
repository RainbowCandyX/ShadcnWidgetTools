#ifndef SHADPASSKEYBACKEND_H
#define SHADPASSKEYBACKEND_H

#include <QString>

#include <functional>
#include <memory>

#include "ShadDef.h"
#include "ShadPasskey.h"

class QWidget;

class ShadPasskeyBackend
{
    public:
        using RegistrationCallback = std::function<void(const ShadPasskeyCredential &)>;
        using AuthenticationCallback = std::function<void(const ShadPasskeyAssertion &)>;
        using ErrorCallback = std::function<void(ShadPasskeyType::Error, const QString &)>;

        ShadPasskeyBackend();
        ~ShadPasskeyBackend();

        static bool isSupported();

        void registerCredential(const ShadPasskeyRegistrationOptions& options, QWidget* window, const RegistrationCallback& onFinished, const ErrorCallback& onError);
        void authenticate(const ShadPasskeyAuthenticationOptions& options, QWidget* window, const AuthenticationCallback& onFinished, const ErrorCallback& onError);
        void cancel();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
};

#endif
