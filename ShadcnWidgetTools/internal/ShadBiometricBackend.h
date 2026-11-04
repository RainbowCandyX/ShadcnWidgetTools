#ifndef SHADBIOMETRICBACKEND_H
#define SHADBIOMETRICBACKEND_H

#include <QString>

#include <functional>
#include <memory>

#include "ShadDef.h"

class QWidget;

class ShadBiometricBackend
{
    public:
        using Callback = std::function<void(bool, ShadBiometricType::Error, const QString &)>;

        ShadBiometricBackend();
        ~ShadBiometricBackend();

        static bool isSupported();
        static ShadBiometricType::Method availableMethod(ShadBiometricType::Policy policy);

        void authenticate(ShadBiometricType::Policy policy, const QString& reason, QWidget* window, const Callback& onFinished);
        void cancel();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
};

#endif
