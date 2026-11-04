#include "CollapsiblePage.h"

#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCollapsible.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadItem.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kCollapsibleWidth = 350;

    ShadItem* createRepositoryItem(const QString& name, QWidget* parent)
    {
        ShadItem* item = new ShadItem(name, QString(), parent);
        item->setVariant(ShadItemType::Outline);
        return item;
    }

    // 标题与触发按钮的提示随语言刷新
    ShadButton* addTitleRow(ShadCollapsible* collapsible, const ExampleI18n::TextFunction& title)
    {
        QWidget* row = new QWidget(collapsible);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        ShadLabel* label = new ShadLabel(row);
        QFont font = label->font();
        font.setPixelSize(14);
        font.setWeight(QFont::DemiBold);
        label->setFont(font);
        ShadButton* trigger = new ShadButton(QString(), ShadButtonType::Ghost, row);
        trigger->setButtonSize(ShadButtonType::IconSmall);
        trigger->setShadIcon(ShadIconType::ChevronsUpDown);
        ExampleI18n::bind(row, [label, trigger, title]()
        {
            label->setText(title());
            trigger->setToolTip(QCoreApplication::translate("CollapsiblePage", "展开或收起"));
        });
        layout->addWidget(label, 1);
        layout->addWidget(trigger);
        collapsible->headerLayout()->addWidget(row);
        collapsible->setTrigger(trigger);
        return trigger;
    }

    ShadCollapsible* createRepositoryCollapsible(QWidget* parent)
    {
        ShadCollapsible* collapsible = new ShadCollapsible(parent);
        collapsible->setMaximumWidth(kCollapsibleWidth);
        addTitleRow(collapsible, [] { return QCoreApplication::translate("CollapsiblePage", "@peduarte 收藏了 3 个仓库"); });
        collapsible->headerLayout()->addWidget(createRepositoryItem(QStringLiteral("@radix-ui/primitives"), collapsible));
        collapsible->contentLayout()->addWidget(createRepositoryItem(QStringLiteral("@radix-ui/colors"), collapsible));
        collapsible->contentLayout()->addWidget(createRepositoryItem(QStringLiteral("@stitches/react"), collapsible));
        return collapsible;
    }
} // namespace

CollapsiblePage::CollapsiblePage(QWidget* parent)
    : ExampleBasePage([] { return tr("折叠面板"); }, [] { return tr("ShadCollapsible：可展开收起的内容区。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击按钮展开"); });
    addStretchItem(basicCard, createRepositoryCollapsible(this));

    ShadCard* openCard = addSection([] { return tr("默认展开"); }, [] { return QStringLiteral("setOpen(true)"); });
    ShadCollapsible* openCollapsible = createRepositoryCollapsible(this);
    openCollapsible->setOpen(true);
    addStretchItem(openCard, openCollapsible);

    ShadCard* formCard = addSection([] { return tr("放置表单"); }, [] { return tr("内容区可以放任意组件"); });
    ShadCollapsible* formCollapsible = new ShadCollapsible(this);
    formCollapsible->setMaximumWidth(kCollapsibleWidth);
    addTitleRow(formCollapsible, [] { return tr("高级设置"); });
    ShadSwitch* autoSaveSwitch = new ShadSwitch(formCollapsible);
    ShadSwitch* lineNumberSwitch = new ShadSwitch(formCollapsible);
    ShadInput* nameInput = new ShadInput(formCollapsible);
    // 只刷新开关文字与占位文字，不改动输入内容
    ExampleI18n::bind(formCollapsible, [autoSaveSwitch, lineNumberSwitch, nameInput]()
    {
        autoSaveSwitch->setText(tr("自动保存"));
        lineNumberSwitch->setText(tr("显示行号"));
        nameInput->setPlaceholderText(tr("项目名称"));
    });
    formCollapsible->contentLayout()->addWidget(autoSaveSwitch);
    formCollapsible->contentLayout()->addWidget(lineNumberSwitch);
    formCollapsible->contentLayout()->addWidget(nameInput);
    addStretchItem(formCard, formCollapsible);
}

CollapsiblePage::~CollapsiblePage()
{
}
