#include "DescriptionsPage.h"

#include <QList>
#include <QString>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDescriptions.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadToggleGroup.h"

namespace
{
    // 尺寸与列数示例的默认值
    constexpr int kDefaultSizeIndex = 1;
    constexpr int kDefaultColumnCount = 3;
    constexpr int kMaxColumnCount = 4;

    // 添加一项文字，切换语言时刷新标签与内容
    void addTextItem(ShadDescriptions* descriptions, ExampleI18n::TextFunction label, ExampleI18n::TextFunction text, int span = 1)
    {
        const int index = descriptions->addItem(QString(), QString(), span);
        ExampleI18n::bind(descriptions, [descriptions, index, label, text]()
        {
            descriptions->setItemLabel(index, label());
            descriptions->setItemText(index, text());
        });
    }

    // 添加一项组件，切换语言时刷新标签
    void addWidgetItem(ShadDescriptions* descriptions, ExampleI18n::TextFunction label, QWidget* content, int span = 1)
    {
        const int index = descriptions->addItem(QString(), content, span);
        ExampleI18n::bind(descriptions, [descriptions, index, label]() { descriptions->setItemLabel(index, label()); });
    }

    // 不随语言变化的文字
    ExampleI18n::TextFunction fixedText(const QString& text)
    {
        return [text]() { return text; };
    }

    ShadLabel* createCaption(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        return label;
    }

    ShadToggleGroup* createToggleGroup(const QList<QString>& texts, QWidget* parent)
    {
        ShadToggleGroup* group = new ShadToggleGroup(ShadToggleGroupType::Single, parent);
        group->setVariant(ShadToggleType::Outline);
        group->setToggleSize(ShadToggleType::Small);
        for (const QString& text : texts)
        {
            group->addItem(text);
        }
        return group;
    }
} // namespace

DescriptionsPage::DescriptionsPage(QWidget* parent)
    : ExampleBasePage([] { return tr("描述列表"); }, [] { return tr("ShadDescriptions：键值对按列网格排列，支持跨列、带边框与纵向排列，内容可以是文字或任意组件。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("3 列排列，标题行结束侧放置操作按钮；最后一项跨 2 列"); });
    ShadDescriptions* userDescriptions = new ShadDescriptions(this);
    ShadButton* editButton = new ShadButton(QString(), ShadButtonType::Outline);
    editButton->setButtonSize(ShadButtonType::Small);
    editButton->setShadIcon(ShadIconType::Pencil);
    ExampleI18n::bind(editButton, [userDescriptions, editButton]()
    {
        userDescriptions->setTitle(tr("用户信息"));
        editButton->setText(tr("编辑"));
    });
    userDescriptions->setExtra(editButton);
    addTextItem(userDescriptions, [] { return tr("姓名"); }, [] { return tr("陈怡君"); });
    addTextItem(userDescriptions, [] { return tr("手机号码"); }, fixedText(QStringLiteral("0912-345-678")));
    addTextItem(userDescriptions, [] { return tr("居住地"); }, [] { return tr("台北市"); });
    addTextItem(userDescriptions, [] { return tr("电子邮件"); }, fixedText(QStringLiteral("yijun.chen@example.com")));
    addTextItem(userDescriptions, [] { return tr("地址"); }, [] { return tr("台北市大安区忠孝东路四段 100 号 8 楼"); }, 2);
    addStretchItem(basicCard, userDescriptions);

    ShadCard* borderedCard = addSection([] { return tr("带边框"); }, [] { return tr("setBordered(true)：标签格带底色，单元格之间有分隔线；状态使用 ShadBadge，收货地址与备注跨 3 列"); });
    ShadDescriptions* orderDescriptions = new ShadDescriptions(this);
    orderDescriptions->setBordered(true);
    ExampleI18n::bind(orderDescriptions, [orderDescriptions]() { orderDescriptions->setTitle(tr("订单详情")); });
    addTextItem(orderDescriptions, [] { return tr("订单编号"); }, fixedText(QStringLiteral("SO-20261031-0042")));
    ShadBadge* statusBadge = new ShadBadge(QString(), ShadBadgeType::Default);
    ExampleI18n::bind(statusBadge, [statusBadge]() { statusBadge->setText(tr("已付款")); });
    addWidgetItem(orderDescriptions, [] { return tr("状态"); }, statusBadge);
    addTextItem(orderDescriptions, [] { return tr("创建时间"); }, fixedText(QStringLiteral("2026-10-31 14:32")));
    addTextItem(orderDescriptions, [] { return tr("付款方式"); }, [] { return tr("信用卡"); });
    addTextItem(orderDescriptions, [] { return tr("金额"); }, fixedText(QStringLiteral("NT$ 3,280")));
    addTextItem(orderDescriptions, [] { return tr("配送方式"); }, [] { return tr("宅配"); });
    addTextItem(orderDescriptions, [] { return tr("收货地址"); }, [] { return tr("台北市大安区忠孝东路四段 100 号 8 楼\n陈怡君 0912-345-678"); }, 3);
    addTextItem(orderDescriptions, [] { return tr("备注"); }, [] { return tr("请于平日晚上六点后送达，送达前请先来电。"); }, 3);
    addStretchItem(borderedCard, orderDescriptions);

    ShadCard* verticalCard = addSection([] { return tr("纵向排列"); }, [] { return tr("setItemLayout(Vertical)：标签在上、内容在下；带边框时标签行与内容行交替"); });
    ShadDescriptions* serverDescriptions = new ShadDescriptions(this);
    serverDescriptions->setItemLayout(ShadDescriptionsType::Vertical);
    serverDescriptions->setBordered(true);
    ExampleI18n::bind(serverDescriptions, [serverDescriptions]() { serverDescriptions->setTitle(tr("服务器信息")); });
    addTextItem(serverDescriptions, [] { return tr("主机名称"); }, fixedText(QStringLiteral("web-01")));
    addTextItem(serverDescriptions, [] { return tr("IP 地址"); }, fixedText(QStringLiteral("10.0.12.34")));
    addTextItem(serverDescriptions, [] { return tr("机房"); }, [] { return tr("台北"); });
    addTextItem(serverDescriptions, [] { return tr("处理器"); }, [] { return tr("8 核"); });
    addTextItem(serverDescriptions, [] { return tr("内存"); }, fixedText(QStringLiteral("32 GB")));
    addTextItem(serverDescriptions, [] { return tr("磁盘"); }, fixedText(QStringLiteral("512 GB SSD")));
    addStretchItem(verticalCard, serverDescriptions);
    ShadDescriptions* plainVerticalDescriptions = new ShadDescriptions(this);
    plainVerticalDescriptions->setItemLayout(ShadDescriptionsType::Vertical);
    addTextItem(plainVerticalDescriptions, [] { return tr("项目名称"); }, [] { return tr("官网改版"); });
    addTextItem(plainVerticalDescriptions, [] { return tr("负责人"); }, [] { return tr("林志豪"); });
    addTextItem(plainVerticalDescriptions, [] { return tr("截止日期"); }, fixedText(QStringLiteral("2026-12-15")));
    addStretchItem(verticalCard, plainVerticalDescriptions);

    ShadCard* sizeCard = addSection([] { return tr("尺寸与列数"); }, [] { return tr("setDescriptionsSize 切换 Small 与 Medium，setColumnCount 设置 1 到 4 列"); });
    ShadToggleGroup* sizeGroup = createToggleGroup({QStringLiteral("Small"), QStringLiteral("Medium")}, this);
    QList<QString> columnTexts;
    for (int column = 1; column <= kMaxColumnCount; ++column)
    {
        columnTexts.append(QString::number(column));
    }
    ShadToggleGroup* columnGroup = createToggleGroup(columnTexts, this);
    addRow(sizeCard, {createCaption([] { return tr("尺寸"); }, this), sizeGroup, createCaption([] { return tr("列数"); }, this), columnGroup});
    ShadDescriptions* productDescriptions = new ShadDescriptions(this);
    productDescriptions->setBordered(true);
    addTextItem(productDescriptions, [] { return tr("商品名称"); }, [] { return tr("机械键盘"); });
    addTextItem(productDescriptions, [] { return tr("型号"); }, fixedText(QStringLiteral("K-87")));
    addTextItem(productDescriptions, [] { return tr("颜色"); }, [] { return tr("黑色"); });
    addTextItem(productDescriptions, [] { return tr("库存"); }, fixedText(QStringLiteral("128")));
    addTextItem(productDescriptions, [] { return tr("价格"); }, fixedText(QStringLiteral("NT$ 2,490")));
    addTextItem(productDescriptions, [] { return tr("产地"); }, [] { return tr("新北市"); });
    addStretchItem(sizeCard, productDescriptions);
    sizeGroup->setCurrentIndex(kDefaultSizeIndex);
    columnGroup->setCurrentIndex(kDefaultColumnCount - 1);
    connect(sizeGroup, &ShadToggleGroup::currentIndexChanged, productDescriptions, [sizeGroup, productDescriptions](int index)
    {
        // 再次点击已选项会取消选中，保持始终有一项被选中
        if (index < 0)
        {
            sizeGroup->setCurrentIndex(productDescriptions->getDescriptionsSize() == ShadDescriptionsType::Small ? 0 : 1);
            return;
        }
        productDescriptions->setDescriptionsSize(index == 0 ? ShadDescriptionsType::Small : ShadDescriptionsType::Medium);
    });
    connect(columnGroup, &ShadToggleGroup::currentIndexChanged, productDescriptions, [columnGroup, productDescriptions](int index)
    {
        if (index < 0)
        {
            columnGroup->setCurrentIndex(productDescriptions->getColumnCount() - 1);
            return;
        }
        productDescriptions->setColumnCount(index + 1);
    });
}

DescriptionsPage::~DescriptionsPage()
{
}
