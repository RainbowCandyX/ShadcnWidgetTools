#include "EmptyPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QSizePolicy>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadAvatar.h"
#include "ShadAvatarGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadEmpty.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTable.h"

namespace
{
    // 扣除按钮两侧预留的焦点环边距
    constexpr int kButtonGap = 8 - 2 * ExampleLayout::kFocusRingWidth;

    // 固定宽度的按钮行，在操作区中水平居中
    QWidget* createButtonRow(const QList<QWidget*>& buttons, QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        row->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kButtonGap);
        for (QWidget* button : buttons)
        {
            layout->addWidget(button);
        }
        return row;
    }

    ShadButton* createButton(ShadButtonType::Variant variant, ShadIconType::IconName icon, ShadButtonType::Size size)
    {
        ShadButton* button = new ShadButton(QString(), variant);
        button->setShadIcon(icon);
        button->setButtonSize(size);
        return button;
    }
} // namespace

EmptyPage::EmptyPage(QWidget* parent)
    : ExampleBasePage([] { return tr("空状态"); }, [] { return tr("ShadEmpty：没有数据时显示的占位内容，包含媒体、标题、描述与操作区。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("Icon 媒体样式，操作区放置按钮"); });
    ShadEmpty* basicEmpty = new ShadEmpty(this);
    basicEmpty->setShadIcon(ShadIconType::FolderCode);
    ShadButton* createProjectButton = createButton(ShadButtonType::Default, ShadIconType::None, ShadButtonType::Medium);
    ShadButton* importProjectButton = createButton(ShadButtonType::Outline, ShadIconType::None, ShadButtonType::Medium);
    basicEmpty->contentLayout()->addWidget(createButtonRow({createProjectButton, importProjectButton}, basicEmpty));
    ShadButton* learnMoreButton = createButton(ShadButtonType::Link, ShadIconType::ArrowUpRight, ShadButtonType::Small);
    basicEmpty->contentLayout()->addWidget(learnMoreButton);
    addStretchItem(basicCard, basicEmpty);

    ShadCard* defaultCard = addSection([] { return tr("媒体样式"); }, [] { return tr("setMediaVariant(Default)：只显示图标，没有底色"); });
    ShadEmpty* defaultEmpty = new ShadEmpty(this);
    defaultEmpty->setMediaVariant(ShadEmptyType::Default);
    defaultEmpty->setShadIcon(ShadIconType::Inbox);
    addStretchItem(defaultCard, defaultEmpty);

    ShadCard* avatarCard = addSection([] { return tr("头像媒体"); }, [] { return tr("setMediaWidget：用头像组等任意组件替换图标"); });
    ShadEmpty* avatarEmpty = new ShadEmpty(this);
    ShadAvatarGroup* avatarGroup = new ShadAvatarGroup();
    avatarGroup->setAvatarSize(ShadAvatarType::Large);
    for (const QString& fallback : QStringList{QStringLiteral("CN"), QStringLiteral("LR"), QStringLiteral("ER")})
    {
        avatarGroup->addAvatar(new ShadAvatar(fallback, avatarGroup));
    }
    avatarEmpty->setMediaWidget(avatarGroup);
    ShadButton* inviteButton = createButton(ShadButtonType::Default, ShadIconType::Plus, ShadButtonType::Small);
    avatarEmpty->contentLayout()->addWidget(inviteButton);
    addStretchItem(avatarCard, avatarEmpty);

    ShadCard* borderCard = addSection([] { return tr("虚线边框"); }, [] { return tr("setBorderVisible：显示虚线边框，用于标出空白区域"); });
    ShadEmpty* borderEmpty = new ShadEmpty(this);
    borderEmpty->setShadIcon(ShadIconType::CloudOff);
    borderEmpty->setBorderVisible(true);
    ShadButton* uploadButton = createButton(ShadButtonType::Outline, ShadIconType::None, ShadButtonType::Small);
    borderEmpty->contentLayout()->addWidget(uploadButton);
    addStretchItem(borderCard, borderEmpty);

    ShadCard* linkCard = addSection([] { return tr("描述中的链接"); }, [] { return tr("描述支持 <a> 链接，悬停时变色，点击时发出 linkActivated"); });
    ShadEmpty* linkEmpty = new ShadEmpty(this);
    linkEmpty->setShadIcon(ShadIconType::SearchX);
    addStretchItem(linkCard, linkEmpty);
    ShadLabel* linkResultLabel = new ShadLabel(this);
    linkResultLabel->setColorRole(ShadThemeType::MutedForeground);
    addItem(linkCard, linkResultLabel);
    // 记录最近点击的链接，切换语言时重新生成提示
    std::shared_ptr<QString> lastLink = std::make_shared<QString>();
    auto updateLinkResult = [linkResultLabel, lastLink]()
    {
        linkResultLabel->setText(lastLink->isEmpty() ? tr("尚未点击链接") : tr("点击了链接：%1").arg(*lastLink));
    };
    connect(linkEmpty, &ShadEmpty::linkActivated, this, [lastLink, updateLinkResult](const QString& link) { *lastLink = link; updateLinkResult(); });

    ShadCard* tableCard = addSection([] { return tr("表格中的空数据"); }, [] { return tr("表格没有数据行时，在表头下方显示空状态"); });
    QWidget* tableContainer = new QWidget(this);
    QVBoxLayout* tableLayout = new QVBoxLayout(tableContainer);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);
    QStandardItemModel* invoiceModel = new QStandardItemModel(0, 4, tableContainer);
    ShadTable* invoiceTable = new ShadTable(tableContainer);
    invoiceTable->setModel(invoiceModel);
    tableLayout->addWidget(invoiceTable);
    ShadEmpty* tableEmpty = new ShadEmpty(tableContainer);
    tableEmpty->setMediaVariant(ShadEmptyType::Default);
    tableEmpty->setShadIcon(ShadIconType::FileX);
    tableLayout->addWidget(tableEmpty);
    addStretchItem(tableCard, tableContainer);

    ExampleI18n::bind(this, [basicEmpty, createProjectButton, importProjectButton, learnMoreButton, defaultEmpty, avatarEmpty, inviteButton, borderEmpty, uploadButton, linkEmpty, invoiceModel, tableEmpty, updateLinkResult]()
    {
        basicEmpty->setTitle(tr("暂无项目"));
        basicEmpty->setDescription(tr("你还没有创建任何项目。创建第一个项目即可开始使用。"));
        createProjectButton->setText(tr("创建项目"));
        importProjectButton->setText(tr("导入项目"));
        learnMoreButton->setText(tr("了解更多"));
        defaultEmpty->setTitle(tr("暂无消息"));
        defaultEmpty->setDescription(tr("收到的新消息会显示在这里。"));
        avatarEmpty->setTitle(tr("暂无团队成员"));
        avatarEmpty->setDescription(tr("邀请团队成员一起协作这个项目。"));
        inviteButton->setText(tr("邀请成员"));
        borderEmpty->setTitle(tr("云存储为空"));
        borderEmpty->setDescription(tr("上传文件到云存储后，即可在任何设备上访问。"));
        uploadButton->setText(tr("上传文件"));
        linkEmpty->setTitle(tr("未找到结果"));
        linkEmpty->setDescription(tr("没有符合条件的结果。试试调整筛选条件，或<a href=\"#help\">查看帮助</a>。"));
        invoiceModel->setHorizontalHeaderLabels({tr("发票"), tr("状态"), tr("付款方式"), tr("金额")});
        tableEmpty->setTitle(tr("暂无发票"));
        tableEmpty->setDescription(tr("创建发票后会显示在这里。"));
        updateLinkResult();
    });
}

EmptyPage::~EmptyPage()
{
}
