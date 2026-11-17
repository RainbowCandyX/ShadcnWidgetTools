#include "TransferPage.h"

#include <QAbstractButton>
#include <QList>
#include <QString>
#include <QStringList>

#include <utility>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"
#include "ShadTransfer.h"

namespace
{
    constexpr int kTransferWidth = 560;
    constexpr int kTransferHeight = 320;

    QList<ShadTransferItem> memberItems()
    {
        QList<ShadTransferItem> items;
        for (int index = 1; index <= 20; ++index)
        {
            ShadTransferItem item;
            item.key = QString::number(index);
            item.title = TransferPage::tr("成员 %1").arg(index);
            item.description = QStringLiteral("member%1@example.com").arg(index);
            item.isDisabled = index % 3 == 0;
            items.append(item);
        }
        return items;
    }

    QList<ShadTransferItem> permissionItems()
    {
        const QList<std::pair<ShadIconType::IconName, QString>> permissions = {
            {ShadIconType::Eye, TransferPage::tr("查看项目")},
            {ShadIconType::Pencil, TransferPage::tr("编辑代码")},
            {ShadIconType::GitMerge, TransferPage::tr("合并请求")},
            {ShadIconType::Rocket, TransferPage::tr("发布版本")},
            {ShadIconType::Users, TransferPage::tr("管理成员")},
            {ShadIconType::Settings, TransferPage::tr("修改设置")},
            {ShadIconType::Trash2, TransferPage::tr("删除项目")},
        };
        QList<ShadTransferItem> items;
        for (int index = 0; index < int(permissions.size()); ++index)
        {
            ShadTransferItem item;
            item.key = QString::number(index);
            item.icon = permissions.at(index).first;
            item.title = permissions.at(index).second;
            items.append(item);
        }
        return items;
    }
} // namespace

TransferPage::TransferPage(QWidget* parent)
    : ExampleBasePage([] { return tr("穿梭框"); }, [] { return tr("ShadTransfer：在两个列表之间移动勾选的项，常用于成员与权限分配。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击整行或按空格勾选，点击中间的按钮移动；禁用的项不能勾选"); });
    ShadTransfer* memberTransfer = new ShadTransfer(this);
    memberTransfer->setFixedSize(kTransferWidth, kTransferHeight);
    memberTransfer->setTargetKeys({QStringLiteral("2"), QStringLiteral("5")});
    ExampleI18n::bind(memberTransfer, [memberTransfer]()
    {
        const QStringList targetKeys = memberTransfer->getTargetKeys();
        memberTransfer->setItems(memberItems());
        memberTransfer->setTargetKeys(targetKeys);
        memberTransfer->setTitles(tr("全部成员"), tr("项目成员"));
    });
    ShadLabel* memberLabel = new ShadLabel(this);
    memberLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateMemberLabel = [memberLabel, memberTransfer]() { memberLabel->setText(tr("项目成员: %1 人").arg(memberTransfer->getTargetKeys().size())); };
    connect(memberTransfer, &ShadTransfer::targetKeysChanged, memberLabel, updateMemberLabel);
    ExampleI18n::bind(memberLabel, updateMemberLabel);
    addItem(basicCard, memberTransfer);
    addItem(basicCard, memberLabel);

    ShadCard* searchCard = addSection([] { return tr("搜索"); }, [] { return tr("setSearchEnabled(true) 在列表顶部显示搜索框，按标题过滤，全选只作用于过滤后的项"); });
    ShadTransfer* permissionTransfer = new ShadTransfer(this);
    permissionTransfer->setFixedSize(kTransferWidth, kTransferHeight);
    permissionTransfer->setSearchEnabled(true);
    permissionTransfer->setTargetKeys({QStringLiteral("0")});
    ExampleI18n::bind(permissionTransfer, [permissionTransfer]()
    {
        const QStringList targetKeys = permissionTransfer->getTargetKeys();
        permissionTransfer->setItems(permissionItems());
        permissionTransfer->setTargetKeys(targetKeys);
    });
    addItem(searchCard, permissionTransfer);

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("setEnabled(false) 禁用整个穿梭框"); });
    ShadSwitch* enabledSwitch = new ShadSwitch(this);
    enabledSwitch->setChecked(true);
    ExampleI18n::bind(enabledSwitch, [enabledSwitch]() { enabledSwitch->setText(tr("可用")); });
    ShadTransfer* disabledTransfer = new ShadTransfer(this);
    disabledTransfer->setFixedSize(kTransferWidth, kTransferHeight);
    ExampleI18n::bind(disabledTransfer, [disabledTransfer]()
    {
        const QStringList targetKeys = disabledTransfer->getTargetKeys();
        disabledTransfer->setItems(permissionItems());
        disabledTransfer->setTargetKeys(targetKeys);
    });
    connect(enabledSwitch, &QAbstractButton::toggled, disabledTransfer, &ShadTransfer::setEnabled);
    addItem(disabledCard, enabledSwitch);
    addItem(disabledCard, disabledTransfer);
}

TransferPage::~TransferPage()
{
}
