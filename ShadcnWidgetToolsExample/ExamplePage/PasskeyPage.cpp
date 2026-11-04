#include "PasskeyPage.h"

#include <QAbstractButton>
#include <QByteArray>
#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QRandomGenerator>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDescriptions.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPasskey.h"
#include "ShadPasskeyButton.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kChallengeSize = 32;
    constexpr int kUserIdSize = 16;

    enum class Status
    {
        Ready = 0,
        Busy,
        Registered,
        Authenticated,
        Failed,
    };

    struct PageState
    {
        Status status{Status::Ready};
        QString message;
    };

    QByteArray randomBytes(int size)
    {
        QByteArray data(size, Qt::Uninitialized);
        QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(data.data()), size / static_cast<int>(sizeof(quint32)));
        return data;
    }

    void updateStatusText(ShadLabel* label, const PageState& state)
    {
        switch (state.status)
        {
            case Status::Ready:
                label->setText(QCoreApplication::translate("PasskeyPage", "状态: 就绪"));
                break;
            case Status::Busy:
                label->setText(QCoreApplication::translate("PasskeyPage", "状态: 进行中"));
                break;
            case Status::Registered:
                label->setText(QCoreApplication::translate("PasskeyPage", "状态: 注册成功"));
                break;
            case Status::Authenticated:
                label->setText(QCoreApplication::translate("PasskeyPage", "状态: 登录成功"));
                break;
            case Status::Failed:
                label->setText(QCoreApplication::translate("PasskeyPage", "状态: 失败, %1").arg(state.message));
                break;
        }
    }

    void setDataValues(ShadDescriptions* descriptions, const QList<QByteArray>& values)
    {
        for (int i = 0; i < descriptions->count(); ++i)
        {
            const QByteArray value = values.value(i);
            descriptions->setItemText(i, value.isEmpty() ? QStringLiteral("-") : ShadPasskey::toBase64Url(value));
        }
    }
}

PasskeyPage::PasskeyPage(QWidget* parent)
    : ExampleBasePage([] { return tr("通行密钥"); }, [] { return tr("ShadPasskey：调用系统通行密钥完成注册与登录，签名交给服务器验证。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("调起系统通行密钥对话框"); });
    ShadInput* rpIdInput = new ShadInput(this);
    rpIdInput->setText(QStringLiteral("example.com"));
    rpIdInput->setFixedWidth(ExampleLayout::kFieldWidth);
    ShadInput* userNameInput = new ShadInput(this);
    userNameInput->setText(QStringLiteral("demo@example.com"));
    userNameInput->setFixedWidth(ExampleLayout::kFieldWidth);
    ExampleI18n::bind(rpIdInput, [rpIdInput, userNameInput]()
    {
        rpIdInput->setPlaceholderText(tr("域名"));
        userNameInput->setPlaceholderText(tr("用户名"));
    });
    addRow(basicCard, {rpIdInput, userNameInput});

    ShadSwitch* simulationSwitch = new ShadSwitch(this);
    ExampleI18n::bind(simulationSwitch, [simulationSwitch]() { simulationSwitch->setText(tr("模拟模式，不调用系统通行密钥")); });
    addRow(basicCard, {simulationSwitch});

    ShadButton* registerButton = new ShadButton(QString(), ShadButtonType::Default, this);
    registerButton->setShadIcon(ShadIconType::Fingerprint);
    ExampleI18n::bind(registerButton, [registerButton]() { registerButton->setText(tr("注册通行密钥")); });
    ShadPasskeyButton* passkeyButton = new ShadPasskeyButton(this);
    addRow(basicCard, {registerButton, passkeyButton});
    ShadPasskey* passkey = passkeyButton->passkey();
    const auto updateRegisterButton = [registerButton, passkey]() { registerButton->setEnabled(!passkey->isBusy() && (passkey->isSimulated() || ShadPasskey::isSupported())); };
    connect(simulationSwitch, &QAbstractButton::toggled, passkey, &ShadPasskey::setSimulated);
    connect(passkey, &ShadPasskey::simulatedChanged, this, updateRegisterButton);
    simulationSwitch->setChecked(true);
    updateRegisterButton();

    ShadLabel* statusLabel = new ShadLabel(this);
    statusLabel->setColorRole(ShadThemeType::MutedForeground);
    statusLabel->setWordWrap(true);
    addStretchItem(basicCard, statusLabel);

    ShadCard* dataCard = addSection([] { return tr("返回数据"); }, [] { return tr("发送给服务器验证的字段，base64url 编码"); });
    ShadDescriptions* descriptions = new ShadDescriptions(this);
    descriptions->setBordered(true);
    descriptions->setColumnCount(1);
    const QList<ExampleI18n::TextFunction> labels = {[] { return tr("凭据 ID"); }, [] { return QStringLiteral("clientDataJSON"); }, [] { return QStringLiteral("authenticatorData"); }, [] { return tr("签名"); }, [] { return tr("用户 ID"); }, [] { return QStringLiteral("attestationObject"); }};
    for (int i = 0; i < labels.size(); ++i)
    {
        descriptions->addItem(QString(), QStringLiteral("-"));
    }
    ExampleI18n::bind(descriptions, [descriptions, labels]()
    {
        for (int i = 0; i < labels.size(); ++i)
        {
            descriptions->setItemLabel(i, labels.at(i)());
        }
    });
    addStretchItem(dataCard, descriptions);

    const std::shared_ptr<PageState> state = std::make_shared<PageState>();
    ExampleI18n::bind(statusLabel, [statusLabel, state]() { updateStatusText(statusLabel, *state); });

    connect(passkey, &ShadPasskey::busyChanged, this, [updateRegisterButton, statusLabel, state](bool busy)
    {
        updateRegisterButton();
        if (busy)
        {
            state->status = Status::Busy;
            updateStatusText(statusLabel, *state);
        }
    });
    connect(registerButton, &QAbstractButton::clicked, this, [passkey, rpIdInput, userNameInput, descriptions]()
    {
        ShadPasskeyRegistrationOptions options;
        options.rpId = rpIdInput->text().trimmed();
        options.rpName = options.rpId;
        options.challenge = randomBytes(kChallengeSize);
        options.userId = randomBytes(kUserIdSize);
        options.userName = userNameInput->text().trimmed();
        setDataValues(descriptions, {});
        passkey->registerCredential(options);
    });
    connect(passkeyButton, &QAbstractButton::clicked, this, [passkey, rpIdInput, descriptions]()
    {
        ShadPasskeyAuthenticationOptions options;
        options.rpId = rpIdInput->text().trimmed();
        options.challenge = randomBytes(kChallengeSize);
        setDataValues(descriptions, {});
        passkey->authenticate(options);
    });
    connect(passkeyButton, &ShadPasskeyButton::registrationFinished, this, [descriptions, statusLabel, state](const ShadPasskeyCredential& credential)
    {
        state->status = Status::Registered;
        updateStatusText(statusLabel, *state);
        setDataValues(descriptions, {credential.credentialId, credential.clientDataJson, QByteArray(), QByteArray(), QByteArray(), credential.attestationObject});
    });
    connect(passkeyButton, &ShadPasskeyButton::authenticationFinished, this, [descriptions, statusLabel, state](const ShadPasskeyAssertion& assertion)
    {
        state->status = Status::Authenticated;
        updateStatusText(statusLabel, *state);
        setDataValues(descriptions, {assertion.credentialId, assertion.clientDataJson, assertion.authenticatorData, assertion.signature, assertion.userHandle, QByteArray()});
    });
    connect(passkeyButton, &ShadPasskeyButton::failed, this, [statusLabel, state](ShadPasskeyType::Error error, const QString& message)
    {
        Q_UNUSED(error);
        state->status = Status::Failed;
        state->message = message;
        updateStatusText(statusLabel, *state);
    });
}

PasskeyPage::~PasskeyPage()
{
}
