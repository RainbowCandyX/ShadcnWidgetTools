#include "LabelPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QPair>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    // 标签在输入框上方：gap-2，标签缩进焦点环宽度以对齐输入框边缘；标签文字随语言刷新
    QWidget* createStackedField(const ExampleI18n::TextFunction& text, QWidget* control, QWidget* parent)
    {
        QWidget* field = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(field);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
        ShadLabel* label = new ShadLabel(field);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
        label->setBuddy(control);
        layout->addWidget(label);
        layout->addWidget(control);
        return field;
    }

    // 标签在控件后面：gap-2，扣除焦点环边距；标签文字随语言刷新
    QWidget* createInlineField(QWidget* control, const ExampleI18n::TextFunction& text, QWidget* parent)
    {
        QWidget* field = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(field);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
        ShadLabel* label = new ShadLabel(field);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        label->setBuddy(control);
        layout->addWidget(control);
        layout->addWidget(label);
        return field;
    }

    // 占位文字随语言刷新，输入内容不受影响
    ShadInput* createInput(const ExampleI18n::TextFunction& placeholder, QWidget* parent)
    {
        ShadInput* input = new ShadInput(parent);
        input->setFixedWidth(ExampleLayout::kFieldWidth);
        ExampleI18n::bind(input, [input, placeholder]() { input->setPlaceholderText(placeholder()); });
        return input;
    }
} // namespace

LabelPage::LabelPage(QWidget* parent)
    : ExampleBasePage([] { return tr("标签"); }, [] { return tr("ShadLabel：与控件关联的文字标签。"); }, parent)
{
    ShadCard* inputCard = addSection([] { return tr("关联输入框"); }, [] { return tr("点击标签聚焦输入框"); });
    addItem(inputCard, createStackedField([] { return tr("邮箱"); }, createInput([] { return QStringLiteral("m@example.com"); }, this), this));

    ShadCard* toggleCard = addSection([] { return tr("关联复选框与开关"); }, [] { return tr("点击标签切换状态"); });
    addRow(toggleCard, {createInlineField(new ShadCheckbox(this), [] { return tr("接受服务条款"); }, this), createInlineField(new ShadSwitch(this), [] { return tr("飞行模式"); }, this)});

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("控件禁用时标签变淡"); });
    ShadSwitch* disableSwitch = new ShadSwitch(this);
    ExampleI18n::bind(disableSwitch, [disableSwitch]() { disableSwitch->setText(tr("禁用下方控件")); });
    disableSwitch->setChecked(true);
    ShadInput* disabledInput = createInput([] { return tr("请输入用户名"); }, this);
    ShadCheckbox* disabledCheckbox = new ShadCheckbox(this);
    for (QWidget* control : QList<QWidget*>{disabledInput, disabledCheckbox})
    {
        control->setEnabled(false);
        connect(disableSwitch, &ShadSwitch::toggled, control, [control](bool isDisabled) { control->setEnabled(!isDisabled); });
    }
    addItem(disabledCard, disableSwitch);
    addItem(disabledCard, createStackedField([] { return tr("用户名"); }, disabledInput, this));
    addItem(disabledCard, createInlineField(disabledCheckbox, [] { return tr("记住我"); }, this));

    ShadCard* colorCard = addSection([] { return tr("颜色"); }, [] { return tr("setColorRole 指定文字颜色"); });
    const QList<QPair<ShadThemeType::ThemeColor, QString>> colorRoles{
        {ShadThemeType::Foreground, QStringLiteral("Foreground")},
        {ShadThemeType::MutedForeground, QStringLiteral("MutedForeground")},
        {ShadThemeType::Primary, QStringLiteral("Primary")},
        {ShadThemeType::Destructive, QStringLiteral("Destructive")},
    };
    QList<QWidget*> colorLabels;
    for (const QPair<ShadThemeType::ThemeColor, QString>& colorRole : colorRoles)
    {
        ShadLabel* label = new ShadLabel(colorRole.second, this);
        label->setColorRole(colorRole.first);
        colorLabels.append(label);
    }
    addRow(colorCard, colorLabels);

    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左布局整体镜像"); });
    QWidget* rtlArea = new QWidget(this);
    rtlArea->setLayoutDirection(Qt::RightToLeft);
    QVBoxLayout* rtlLayout = new QVBoxLayout(rtlArea);
    rtlLayout->setContentsMargins(0, 0, 0, 0);
    rtlLayout->setSpacing(12);
    // AlignLeading 在从右到左布局中靠右；阿拉伯语示例文字不翻译
    rtlLayout->addWidget(createStackedField([] { return QStringLiteral("البريد الإلكتروني"); }, createInput([] { return QStringLiteral("m@example.com"); }, rtlArea), rtlArea), 0, Qt::AlignLeading);
    rtlLayout->addWidget(createInlineField(new ShadCheckbox(rtlArea), [] { return QStringLiteral("قبول الشروط والأحكام"); }, rtlArea), 0, Qt::AlignLeading);
    addStretchItem(rtlCard, rtlArea);
}

LabelPage::~LabelPage()
{
}
