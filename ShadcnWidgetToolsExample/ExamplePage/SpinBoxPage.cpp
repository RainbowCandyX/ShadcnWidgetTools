#include "SpinBoxPage.h"

#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDoubleSpinBox.h"
#include "ShadLabel.h"
#include "ShadSpinBox.h"

SpinBoxPage::SpinBoxPage(QWidget* parent)
    : ExampleBasePage([] { return tr("数字输入框"); }, [] { return tr("ShadSpinBox / ShadDoubleSpinBox：外观与输入框一致的数字输入框，提供四种按钮布局。"); }, parent)
{
    // 基础用法
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("取值范围 0 到 100，右键打开操作菜单"); });
    ShadSpinBox* quantitySpinBox = new ShadSpinBox(this);
    quantitySpinBox->setRange(0, 100);
    quantitySpinBox->setValue(12);
    addRow(basicCard, {createField([] { return tr("数量"); }, quantitySpinBox)});

    // 按钮布局
    ShadCard* layoutCard = addSection([] { return tr("按钮布局"); }, [] { return tr("setButtonLayout：四种增减按钮布局"); });
    struct LayoutDemo
    {
        QString name;
        ShadSpinBoxType::ButtonLayout layout;
    };
    const QList<LayoutDemo> layoutDemos{
        {QStringLiteral("Inline"), ShadSpinBoxType::Inline},
        {QStringLiteral("Compact"), ShadSpinBoxType::Compact},
        {QStringLiteral("Side"), ShadSpinBoxType::Side},
        {QStringLiteral("PlusMinusSide"), ShadSpinBoxType::PlusMinusSide},
    };
    QList<QWidget*> layoutFields;
    for (const LayoutDemo& demo : layoutDemos)
    {
        ShadSpinBox* spinBox = new ShadSpinBox(this);
        spinBox->setRange(0, 999);
        spinBox->setValue(50);
        spinBox->setButtonLayout(demo.layout);
        const QString name = demo.name;
        layoutFields.append(createField([name] { return name; }, spinBox));
    }
    addRow(layoutCard, layoutFields);

    // 小数
    ShadCard* decimalCard = addSection([] { return tr("小数"); }, [] { return tr("ShadDoubleSpinBox：设置小数位数与步长"); });
    ShadDoubleSpinBox* weightSpinBox = new ShadDoubleSpinBox(this);
    weightSpinBox->setRange(0.0, 200.0);
    weightSpinBox->setDecimals(2);
    weightSpinBox->setSingleStep(0.5);
    weightSpinBox->setValue(62.5);
    ShadDoubleSpinBox* ratioSpinBox = new ShadDoubleSpinBox(this);
    ratioSpinBox->setRange(0.0, 1.0);
    ratioSpinBox->setDecimals(3);
    ratioSpinBox->setSingleStep(0.005);
    ratioSpinBox->setValue(0.25);
    ratioSpinBox->setButtonLayout(ShadSpinBoxType::Compact);
    addRow(decimalCard, {createField([] { return tr("体重（千克）"); }, weightSpinBox), createField([] { return tr("比例"); }, ratioSpinBox)});

    // 前后缀
    ShadCard* affixCard = addSection([] { return tr("前后缀"); }, [] { return tr("数值前后显示固定文字，最小值时显示替代文字"); });
    ShadDoubleSpinBox* priceSpinBox = new ShadDoubleSpinBox(this);
    priceSpinBox->setPrefix(QStringLiteral("¥ "));
    priceSpinBox->setRange(0.0, 99999.0);
    priceSpinBox->setDecimals(2);
    priceSpinBox->setValue(199.0);
    ShadSpinBox* percentSpinBox = new ShadSpinBox(this);
    percentSpinBox->setSuffix(QStringLiteral(" %"));
    percentSpinBox->setRange(0, 100);
    percentSpinBox->setSingleStep(5);
    percentSpinBox->setValue(80);
    percentSpinBox->setButtonLayout(ShadSpinBoxType::PlusMinusSide);
    ShadSpinBox* limitSpinBox = new ShadSpinBox(this);
    limitSpinBox->setRange(0, 10);
    // 只改替代文字与后缀，数值保持不变
    ExampleI18n::bind(limitSpinBox, [limitSpinBox]()
    {
        limitSpinBox->setSpecialValueText(tr("不限"));
        limitSpinBox->setSuffix(tr(" 次"));
    });
    addRow(affixCard, {createField([] { return tr("价格"); }, priceSpinBox), createField([] { return tr("音量"); }, percentSpinBox), createField([] { return tr("重试次数"); }, limitSpinBox)});

    // 禁用与错误态
    ShadCard* stateCard = addSection([] { return tr("禁用与错误态"); }, [] { return tr("禁用与错误态，到达边界时对应按钮半透明"); });
    ShadSpinBox* disabledSpinBox = new ShadSpinBox(this);
    disabledSpinBox->setValue(5);
    disabledSpinBox->setEnabled(false);
    ShadSpinBox* invalidSpinBox = new ShadSpinBox(this);
    invalidSpinBox->setRange(0, 10);
    invalidSpinBox->setValue(10);
    invalidSpinBox->setInvalid(true);
    addRow(stateCard, {createField([] { return tr("禁用"); }, disabledSpinBox), createField([] { return tr("错误（已达最大值）"); }, invalidSpinBox)});

    // 数值变化
    ShadCard* signalCard = addSection([] { return tr("数值变化"); }, [] { return tr("valueChanged：数值变化时更新下方文字"); });
    ShadSpinBox* signalSpinBox = new ShadSpinBox(this);
    signalSpinBox->setRange(1, 12);
    signalSpinBox->setValue(3);
    ShadLabel* signalLabel = new ShadLabel(this);
    signalLabel->setColorRole(ShadThemeType::MutedForeground);
    // 按当前数值生成文字，数值变化与切换语言时都调用
    const auto updateSignalLabel = [signalLabel, signalSpinBox]() { signalLabel->setText(tr("订阅时长：%1 个月，合计 %2 元").arg(signalSpinBox->value()).arg(signalSpinBox->value() * 30)); };
    connect(signalSpinBox, &ShadSpinBox::valueChanged, signalLabel, updateSignalLabel);
    ExampleI18n::bind(signalLabel, [signalSpinBox, updateSignalLabel]()
    {
        signalSpinBox->setSuffix(tr(" 个月"));
        updateSignalLabel();
    });
    addRow(signalCard, {createField([] { return tr("订阅时长"); }, signalSpinBox), signalLabel});
}

SpinBoxPage::~SpinBoxPage()
{
}

QWidget* SpinBoxPage::createField(const ExampleI18n::TextFunction& labelText, QWidget* field)
{
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    // gap-3 扣除输入框上方 3px 焦点环边距
    layout->setSpacing(12 - ExampleLayout::kFocusRingWidth);
    ShadLabel* label = new ShadLabel(container);
    ExampleI18n::bind(label, [label, labelText]() { label->setText(labelText()); });
    label->setBuddy(field);
    // 标签与输入框可视边缘对齐
    label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    layout->addWidget(label);
    layout->addWidget(field, 0, Qt::AlignLeft);
    return container;
}
