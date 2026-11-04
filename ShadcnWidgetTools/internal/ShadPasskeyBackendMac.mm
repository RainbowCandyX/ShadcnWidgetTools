#include "ShadPasskeyBackend.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QWidget>

#include <functional>
#include <utility>

#include "ShadDef.h"
#include "ShadPasskey.h"

#import <AppKit/AppKit.h>
#import <AuthenticationServices/AuthenticationServices.h>
#import <Foundation/Foundation.h>

using ShadPasskeyHandler = std::function<void(ASAuthorization*, NSError*)>;

@interface ShadPasskeyDelegate : NSObject <ASAuthorizationControllerDelegate, ASAuthorizationControllerPresentationContextProviding>
- (instancetype)initWithAnchor:(NSWindow*)anchor handler:(ShadPasskeyHandler)handler;
- (void)invalidate;
@end

@implementation ShadPasskeyDelegate
{
    NSWindow* _anchor;
    ShadPasskeyHandler _handler;
}

- (instancetype)initWithAnchor:(NSWindow*)anchor handler:(ShadPasskeyHandler)handler
{
    self = [super init];
    if (self != nil)
    {
        _anchor = anchor;
        _handler = std::move(handler);
    }
    return self;
}

- (void)invalidate
{
    _handler = nullptr;
}

- (void)finishWithAuthorization:(ASAuthorization*)authorization error:(NSError*)error
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!self->_handler)
        {
            return;
        }
        const ShadPasskeyHandler handler = std::move(self->_handler);
        self->_handler = nullptr;
        handler(authorization, error);
    });
}

- (void)authorizationController:(ASAuthorizationController*)controller didCompleteWithAuthorization:(ASAuthorization*)authorization
{
    [self finishWithAuthorization:authorization error:nil];
}

- (void)authorizationController:(ASAuthorizationController*)controller didCompleteWithError:(NSError*)error
{
    [self finishWithAuthorization:nil error:error];
}

- (ASPresentationAnchor)presentationAnchorForAuthorizationController:(ASAuthorizationController*)controller
{
    if (_anchor != nil)
    {
        return _anchor;
    }
    return [[NSWindow alloc] init];
}
@end

namespace
{
    NSWindow* anchorWindow(QWidget* window)
    {
        if (window != nullptr)
        {
            NSView* view = (__bridge NSView*)reinterpret_cast<void*>(window->window()->winId());
            if (view.window != nil)
            {
                return view.window;
            }
        }
        return NSApp.keyWindow != nil ? NSApp.keyWindow : NSApp.mainWindow;
    }

    QByteArray toByteArray(NSData* data)
    {
        return data != nil ? QByteArray::fromNSData(data) : QByteArray();
    }

    API_AVAILABLE(macos(12.0)) ASAuthorizationPublicKeyCredentialUserVerificationPreference userVerificationPreference(ShadPasskeyType::UserVerification userVerification)
    {
        switch (userVerification)
        {
            case ShadPasskeyType::Required:
                return ASAuthorizationPublicKeyCredentialUserVerificationPreferenceRequired;
            case ShadPasskeyType::Discouraged:
                return ASAuthorizationPublicKeyCredentialUserVerificationPreferenceDiscouraged;
            case ShadPasskeyType::Preferred:
                break;
        }
        return ASAuthorizationPublicKeyCredentialUserVerificationPreferencePreferred;
    }

    void reportError(NSError* error, const ShadPasskeyBackend::ErrorCallback& onError)
    {
        const bool isAuthorizationError = [error.domain isEqualToString:ASAuthorizationErrorDomain];
        if (isAuthorizationError && error.code == ASAuthorizationErrorCanceled)
        {
            onError(ShadPasskeyType::Cancelled, QString());
            return;
        }
        const QString description = QString::fromNSString(error.localizedDescription);
        if (isAuthorizationError && error.code == ASAuthorizationErrorFailed && description.contains(QStringLiteral("not associated with domain")))
        {
            onError(ShadPasskeyType::DomainNotAllowed, description);
            return;
        }
        onError(ShadPasskeyType::Other, description);
    }
}

struct ShadPasskeyBackend::Impl
{
    ASAuthorizationController* controller{nil};
    ShadPasskeyDelegate* delegate{nil};

    void release()
    {
        [delegate invalidate];
        controller = nil;
        delegate = nil;
    }

    void perform(ASAuthorizationRequest* request, QWidget* window, ShadPasskeyHandler handler)
    {
        release();
        delegate = [[ShadPasskeyDelegate alloc] initWithAnchor:anchorWindow(window) handler:[this, handler](ASAuthorization* authorization, NSError* error)
        {
            controller = nil;
            delegate = nil;
            handler(authorization, error);
        }];
        controller = [[ASAuthorizationController alloc] initWithAuthorizationRequests:@[request]];
        controller.delegate = delegate;
        controller.presentationContextProvider = delegate;
        [controller performRequests];
    }
};

ShadPasskeyBackend::ShadPasskeyBackend()
    : _impl(std::make_unique<Impl>())
{
}

ShadPasskeyBackend::~ShadPasskeyBackend()
{
    cancel();
    _impl->release();
}

bool ShadPasskeyBackend::isSupported()
{
    if (@available(macOS 12.0, *))
    {
        return true;
    }
    return false;
}

void ShadPasskeyBackend::registerCredential(const ShadPasskeyRegistrationOptions& options, QWidget* window, const RegistrationCallback& onFinished, const ErrorCallback& onError)
{
    if (@available(macOS 12.0, *))
    {
        ASAuthorizationPlatformPublicKeyCredentialProvider* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc] initWithRelyingPartyIdentifier:options.rpId.toNSString()];
        ASAuthorizationPlatformPublicKeyCredentialRegistrationRequest* request = [provider createCredentialRegistrationRequestWithChallenge:options.challenge.toNSData() name:options.userName.toNSString() userID:options.userId.toNSData()];
        request.displayName = options.displayName.isEmpty() ? nil : options.displayName.toNSString();
        request.userVerificationPreference = userVerificationPreference(options.userVerification);
        _impl->perform(request, window, [onFinished, onError](ASAuthorization* authorization, NSError* error)
        {
            if (error != nil)
            {
                reportError(error, onError);
                return;
            }
            if (![authorization.credential isKindOfClass:[ASAuthorizationPlatformPublicKeyCredentialRegistration class]])
            {
                onError(ShadPasskeyType::Other, QString());
                return;
            }
            ASAuthorizationPlatformPublicKeyCredentialRegistration* registration = (ASAuthorizationPlatformPublicKeyCredentialRegistration*)authorization.credential;
            ShadPasskeyCredential credential;
            credential.credentialId = toByteArray(registration.credentialID);
            credential.clientDataJson = toByteArray(registration.rawClientDataJSON);
            credential.attestationObject = toByteArray(registration.rawAttestationObject);
            onFinished(credential);
        });
        return;
    }
    onError(ShadPasskeyType::NotSupported, QString());
}

void ShadPasskeyBackend::authenticate(const ShadPasskeyAuthenticationOptions& options, QWidget* window, const AuthenticationCallback& onFinished, const ErrorCallback& onError)
{
    if (@available(macOS 12.0, *))
    {
        ASAuthorizationPlatformPublicKeyCredentialProvider* provider = [[ASAuthorizationPlatformPublicKeyCredentialProvider alloc] initWithRelyingPartyIdentifier:options.rpId.toNSString()];
        ASAuthorizationPlatformPublicKeyCredentialAssertionRequest* request = [provider createCredentialAssertionRequestWithChallenge:options.challenge.toNSData()];
        request.userVerificationPreference = userVerificationPreference(options.userVerification);
        NSMutableArray<ASAuthorizationPlatformPublicKeyCredentialDescriptor*>* allowed = [NSMutableArray array];
        for (const QByteArray& credentialId : options.allowCredentials)
        {
            [allowed addObject:[[ASAuthorizationPlatformPublicKeyCredentialDescriptor alloc] initWithCredentialID:credentialId.toNSData()]];
        }
        request.allowedCredentials = allowed;
        _impl->perform(request, window, [onFinished, onError](ASAuthorization* authorization, NSError* error)
        {
            if (error != nil)
            {
                reportError(error, onError);
                return;
            }
            if (![authorization.credential isKindOfClass:[ASAuthorizationPlatformPublicKeyCredentialAssertion class]])
            {
                onError(ShadPasskeyType::Other, QString());
                return;
            }
            ASAuthorizationPlatformPublicKeyCredentialAssertion* credential = (ASAuthorizationPlatformPublicKeyCredentialAssertion*)authorization.credential;
            ShadPasskeyAssertion assertion;
            assertion.credentialId = toByteArray(credential.credentialID);
            assertion.clientDataJson = toByteArray(credential.rawClientDataJSON);
            assertion.authenticatorData = toByteArray(credential.rawAuthenticatorData);
            assertion.signature = toByteArray(credential.signature);
            assertion.userHandle = toByteArray(credential.userID);
            onFinished(assertion);
        });
        return;
    }
    onError(ShadPasskeyType::NotSupported, QString());
}

void ShadPasskeyBackend::cancel()
{
    if (_impl->controller == nil)
    {
        return;
    }
    if (@available(macOS 13.0, *))
    {
        [_impl->controller cancel];
    }
}
