#include "TagInputPage.h"

#include <QString>
#include <QStringList>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadTagInput.h"

namespace
{
    constexpr int kMaxTags = 3;

    ShadTagInput* createTagInput(ExampleI18n::TextFunction placeholder, QWidget* parent)
    {
        ShadTagInput* tagInput = new ShadTagInput(parent);
        tagInput->setFixedWidth(ExampleLayout::kWideFieldWidth);
        ExampleI18n::bind(tagInput, [tagInput, placeholder]() { tagInput->setPlaceholderText(placeholder()); });
        return tagInput;
    }

    ShadLabel* createMutedLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

TagInputPage::TagInputPage(QWidget* parent)
    : ExampleBasePage([] { return tr("标签输入"); }, [] { return tr("ShadTagInput：输入文字后按回车生成标签，标签显示在输入框内，放不下时自动换行。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("回车生成标签；输入为空时按 Backspace 选中最后一个标签，再按一次删除"); });
    ShadTagInput* basicInput = createTagInput([] { return tr("输入标签后按回车"); }, this);
    basicInput->setTags({QStringLiteral("Qt"), QStringLiteral("C++"), QStringLiteral("Lua")});
    ShadLabel* tagsLabel = createMutedLabel(this);
    const auto updateTagsLabel = [basicInput, tagsLabel]() { tagsLabel->setText(basicInput->count() == 0 ? tr("当前标签: 无") : tr("当前标签: %1").arg(basicInput->getTags().join(QStringLiteral(", ")))); };
    ExampleI18n::bind(tagsLabel, updateTagsLabel);
    connect(basicInput, &ShadTagInput::tagsChanged, tagsLabel, updateTagsLabel);
    addItem(basicCard, basicInput);
    addItem(basicCard, tagsLabel);

    ShadCard* separatorCard = addSection([] { return tr("分隔符"); }, [] { return tr("setSeparators(\",;\")：输入逗号或分号即生成标签，粘贴多行文字时每行生成一个"); });
    ShadTagInput* emailInput = createTagInput([] { return tr("输入邮箱，用逗号或分号分隔"); }, this);
    emailInput->setSeparators(QStringLiteral(",;"));
    addItem(separatorCard, emailInput);

    ShadCard* limitCard = addSection([] { return tr("数量上限"); }, [] { return tr("setMaxTags(3)：重复或超出上限的标签被拒绝，并发出 tagRejected"); });
    ShadTagInput* limitInput = createTagInput([] { return tr("最多添加 %1 个标签").arg(kMaxTags); }, this);
    limitInput->setMaxTags(kMaxTags);
    ShadLabel* limitLabel = createMutedLabel(this);
    const auto updateLimitLabel = [this, limitInput, limitLabel]() { limitLabel->setText(_rejectedTag.isEmpty() ? tr("已添加 %1 / %2").arg(limitInput->count()).arg(kMaxTags) : tr("已添加 %1 / %2，已拒绝: %3").arg(limitInput->count()).arg(kMaxTags).arg(_rejectedTag)); };
    ExampleI18n::bind(limitLabel, updateLimitLabel);
    connect(limitInput, &ShadTagInput::tagsChanged, limitLabel, [this, updateLimitLabel]()
    {
        _rejectedTag.clear();
        updateLimitLabel();
    });
    connect(limitInput, &ShadTagInput::tagRejected, limitLabel, [this, updateLimitLabel](const QString& tag)
    {
        _rejectedTag = tag;
        updateLimitLabel();
    });
    addItem(limitCard, limitInput);
    addItem(limitCard, limitLabel);

    ShadCard* stateCard = addSection([] { return tr("只读与禁用"); }, [] { return tr("setReadOnly(true) 不显示输入框与删除按钮；setEnabled(false) 整体置灰"); });
    ShadTagInput* readOnlyInput = new ShadTagInput(this);
    readOnlyInput->setFixedWidth(ExampleLayout::kWideFieldWidth);
    readOnlyInput->setTags({QStringLiteral("macOS"), QStringLiteral("Windows"), QStringLiteral("Linux")});
    readOnlyInput->setReadOnly(true);
    ShadTagInput* disabledInput = new ShadTagInput(this);
    disabledInput->setFixedWidth(ExampleLayout::kWideFieldWidth);
    disabledInput->setTags({QStringLiteral("macOS"), QStringLiteral("Windows"), QStringLiteral("Linux")});
    disabledInput->setEnabled(false);
    addItem(stateCard, readOnlyInput);
    addItem(stateCard, disabledInput);

    ShadCard* invalidCard = addSection([] { return tr("错误态"); }, [] { return tr("setInvalid(true)：边框与焦点环变为 destructive 色"); });
    ShadTagInput* invalidInput = createTagInput([] { return tr("输入标签后按回车"); }, this);
    invalidInput->setInvalid(true);
    addItem(invalidCard, invalidInput);
}

TagInputPage::~TagInputPage()
{
}
