#include "AccordionPage.h"

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadAccordion.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"

namespace
{
    constexpr int kAccordionWidth = 512;
    constexpr int kCardWidth = 384;

    ShadAccordion* createAccordion(QWidget* parent)
    {
        ShadAccordion* accordion = new ShadAccordion(parent);
        accordion->setMaximumWidth(kAccordionWidth);
        return accordion;
    }

    // 添加一项，标题与内容随语言刷新
    void addTranslatedItem(ShadAccordion* accordion, const QString& value, const ExampleI18n::TextFunction& title, const ExampleI18n::TextFunction& text)
    {
        ShadLabel* content = accordion->addItem(value, QString(), QString());
        ExampleI18n::bind(content, [accordion, value, content, title, text]()
        {
            accordion->setItemTitle(value, title());
            content->setText(text());
        });
    }

    void addFaqItems(ShadAccordion* accordion)
    {
        addTranslatedItem(accordion, QStringLiteral("shipping"), [] { return QCoreApplication::translate("AccordionPage", "多久能发货？"); }, []
        {
            return QCoreApplication::translate("AccordionPage", "下单后 1 到 2 个工作日发货。");
        });
        addTranslatedItem(accordion, QStringLiteral("returns"), [] { return QCoreApplication::translate("AccordionPage", "可以退货吗？"); }, []
        {
            return QCoreApplication::translate("AccordionPage", "收货 30 天内可申请退货。");
        });
        addTranslatedItem(accordion, QStringLiteral("support"), [] { return QCoreApplication::translate("AccordionPage", "怎么联系客服？"); }, []
        {
            return QCoreApplication::translate("AccordionPage", "工作日 9:00 到 18:00 在线。");
        });
    }
} // namespace

AccordionPage::AccordionPage(QWidget* parent)
    : ExampleBasePage([] { return tr("手风琴"); }, [] { return tr("ShadAccordion：可折叠的内容列表。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("同时只展开一项"); });
    ShadAccordion* basic = createAccordion(this);
    addFaqItems(basic);
    basic->setExpandedItems({QStringLiteral("shipping")});
    addStretchItem(basicCard, basic);

    ShadCard* multipleCard = addSection([] { return tr("多选"); }, [] { return QStringLiteral("setMultiple(true)"); });
    ShadAccordion* multiple = createAccordion(this);
    multiple->setMultiple(true);
    addTranslatedItem(multiple, QStringLiteral("notifications"), [] { return tr("通知"); }, [] { return tr("选择要接收的通知。"); });
    addTranslatedItem(multiple, QStringLiteral("privacy"), [] { return tr("隐私"); }, [] { return tr("控制谁能看到你的资料。"); });
    addTranslatedItem(multiple, QStringLiteral("billing"), [] { return tr("账单"); }, [] { return tr("管理付款方式。"); });
    multiple->setExpandedItems({QStringLiteral("notifications")});
    addStretchItem(multipleCard, multiple);

    ShadCard* disabledCard = addSection([] { return tr("禁用项"); }, [] { return QStringLiteral("setItemDisabled"); });
    ShadAccordion* disabled = createAccordion(this);
    addFaqItems(disabled);
    disabled->setItemDisabled(QStringLiteral("returns"), true);
    addStretchItem(disabledCard, disabled);

    ShadCard* borderedCard = addSection([] { return tr("带边框"); }, []
    {
        return QStringLiteral("ShadAccordionType::Bordered");
    });
    ShadAccordion* bordered = createAccordion(this);
    bordered->setVariant(ShadAccordionType::Bordered);
    addFaqItems(bordered);
    bordered->setExpandedItems({QStringLiteral("returns")});
    addStretchItem(borderedCard, bordered);

    ShadCard* cardSection = addSection([] { return tr("卡片中使用"); }, [] { return tr("放进 ShadCard"); });
    ShadCard* planCard = new ShadCard(this);
    ExampleI18n::bind(planCard, [planCard]()
    {
        planCard->setTitle(tr("订阅与账单"));
        planCard->setDescription(tr("常见问题"));
    });
    planCard->setMaximumWidth(kCardWidth);
    ShadAccordion* planAccordion = new ShadAccordion(planCard);
    addTranslatedItem(planAccordion, QStringLiteral("plans"), [] { return tr("有哪些套餐？"); }, [] { return tr("免费版、专业版和团队版。"); });
    addTranslatedItem(planAccordion, QStringLiteral("upgrade"), [] { return tr("如何升级？"); }, [] { return tr("在设置中更换套餐。"); });
    addTranslatedItem(planAccordion, QStringLiteral("cancel"), [] { return tr("可以随时取消吗？"); }, [] { return tr("可以，到期后不再续费。"); });
    planAccordion->setExpandedItems({QStringLiteral("plans")});
    planCard->contentLayout()->addWidget(planAccordion);
    addStretchItem(cardSection, planCard);

    ShadCard* controlCard = addSection([] { return tr("外部控制"); }, [] { return QStringLiteral("setExpandedItems"); });
    ShadAccordion* controlled = createAccordion(this);
    controlled->setMultiple(true);
    addFaqItems(controlled);
    ShadButton* expandButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(expandButton, &ShadButton::clicked, controlled, [controlled]()
    {
        controlled->setExpandedItems(controlled->getItems());
    });
    ShadButton* collapseButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(collapseButton, &ShadButton::clicked, controlled, [controlled]() { controlled->setExpandedItems(QStringList()); });
    ExampleI18n::bind(this, [expandButton, collapseButton]()
    {
        expandButton->setText(tr("全部展开"));
        collapseButton->setText(tr("全部收起"));
    });
    addRow(controlCard, {expandButton, collapseButton});
    addStretchItem(controlCard, controlled);
}

AccordionPage::~AccordionPage()
{
}
