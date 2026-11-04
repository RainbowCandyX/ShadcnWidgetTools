#include "ShadPasskeyButton.h"

#include <QEvent>
#include <QObject>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadPasskeyButtonPrivate.h"
#include "ShadStyleHelper.h"

ShadPasskeyButton::ShadPasskeyButton(QWidget* parent)
    : ShadButton(parent), _private(new ShadPasskeyButtonPrivate())
{
    ShadPasskeyButtonPrivate* d = _private.data();
    d->_passkey = new ShadPasskey(this);
    setVariant(ShadButtonType::Outline);
    setShadIcon(ShadIconType::KeyRound);
    setEnabled(ShadPasskey::isSupported());
    connect(d->_passkey, &ShadPasskey::busyChanged, this, &ShadButton::setLoading);
    connect(d->_passkey, &ShadPasskey::simulatedChanged, this, [this](bool simulated)
    {
        setEnabled(simulated || ShadPasskey::isSupported());
        retranslate();
    });
    connect(d->_passkey, &ShadPasskey::registrationFinished, this, &ShadPasskeyButton::registrationFinished);
    connect(d->_passkey, &ShadPasskey::authenticationFinished, this, &ShadPasskeyButton::authenticationFinished);
    connect(d->_passkey, &ShadPasskey::failed, this, &ShadPasskeyButton::failed);
    retranslate();
}

ShadPasskeyButton::~ShadPasskeyButton()
{
    ShadPasskeyButtonPrivate* d = _private.data();
    QObject::disconnect(d->_passkey, nullptr, this, nullptr);
}

ShadPasskey* ShadPasskeyButton::passkey() const
{
    return _private->_passkey;
}

void ShadPasskeyButton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    ShadButton::changeEvent(event);
}

void ShadPasskeyButton::retranslate()
{
    ShadStyleHelper::applyDefaultText(this, "text", text(), tr("使用通行密钥登录"), [this](const QString& text) { setText(text); });
    const QString toolTipText = ShadPasskey::isSupported() || _private->_passkey->isSimulated() ? QString() : tr("当前平台不支持通行密钥");
    ShadStyleHelper::applyDefaultText(this, "toolTip", toolTip(), toolTipText, [this](const QString& text) { setToolTip(text); });
}
