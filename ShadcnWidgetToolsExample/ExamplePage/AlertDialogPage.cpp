#include "AlertDialogPage.h"

#include <QCoreApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadAlertDialog.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadIconType.h"
#include "ShadLabel.h"

namespace
{
    // 结果标签显示的状态
    enum class ResultState
    {
        Initial,
        Accepted,
        Rejected
    };

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 标题与描述随语言刷新，对话框打开期间切换语言同样生效
    ShadAlertDialog* createAlertDialog(const ExampleI18n::TextFunction& title, const ExampleI18n::TextFunction& description, QWidget* parent)
    {
        ShadAlertDialog* dialog = new ShadAlertDialog(parent);
        ExampleI18n::bind(dialog, [dialog, title, description]()
        {
            dialog->setTitle(title());
            dialog->setDescription(description());
        });
        return dialog;
    }

    // 基础用法
    ShadAlertDialog* createBasicDialog(QWidget* parent)
    {
        return createAlertDialog([] { return QCoreApplication::translate("AlertDialogPage", "确定要继续吗？"); }, []
        {
            return QCoreApplication::translate("AlertDialogPage", "此操作无法撤销，你的账户将被永久删除，数据也会从我们的服务器上移除。");
        }, parent);
    }

    // 小尺寸
    ShadAlertDialog* createSmallDialog(QWidget* parent)
    {
        ShadAlertDialog* dialog = createAlertDialog([] { return QCoreApplication::translate("AlertDialogPage", "允许配件连接？"); }, []
        {
            return QCoreApplication::translate("AlertDialogPage", "是否允许 USB 配件连接到这台设备？");
        }, parent);
        dialog->setSize(ShadAlertDialogType::Small);
        ExampleI18n::bind(dialog, [dialog]()
        {
            dialog->cancelButton()->setText(QCoreApplication::translate("AlertDialogPage", "不允许"));
            dialog->actionButton()->setText(QCoreApplication::translate("AlertDialogPage", "允许"));
        });
        return dialog;
    }

    // 带媒体
    ShadAlertDialog* createMediaDialog(QWidget* parent)
    {
        ShadAlertDialog* dialog = createAlertDialog([] { return QCoreApplication::translate("AlertDialogPage", "确定要继续吗？"); }, []
        {
            return QCoreApplication::translate("AlertDialogPage", "你的账户将被永久删除，数据也会从我们的服务器上移除。");
        }, parent);
        dialog->setMediaIcon(ShadIconType::Bluetooth);
        return dialog;
    }

    // 小尺寸带媒体
    ShadAlertDialog* createSmallMediaDialog(QWidget* parent)
    {
        ShadAlertDialog* dialog = createSmallDialog(parent);
        dialog->setMediaIcon(ShadIconType::Bluetooth);
        return dialog;
    }

    // 危险操作，exec 返回后由调用方删除
    ShadAlertDialog* createDeleteDialog(QWidget* parent)
    {
        ShadAlertDialog* dialog = createAlertDialog([] { return QCoreApplication::translate("AlertDialogPage", "删除聊天？"); }, []
        {
            return QCoreApplication::translate("AlertDialogPage", "这会永久删除这段聊天记录，聊天中保存的记忆可以在设置中删除。");
        }, parent);
        dialog->setSize(ShadAlertDialogType::Small);
        dialog->setMediaIcon(ShadIconType::Trash2);
        dialog->setMediaVariant(ShadAlertDialogType::Destructive);
        dialog->cancelButton()->setVariant(ShadButtonType::Ghost);
        ExampleI18n::bind(dialog, [dialog]()
        {
            dialog->actionButton()->setText(QCoreApplication::translate("AlertDialogPage", "删除"));
        });
        dialog->actionButton()->setVariant(ShadButtonType::Destructive);
        return dialog;
    }

    // 在对话框中打开
    ShadDialog* createHostDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        ShadButton* openButton = new ShadButton(dialog);
        ExampleI18n::bind(dialog, [dialog, openButton]()
        {
            dialog->setTitle(QCoreApplication::translate("AlertDialogPage", "警告对话框示例"));
            dialog->setDescription(QCoreApplication::translate("AlertDialogPage", "点击下面的按钮打开警告对话框。"));
            openButton->setText(QCoreApplication::translate("AlertDialogPage", "打开警告对话框"));
        });
        dialog->footerLayout()->addWidget(openButton);
        QObject::connect(openButton, &ShadButton::clicked, dialog, [dialog]()
        {
            ShadAlertDialog* nestedDialog = createBasicDialog(dialog);
            nestedDialog->setSize(ShadAlertDialogType::Small);
            ExampleLayout::openTransientDialog(nestedDialog);
        });
        return dialog;
    }
} // namespace

AlertDialogPage::AlertDialogPage(QWidget* parent)
    : ExampleBasePage([] { return tr("警告对话框"); }, [] { return tr("ShadAlertDialog：打断用户并要求回应的模态对话框，内置取消与确认按钮。"); }, parent)
{
    // 基础用法
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("标题与描述，页脚带取消与继续按钮"); });
    ShadButton* basicButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(basicButton, &ShadButton::clicked, this, [this]()
    {
        ExampleLayout::openTransientDialog(createBasicDialog(this));
    });
    addItem(basicCard, basicButton);

    ShadCard* smallCard = addSection([] { return tr("小尺寸"); }, [] { return tr("setSize(Small)：更窄，内容居中，按钮等宽并排"); });
    ShadButton* smallButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(smallButton, &ShadButton::clicked, this, [this]()
    {
        ExampleLayout::openTransientDialog(createSmallDialog(this));
    });
    addItem(smallCard, smallButton);

    ShadCard* mediaCard = addSection([] { return tr("带媒体"); }, [] { return tr("setMediaIcon：在标题区显示媒体图标"); });
    ShadButton* mediaButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(mediaButton, &ShadButton::clicked, this, [this]()
    {
        ExampleLayout::openTransientDialog(createMediaDialog(this));
    });
    addItem(mediaCard, mediaButton);

    ShadCard* smallMediaCard = addSection([] { return tr("小尺寸带媒体"); }, [] { return tr("小尺寸下媒体区、标题与描述上下排列并居中"); });
    ShadButton* smallMediaButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(smallMediaButton, &ShadButton::clicked, this, [this]()
    {
        ExampleLayout::openTransientDialog(createSmallMediaDialog(this));
    });
    addItem(smallMediaCard, smallMediaButton);

    ShadCard* destructiveCard = addSection([] { return tr("危险操作"); }, [] { return tr("媒体区与确认按钮使用危险配色"); });
    ShadButton* deleteButton = new ShadButton(QString(), ShadButtonType::Destructive, this);
    ShadLabel* deleteResult = createMutedText(this);
    const std::shared_ptr<ResultState> deleteState = std::make_shared<ResultState>(ResultState::Initial);
    const auto updateDeleteResult = [deleteResult, deleteState]()
    {
        switch (*deleteState)
        {
            case ResultState::Accepted:
                deleteResult->setText(tr("已删除聊天"));
                break;
            case ResultState::Rejected:
                deleteResult->setText(tr("已取消"));
                break;
            default:
                deleteResult->setText(tr("聊天仍然保留"));
                break;
        }
    };
    connect(deleteButton, &ShadButton::clicked, this, [this, deleteState, updateDeleteResult]()
    {
        QPointer<ShadAlertDialog> dialog = createDeleteDialog(this);
        const bool isAccepted = dialog->exec() == QDialog::Accepted;
        // 对话框打开期间页面被销毁时，对话框已随页面删除
        if (dialog.isNull())
        {
            return;
        }
        delete dialog.data();
        *deleteState = isAccepted ? ResultState::Accepted : ResultState::Rejected;
        updateDeleteResult();
    });
    addRow(destructiveCard, {deleteButton, deleteResult});

    ShadCard* nestedCard = addSection([] { return tr("在对话框中打开"); }, [] { return tr("在另一个对话框之上打开警告对话框"); });
    ShadButton* nestedButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(nestedButton, &ShadButton::clicked, this, [this]()
    {
        ExampleLayout::openTransientDialog(createHostDialog(this));
    });
    addItem(nestedCard, nestedButton);

    // 静态函数
    ShadCard* staticCard = addSection([] { return tr("静态函数"); }, []
    {
        return tr("ShadAlertDialog::confirm：一行代码打开并返回确认结果");
    });
    ShadButton* logoutButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadLabel* logoutResult = createMutedText(this);
    const std::shared_ptr<ResultState> logoutState = std::make_shared<ResultState>(ResultState::Initial);
    const auto updateLogoutResult = [logoutResult, logoutState]()
    {
        switch (*logoutState)
        {
            case ResultState::Accepted:
                logoutResult->setText(tr("confirm 返回 true：已退出登录"));
                break;
            case ResultState::Rejected:
                logoutResult->setText(tr("confirm 返回 false：已取消"));
                break;
            default:
                logoutResult->setText(tr("仍然保持登录"));
                break;
        }
    };
    connect(logoutButton, &ShadButton::clicked, this, [this, logoutState, updateLogoutResult]()
    {
        // confirm 内部创建对话框，打开期间切换语言不会刷新其文字
        const bool isConfirmed = ShadAlertDialog::confirm(this, tr("退出登录？"), tr("退出后需要重新输入账号和密码。"), tr("退出"));
        *logoutState = isConfirmed ? ResultState::Accepted : ResultState::Rejected;
        updateLogoutResult();
    });
    addRow(staticCard, {logoutButton, logoutResult});

    ExampleI18n::bind(this, [basicButton, smallButton, mediaButton, smallMediaButton, deleteButton, nestedButton, logoutButton, updateDeleteResult, updateLogoutResult]()
    {
        basicButton->setText(tr("默认"));
        smallButton->setText(tr("小尺寸"));
        mediaButton->setText(tr("默认（媒体）"));
        smallMediaButton->setText(tr("小尺寸（媒体）"));
        deleteButton->setText(tr("删除聊天"));
        nestedButton->setText(tr("打开对话框"));
        logoutButton->setText(tr("退出登录"));
        updateDeleteResult();
        updateLogoutResult();
    });
}

AlertDialogPage::~AlertDialogPage()
{
}
