#include "ShadBiometricBackend.h"

#include <QString>
#include <QWidget>
#include <QtGlobal>

#include <functional>
#include <memory>

#include "ShadDef.h"

#import <Foundation/Foundation.h>
#import <LocalAuthentication/LocalAuthentication.h>

namespace
{
    void reportError(NSError* error, const ShadBiometricBackend::Callback& onFinished)
    {
        ShadBiometricType::Error result = ShadBiometricType::Other;
        QString detail;
        const bool isLocalAuthenticationError = error != nil && [error.domain isEqualToString:LAErrorDomain];
        switch (isLocalAuthenticationError ? error.code : 0)
        {
            case LAErrorUserCancel:
            case LAErrorSystemCancel:
            case LAErrorAppCancel:
            case LAErrorUserFallback:
                result = ShadBiometricType::Cancelled;
                break;
            case LAErrorBiometryNotAvailable:
                result = ShadBiometricType::NotSupported;
                break;
            case LAErrorBiometryNotEnrolled:
            case LAErrorPasscodeNotSet:
                result = ShadBiometricType::NotEnrolled;
                break;
            case LAErrorBiometryLockout:
                result = ShadBiometricType::Lockout;
                break;
            case LAErrorAuthenticationFailed:
                result = ShadBiometricType::Failed;
                break;
            default:
                detail = error != nil ? QString::fromNSString(error.localizedDescription) : QString();
                break;
        }
        onFinished(false, result, detail);
    }

    void deliver(const std::shared_ptr<ShadBiometricBackend::Callback>& callback, BOOL success, NSError* error)
    {
        if (!*callback)
        {
            return;
        }
        const ShadBiometricBackend::Callback onFinished = *callback;
        *callback = nullptr;
        if (success)
        {
            onFinished(true, ShadBiometricType::Other, QString());
            return;
        }
        reportError(error, onFinished);
    }
}

struct ShadBiometricBackend::Impl
{
    LAContext* context{nil};
    std::shared_ptr<Callback> callback;

    void release()
    {
        if (callback)
        {
            *callback = nullptr;
            callback.reset();
        }
        [context invalidate];
        context = nil;
    }
};

ShadBiometricBackend::ShadBiometricBackend()
    : _impl(std::make_unique<Impl>())
{
}

ShadBiometricBackend::~ShadBiometricBackend()
{
    _impl->release();
}

bool ShadBiometricBackend::isSupported()
{
    return true;
}

ShadBiometricType::Method ShadBiometricBackend::availableMethod(ShadBiometricType::Policy policy)
{
    LAContext* context = [[LAContext alloc] init];
    if ([context canEvaluatePolicy:LAPolicyDeviceOwnerAuthenticationWithBiometrics error:nil])
    {
        return context.biometryType == LABiometryTypeFaceID ? ShadBiometricType::Face : ShadBiometricType::Fingerprint;
    }
    if (policy == ShadBiometricType::DeviceOwner && [context canEvaluatePolicy:LAPolicyDeviceOwnerAuthentication error:nil])
    {
        return ShadBiometricType::Password;
    }
    return ShadBiometricType::Unavailable;
}

void ShadBiometricBackend::authenticate(ShadBiometricType::Policy policy, const QString& reason, QWidget* window, const Callback& onFinished)
{
    Q_UNUSED(window);
    _impl->release();
    LAContext* context = [[LAContext alloc] init];
    const LAPolicy localPolicy = policy == ShadBiometricType::DeviceOwner ? LAPolicyDeviceOwnerAuthentication : LAPolicyDeviceOwnerAuthenticationWithBiometrics;
    if (policy == ShadBiometricType::Biometrics)
    {
        context.localizedFallbackTitle = @"";
    }
    const std::shared_ptr<Callback> callback = std::make_shared<Callback>(onFinished);
    _impl->context = context;
    _impl->callback = callback;
    NSError* error = nil;
    if (![context canEvaluatePolicy:localPolicy error:&error])
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            deliver(callback, NO, error);
        });
        return;
    }
    [context evaluatePolicy:localPolicy localizedReason:reason.toNSString() reply:^(BOOL success, NSError* evaluateError) {
        dispatch_async(dispatch_get_main_queue(), ^{
            deliver(callback, success, evaluateError);
        });
    }];
}

void ShadBiometricBackend::cancel()
{
    [_impl->context invalidate];
}
