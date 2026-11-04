#include "ItemPage.h"

#include <QColor>
#include <QCoreApplication>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>
#include <memory>
#include <utility>

#include "ExampleI18n.h"
#include "ShadAvatar.h"
#include "ShadAvatarGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadItem.h"
#include "ShadItemGroup.h"
#include "ShadLabel.h"

namespace
{
    // 生成图片的边长，媒体区按屏幕缩放比例从中缩小
    constexpr int kImageSize = 80;

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    ShadItemGroup* createGroup(QWidget* parent)
    {
        return new ShadItemGroup(parent);
    }

    // 添加到分组末尾的列表项；与基类的 addItem 区分命名。description 为空函数时不显示描述
    ShadItem* appendItem(ShadItemGroup* group, ShadItemType::Variant variant, ShadItemType::Size size, ExampleI18n::TextFunction title, ExampleI18n::TextFunction description)
    {
        ShadItem* item = new ShadItem(group);
        item->setVariant(variant);
        item->setSize(size);
        ExampleI18n::bind(item, [item, title = std::move(title), description = std::move(description)]()
        {
            item->setTitle(title());
            item->setDescription(description ? description() : QString());
        });
        group->addItem(item);
        return item;
    }

    // 操作区按钮，对应示例中的 size="sm"
    ShadButton* createActionButton(ExampleI18n::TextFunction text, ShadButtonType::Variant variant, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), variant, parent);
        button->setButtonSize(ShadButtonType::Small);
        ExampleI18n::bind(button, [button, text = std::move(text)]() { button->setText(text()); });
        return button;
    }

    // 示例用的图片：没有使用网络图片，按两种颜色生成渐变
    QPixmap createImage(const QColor& from, const QColor& to)
    {
        QPixmap image(kImageSize, kImageSize);
        QPainter painter(&image);
        QLinearGradient gradient(QPointF(0, 0), QPointF(kImageSize, kImageSize));
        gradient.setColorAt(0.0, from);
        gradient.setColorAt(1.0, to);
        painter.fillRect(image.rect(), gradient);
        return image;
    }

    // 一种样式的常见组合：仅标题、标题 + 描述、标题 + 描述 + 按钮、图标 + 标题 + 描述 + 按钮、多个操作
    ShadItemGroup* createVariantGroup(ShadItemType::Variant variant, QWidget* parent)
    {
        ShadItemGroup* group = createGroup(parent);
        appendItem(group, variant, ShadItemType::Medium, [] { return QCoreApplication::translate("ItemPage", "仅标题"); }, nullptr);
        appendItem(group, variant, ShadItemType::Medium, [] { return QCoreApplication::translate("ItemPage", "标题 + 描述"); }, [] { return QCoreApplication::translate("ItemPage", "描述文字用于补充更多上下文信息。"); });
        ShadItem* buttonItem = appendItem(group, variant, ShadItemType::Medium, [] { return QCoreApplication::translate("ItemPage", "标题 + 描述 + 按钮"); }, [] { return QCoreApplication::translate("ItemPage", "这一项包含标题、描述和一个操作按钮。"); });
        ShadButton* buttonItemAction = new ShadButton(QString(), ShadButtonType::Outline, buttonItem);
        ExampleI18n::bind(buttonItemAction, [buttonItemAction]() { buttonItemAction->setText(QCoreApplication::translate("ItemPage", "操作")); });
        buttonItem->addActionWidget(buttonItemAction);
        ShadItem* mediaItem = appendItem(group, variant, ShadItemType::Medium, [] { return QCoreApplication::translate("ItemPage", "图标 + 标题 + 描述 + 按钮"); }, [] { return QCoreApplication::translate("ItemPage", "完整的列表项：媒体、标题、描述和操作按钮。描述最多显示两行，超出部分在第二行末尾省略。"); });
        mediaItem->setMediaIcon(ShadIconType::Inbox);
        mediaItem->addActionWidget(createActionButton([] { return QCoreApplication::translate("ItemPage", "操作"); }, ShadButtonType::Default, mediaItem));
        ShadItem* actionsItem = appendItem(group, variant, ShadItemType::Medium, [] { return QCoreApplication::translate("ItemPage", "多个操作"); }, [] { return QCoreApplication::translate("ItemPage", "操作区可以放多个按钮。"); });
        actionsItem->addActionWidget(createActionButton([] { return QCoreApplication::translate("ItemPage", "取消"); }, ShadButtonType::Outline, actionsItem));
        actionsItem->addActionWidget(createActionButton([] { return QCoreApplication::translate("ItemPage", "确认"); }, ShadButtonType::Default, actionsItem));
        return group;
    }
} // namespace

ItemPage::ItemPage(QWidget* parent)
    : ExampleBasePage([] { return tr("列表项"); }, [] { return tr("ShadItem / ShadItemGroup：由媒体区、标题与描述、操作区组成的列表项，可分组排列。"); }, parent)
{
    // 三种样式
    ShadCard* defaultCard = addSection([] { return tr("默认样式"); }, [] { return tr("没有边框与背景，带图标与操作按钮"); });
    addStretchItem(defaultCard, createVariantGroup(ShadItemType::Default, this));

    ShadCard* outlineCard = addSection([] { return tr("描边样式"); }, [] { return tr("Outline 变体，带边框"); });
    addStretchItem(outlineCard, createVariantGroup(ShadItemType::Outline, this));

    ShadCard* mutedCard = addSection([] { return tr("柔和样式"); }, [] { return tr("Muted 变体，带柔和背景"); });
    addStretchItem(mutedCard, createVariantGroup(ShadItemType::Muted, this));

    // 三种尺寸
    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("三种尺寸，内边距与间距逐级缩小"); });
    ShadItemGroup* sizeGroup = createGroup(this);
    struct SizeDemo
    {
        ShadItemType::Size size;
        ExampleI18n::TextFunction title;
    };
    const QList<SizeDemo> sizes{{ShadItemType::Medium, [] { return tr("默认尺寸"); }}, {ShadItemType::Small, [] { return tr("小尺寸"); }}, {ShadItemType::ExtraSmall, [] { return tr("超小尺寸"); }}};
    for (const SizeDemo& size : sizes)
    {
        ShadItem* item = appendItem(sizeGroup, ShadItemType::Outline, size.size, size.title, [] { return tr("图标、标题、描述和操作按钮。"); });
        item->setMediaIcon(ShadIconType::Inbox);
        item->addActionWidget(createActionButton([] { return tr("操作"); }, ShadButtonType::Outline, item));
    }
    addStretchItem(sizeCard, sizeGroup);

    // 图片媒体
    ShadCard* imageCard = addSection([] { return tr("图片媒体"); }, [] { return tr("媒体区显示圆角正方形图片"); });
    ShadItemGroup* imageGroup = createGroup(this);
    ShadItem* projectItem = appendItem(imageGroup, ShadItemType::Outline, ShadItemType::Medium, [] { return tr("项目面板"); }, [] { return tr("项目设置与配置概览。"); });
    projectItem->setMediaPixmap(createImage(QColor(0x60, 0xa5, 0xfa), QColor(0x1e, 0x3a, 0x8a)));
    ShadItem* documentItem = appendItem(imageGroup, ShadItemType::Outline, ShadItemType::Small, [] { return tr("文档"); }, [] { return tr("显示元数据的文档。"); });
    documentItem->setMediaPixmap(createImage(QColor(0xf9, 0xa8, 0xd4), QColor(0x9d, 0x17, 0x4d)));
    documentItem->addActionWidget(createActionButton([] { return tr("查看"); }, ShadButtonType::Outline, documentItem));
    ShadItem* fileItem = appendItem(imageGroup, ShadItemType::Outline, ShadItemType::ExtraSmall, [] { return tr("附件"); }, [] { return tr("包含图片、标题、描述和下载按钮。"); });
    fileItem->setMediaPixmap(createImage(QColor(0x86, 0xef, 0xac), QColor(0x16, 0x65, 0x34)));
    fileItem->addActionWidget(createActionButton([] { return tr("下载"); }, ShadButtonType::Default, fileItem));
    addStretchItem(imageCard, imageGroup);

    // 头像媒体
    ShadCard* avatarCard = addSection([] { return tr("头像媒体"); }, [] { return tr("媒体区放入任意组件，例如头像或头像组"); });
    ShadItemGroup* avatarGroup = createGroup(this);
    ShadItem* memberItem = appendItem(avatarGroup, ShadItemType::Outline, ShadItemType::Medium, [] { return tr("陈娜"); }, [] { return tr("设计负责人，2 小时前活跃"); });
    memberItem->setMediaWidget(new ShadAvatar(QStringLiteral("CN"), memberItem));
    memberItem->addActionWidget(createActionButton([] { return tr("发消息"); }, ShadButtonType::Outline, memberItem));
    ShadItem* teamItem = appendItem(avatarGroup, ShadItemType::Outline, ShadItemType::Medium, [] { return tr("还没有团队成员"); }, [] { return tr("邀请团队成员一起协作这个项目。"); });
    ShadAvatarGroup* avatars = new ShadAvatarGroup(teamItem);
    for (const QString& fallback : {QStringLiteral("CN"), QStringLiteral("LR"), QStringLiteral("ER")})
    {
        avatars->addAvatar(new ShadAvatar(fallback, avatars));
    }
    teamItem->setMediaWidget(avatars);
    teamItem->addActionWidget(createActionButton([] { return tr("邀请"); }, ShadButtonType::Default, teamItem));
    addStretchItem(avatarCard, avatarGroup);

    // 可点击
    ShadCard* clickableCard = addSection([] { return tr("可点击"); }, [] { return tr("setClickable(true)：作为链接使用，单击发出 clicked"); });
    ShadItemGroup* clickableGroup = createGroup(this);
    ShadLabel* clickResult = createMutedText(this);
    ShadItem* titleLink = appendItem(clickableGroup, ShadItemType::Default, ShadItemType::Medium, [] { return tr("仅标题"); }, nullptr);
    ShadItem* descriptionLink = appendItem(clickableGroup, ShadItemType::Default, ShadItemType::Medium, [] { return tr("标题 + 描述"); }, [] { return tr("可点击的列表项，包含标题和描述。"); });
    ShadItem* mediaLink = appendItem(clickableGroup, ShadItemType::Default, ShadItemType::Medium, [] { return tr("图标 + 标题 + 描述"); }, [] { return tr("完整的可点击列表项。"); });
    mediaLink->setMediaIcon(ShadIconType::Inbox);
    ShadItem* actionLink = appendItem(clickableGroup, ShadItemType::Default, ShadItemType::Medium, [] { return tr("带操作"); }, [] { return tr("可点击的列表项也可以有操作按钮。"); });
    ShadButton* shareButton = createActionButton([] { return tr("分享"); }, ShadButtonType::Outline, actionLink);
    actionLink->addActionWidget(shareButton);
    // 保存生成当前结果文字的函数，切换语言时按最近一次点击重新生成；列表项标题的绑定先连接，先于这里刷新
    const std::shared_ptr<ExampleI18n::TextFunction> clickText = std::make_shared<ExampleI18n::TextFunction>([] { return tr("点击任意一行"); });
    const auto updateClickResult = [clickResult, clickText]() { clickResult->setText((*clickText)()); };
    for (ShadItem* item : {titleLink, descriptionLink, mediaLink, actionLink})
    {
        item->setClickable(true);
        connect(item, &ShadItem::clicked, clickResult, [item, clickText, updateClickResult]()
        {
            *clickText = [item]() { return tr("点击了：%1").arg(item->getTitle()); };
            updateClickResult();
        });
    }
    connect(shareButton, &ShadButton::clicked, clickResult, [clickText, updateClickResult]()
    {
        *clickText = []() { return tr("点击了：分享按钮"); };
        updateClickResult();
    });
    ExampleI18n::bind(clickResult, updateClickResult);
    addStretchItem(clickableCard, clickableGroup);
    addItem(clickableCard, clickResult);

    // 分组与分隔线
    ShadCard* separatorCard = addSection([] { return tr("分组与分隔线"); }, [] { return tr("纵向排列多个列表项，并插入分隔线"); });
    ShadItemGroup* mailGroup = createGroup(this);
    struct MailboxDemo
    {
        ShadIconType::IconName icon;
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction description;
    };
    const QList<MailboxDemo> mailboxes{{ShadIconType::Inbox, [] { return tr("收件箱"); }, [] { return tr("查看所有收到的消息。"); }}, {ShadIconType::Send, [] { return tr("已发送"); }, [] { return tr("查看所有已发送的消息。"); }}, {ShadIconType::FileText, [] { return tr("草稿"); }, [] { return tr("查看所有草稿。"); }}, {ShadIconType::Archive, [] { return tr("归档"); }, [] { return tr("查看已归档的消息。"); }}};
    for (int index = 0; index < mailboxes.size(); ++index)
    {
        if (index > 0)
        {
            mailGroup->addSeparator();
        }
        ShadItem* item = appendItem(mailGroup, ShadItemType::Outline, ShadItemType::Medium, mailboxes.at(index).title, mailboxes.at(index).description);
        item->setMediaIcon(mailboxes.at(index).icon);
    }
    addStretchItem(separatorCard, mailGroup);

    // 页眉与页脚
    ShadCard* edgeCard = addSection([] { return tr("页眉与页脚"); }, [] { return tr("在主行上方或下方添加文字或任意组件"); });
    ShadItemGroup* edgeGroup = createGroup(this);
    ShadItem* headerItem = appendItem(edgeGroup, ShadItemType::Default, ShadItemType::Medium, [] { return tr("组件库"); }, [] { return tr("一套可复用的界面组件，用于构建风格一致的界面。"); });
    ExampleI18n::bind(headerItem, [headerItem]() { headerItem->setHeaderText(tr("设计系统")); });
    ShadItem* footerItem = appendItem(edgeGroup, ShadItemType::Outline, ShadItemType::Medium, [] { return tr("第四季度报告"); }, [] { return tr("第四季度的收入、支出与增长指标概览。"); });
    ExampleI18n::bind(footerItem, [footerItem]() { footerItem->setFooterText(tr("2 小时前更新")); });
    ShadItem* bothItem = appendItem(edgeGroup, ShadItemType::Muted, ShadItemType::Medium, [] { return tr("网站改版"); }, [] { return tr("按现代设计原则全面改版公司网站，提升用户体验。"); });
    ExampleI18n::bind(bothItem, [bothItem]()
    {
        bothItem->setHeaderText(tr("团队项目"));
        bothItem->setFooterText(tr("5 分钟前更新"));
    });
    addStretchItem(edgeCard, edgeGroup);

    // RTL
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左布局时媒体区与操作区左右对调"); });
    ShadItemGroup* rtlGroup = createGroup(this);
    rtlGroup->setLayoutDirection(Qt::RightToLeft);
    ShadItem* rtlItem = appendItem(rtlGroup, ShadItemType::Outline, ShadItemType::Medium, [] { return tr("收件箱"); }, [] { return tr("查看所有收到的消息。"); });
    rtlItem->setMediaIcon(ShadIconType::Inbox);
    rtlItem->addActionWidget(createActionButton([] { return tr("打开"); }, ShadButtonType::Outline, rtlItem));
    ShadItem* rtlLink = appendItem(rtlGroup, ShadItemType::Muted, ShadItemType::Small, [] { return tr("可点击的列表项"); }, [] { return tr("悬停与焦点环同样适用。"); });
    rtlLink->setClickable(true);
    addStretchItem(rtlCard, rtlGroup);
}

ItemPage::~ItemPage()
{
}
