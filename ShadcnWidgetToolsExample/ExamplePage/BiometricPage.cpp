#include "BiometricPage.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadBiometric.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    enum class Status
    {
        Ready = 0,
        Busy,
        Succeeded,
        Failed,
    };

    struct PageState
    {
        Status status{Status::Ready};
        QString message;
    };

    ShadIconType::IconName methodIcon(ShadBiometricType::Method method)
    {
        switch (method)
        {
            case ShadBiometricType::Face:
                return ShadIconType::ScanFace;
            case ShadBiometricType::Password:
                return ShadIconType::LockKeyhole;
            case ShadBiometricType::Fingerprint:
            case ShadBiometricType::Unavailable:
                break;
        }
        return ShadIconType::Fingerprint;
    }

    void updateStatusText(ShadLabel* label, const PageState& state, ShadBiometricType::Method method)
    {
        switch (state.status)
        {
            case Status::Ready:
                label->setText(method == ShadBiometricType::Unavailable ? QCoreApplication::translate("BiometricPage", "状态: 没有可用的验证方式") : QCoreApplication::translate("BiometricPage", "状态: 就绪"));
                break;
            case Status::Busy:
                label->setText(QCoreApplication::translate("BiometricPage", "状态: 等待验证"));
                break;
            case Status::Succeeded:
                label->setText(QCoreApplication::translate("BiometricPage", "状态: 验证通过"));
                break;
            case Status::Failed:
                label->setText(QCoreApplication::translate("BiometricPage", "状态: 失败, %1").arg(state.message));
                break;
        }
    }
}

BiometricPage::BiometricPage(QWidget* parent)
    : ExampleBasePage([] { return tr("身份验证"); }, [] { return tr("ShadBiometric：调用 Touch ID 或 Windows Hello 在本机确认本人。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("验证通过后再执行敏感操作"); });
    ShadBiometric* biometric = new ShadBiometric(this);

    ShadSwitch* deviceOwnerSwitch = new ShadSwitch(this);
    ExampleI18n::bind(deviceOwnerSwitch, [deviceOwnerSwitch]() { deviceOwnerSwitch->setText(tr("允许改用密码或 PIN")); });
    addRow(basicCard, {deviceOwnerSwitch});

    ShadButton* verifyButton = new ShadButton(QString(), ShadButtonType::Default, this);
    ExampleI18n::bind(verifyButton, [verifyButton]() { verifyButton->setText(tr("验证身份")); });
    addRow(basicCard, {verifyButton});

    ShadLabel* statusLabel = new ShadLabel(this);
    statusLabel->setColorRole(ShadThemeType::MutedForeground);
    statusLabel->setWordWrap(true);
    addStretchItem(basicCard, statusLabel);

    const std::shared_ptr<PageState> state = std::make_shared<PageState>();
    const std::shared_ptr<ShadBiometricType::Method> method = std::make_shared<ShadBiometricType::Method>(biometric->availableMethod());
    const auto refresh = [biometric, verifyButton, statusLabel, state, method]()
    {
        *method = biometric->availableMethod();
        verifyButton->setShadIcon(methodIcon(*method));
        verifyButton->setEnabled(*method != ShadBiometricType::Unavailable);
        updateStatusText(statusLabel, *state, *method);
    };
    ExampleI18n::bind(statusLabel, [statusLabel, state, method]() { updateStatusText(statusLabel, *state, *method); });
    refresh();

    connect(deviceOwnerSwitch, &QAbstractButton::toggled, biometric, [biometric, state, refresh](bool checked)
    {
        biometric->setPolicy(checked ? ShadBiometricType::DeviceOwner : ShadBiometricType::Biometrics);
        state->status = Status::Ready;
        refresh();
    });
    connect(verifyButton, &QAbstractButton::clicked, this, [biometric]() { biometric->authenticate(tr("确认是你本人")); });
    connect(biometric, &ShadBiometric::busyChanged, this, [verifyButton, statusLabel, state, method](bool busy)
    {
        verifyButton->setLoading(busy);
        if (busy)
        {
            state->status = Status::Busy;
            updateStatusText(statusLabel, *state, *method);
        }
    });
    connect(biometric, &ShadBiometric::succeeded, this, [statusLabel, state, method]()
    {
        state->status = Status::Succeeded;
        updateStatusText(statusLabel, *state, *method);
    });
    connect(biometric, &ShadBiometric::failed, this, [state, refresh](ShadBiometricType::Error error, const QString& message)
    {
        Q_UNUSED(error);
        state->status = Status::Failed;
        state->message = message;
        refresh();
    });
}

BiometricPage::~BiometricPage()
{
}
