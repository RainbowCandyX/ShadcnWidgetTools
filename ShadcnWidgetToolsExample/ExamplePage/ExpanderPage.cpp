#include "ExpanderPage.h"

#include <QHBoxLayout>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadExpander.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    ShadLabel* createIconLabel(ShadIconType::IconName icon, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(ShadIcon::glyph(icon), parent);
        label->setFont(ShadIcon::font(16));
        return label;
    }

    // 文字随语言刷新的标签
    ShadLabel* createTextLabel(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        return label;
    }

    QWidget* createSettingRow(ExampleI18n::TextFunction title, QWidget* control, QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(createTextLabel(title, row), 1);
        layout->addWidget(control);
        return row;
    }
} // namespace

ExpanderPage::ExpanderPage(QWidget* parent)
    : ExampleBasePage([] { return tr("抽屉面板"); }, [] { return tr("ShadExpander：标题行展开内容行。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击标题行展开"); });
    ShadExpander* expander = new ShadExpander(this);
    expander->headerLayout()->addWidget(createIconLabel(ShadIconType::MessageSquareShare, expander));
    expander->headerLayout()->addWidget(createTextLabel([] { return tr("抽屉面板"); }, expander), 1);
    for (int index = 1; index <= 3; ++index)
    {
        // 只更新文字，勾选状态不变
        ShadCheckbox* checkbox = new ShadCheckbox(expander);
        ExampleI18n::bind(checkbox, [checkbox, index]() { checkbox->setText(tr("测试窗口%1").arg(index)); });
        expander->addItem(checkbox);
    }
    addStretchItem(basicCard, expander);

    ShadCard* settingsCard = addSection([] { return tr("设置分组"); }, [] { return tr("每行放一项设置"); });
    ShadExpander* settingsExpander = new ShadExpander(this);
    settingsExpander->headerLayout()->addWidget(createIconLabel(ShadIconType::Settings2, settingsExpander));
    settingsExpander->headerLayout()->addWidget(createTextLabel([] { return tr("编辑器"); }, settingsExpander), 1);
    settingsExpander->addItem(createSettingRow([] { return tr("自动保存"); }, new ShadSwitch(settingsExpander), settingsExpander));
    settingsExpander->addItem(createSettingRow([] { return tr("显示行号"); }, new ShadSwitch(settingsExpander), settingsExpander));
    ShadComboBox* tabSizeComboBox = new ShadComboBox(settingsExpander);
    tabSizeComboBox->addItems({QString(), QString(), QString()});
    // 按索引更新选项文字，当前选中项不变
    ExampleI18n::bind(tabSizeComboBox, [tabSizeComboBox]()
    {
        tabSizeComboBox->setItemText(0, tr("2 个空格"));
        tabSizeComboBox->setItemText(1, tr("4 个空格"));
        tabSizeComboBox->setItemText(2, tr("制表符"));
    });
    settingsExpander->addItem(createSettingRow([] { return tr("缩进"); }, tabSizeComboBox, settingsExpander));
    addStretchItem(settingsCard, settingsExpander);
}

ExpanderPage::~ExpanderPage()
{
}
