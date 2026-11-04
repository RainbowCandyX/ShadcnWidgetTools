#ifndef SHADBIOMETRICPRIVATE_H
#define SHADBIOMETRICPRIVATE_H

#include <QPointer>
#include <QString>
#include <QWidget>

#include <memory>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadBiometric;
class ShadBiometricBackend;

class ShadBiometricPrivate
{
    SHAD_D_CREATE(ShadBiometric)

    public:
        ShadBiometricPrivate();
        ~ShadBiometricPrivate();

        QWidget* resolveWindow() const;
        void setBusy(bool busy);
        void finish(bool isSuccess, ShadBiometricType::Error error, const QString& detail);
        static QString errorMessage(ShadBiometricType::Error error, const QString& detail);

        std::unique_ptr<ShadBiometricBackend> _backend;
        QPointer<QWidget> _window;
        ShadBiometricType::Policy _policy{ShadBiometricType::Biometrics};
        bool _isBusy{false};
};

#endif
