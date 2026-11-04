#include "ComboBoxPage.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadLabel.h"

namespace
{
    // 触发框宽度（含左右各 3px 焦点环边距）
    constexpr int kComboBoxWidth = 246;

    ShadComboBox* createComboBox(ExampleI18n::TextFunction placeholder, const QStringList& items, QWidget* parent)
    {
        ShadComboBox* comboBox = new ShadComboBox(parent);
        ExampleI18n::bind(comboBox, [comboBox, placeholder]() { comboBox->setPlaceholderText(placeholder()); });
        comboBox->addItems(items);
        // 默认不选中，显示占位文字
        comboBox->setCurrentIndex(-1);
        comboBox->setFixedWidth(kComboBoxWidth);
        return comboBox;
    }

    void setItemEnabled(ShadComboBox* comboBox, int row, bool isEnabled)
    {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(comboBox->model());
        if (model == nullptr)
        {
            return;
        }
        if (QStandardItem* item = model->item(row))
        {
            item->setEnabled(isEnabled);
        }
    }
} // namespace

ComboBoxPage::ComboBoxPage(QWidget* parent)
    : ExampleBasePage([] { return tr("下拉选择"); }, [] { return tr("ShadComboBox：下拉选择组件，开启搜索后可模糊过滤选项。"); }, parent)
{
    const QStringList fruits = {QStringLiteral("Apple"), QStringLiteral("Banana"), QStringLiteral("Blueberry"), QStringLiteral("Grapes"), QStringLiteral("Pineapple")};

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击或用键盘打开，直接输入文字可跳到匹配项"); });
    ShadComboBox* fruitComboBox = createComboBox([] { return tr("选择水果"); }, fruits, this);
    ShadLabel* fruitLabel = new ShadLabel(this);
    fruitLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateFruitLabel = [fruitLabel, fruitComboBox]()
    {
        const QString text = fruitComboBox->currentText();
        fruitLabel->setText(text.isEmpty() ? tr("当前选择: 无") : tr("当前选择: %1").arg(text));
    };
    connect(fruitComboBox, &ShadComboBox::currentTextChanged, fruitLabel, updateFruitLabel);
    ExampleI18n::bind(fruitLabel, updateFruitLabel);
    addRow(basicCard, {fruitComboBox, fruitLabel});

    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("Medium 与 Small 两种尺寸"); });
    ShadComboBox* mediumComboBox = createComboBox([] { return QStringLiteral("Medium"); }, fruits, this);
    ShadComboBox* smallComboBox = createComboBox([] { return QStringLiteral("Small"); }, fruits, this);
    smallComboBox->setComboBoxSize(ShadComboBoxType::Small);
    addRow(sizeCard, {mediumComboBox, smallComboBox});

    ShadCard* searchCard = addSection([] { return tr("搜索"); }, [] { return tr("开启搜索后按模糊匹配过滤，并按相关度排序"); });
    const QStringList frameworks = {QStringLiteral("Next.js"), QStringLiteral("SvelteKit"), QStringLiteral("Nuxt.js"), QStringLiteral("Remix"), QStringLiteral("Astro"), QStringLiteral("Angular"), QStringLiteral("Vue.js"), QStringLiteral("React Router"), QStringLiteral("Solid Start"), QStringLiteral("Qwik City"), QStringLiteral("Gatsby"), QStringLiteral("Ember.js")};
    ShadComboBox* frameworkComboBox = createComboBox([] { return tr("选择框架"); }, frameworks, this);
    frameworkComboBox->setSearchEnabled(true);
    ExampleI18n::bind(frameworkComboBox, [frameworkComboBox]()
    {
        frameworkComboBox->setSearchPlaceholderText(tr("搜索框架..."));
        frameworkComboBox->setEmptyText(tr("没有找到框架"));
    });
    const QStringList cities = {QStringLiteral("New York"), QStringLiteral("Los Angeles"), QStringLiteral("Chicago"), QStringLiteral("San Francisco"), QStringLiteral("Seattle"), QStringLiteral("Boston"), QStringLiteral("Houston"), QStringLiteral("Miami"), QStringLiteral("Austin"), QStringLiteral("Las Vegas"), QStringLiteral("Singapore"), QStringLiteral("Bangkok"), QStringLiteral("Kuala Lumpur"), QStringLiteral("Jakarta"), QStringLiteral("Manila"), QStringLiteral("Ho Chi Minh City"), QStringLiteral("Hanoi"), QStringLiteral("Phnom Penh"), QStringLiteral("Yangon"), QStringLiteral("Chiang Mai")};
    ShadComboBox* cityComboBox = createComboBox([] { return tr("选择城市"); }, cities, this);
    cityComboBox->setSearchEnabled(true);
    ExampleI18n::bind(cityComboBox, [cityComboBox]() { cityComboBox->setSearchPlaceholderText(tr("搜索城市...")); });
    addRow(searchCard, {frameworkComboBox, cityComboBox});

    ShadCard* multiCard = addSection([] { return tr("多选"); }, [] { return tr("每项带复选框可多选，已选项以徽章显示"); });
    ShadComboBox* fruitMultiComboBox = createComboBox([] { return tr("选择水果"); }, fruits, this);
    fruitMultiComboBox->setSelectionMode(ShadComboBoxType::MultiSelection);
    fruitMultiComboBox->setCheckedRows({0, 2});
    ShadComboBox* frameworkMultiComboBox = createComboBox([] { return tr("选择框架"); }, frameworks, this);
    frameworkMultiComboBox->setSelectionMode(ShadComboBoxType::MultiSelection);
    frameworkMultiComboBox->setSearchEnabled(true);
    ExampleI18n::bind(frameworkMultiComboBox, [frameworkMultiComboBox]() { frameworkMultiComboBox->setSearchPlaceholderText(tr("搜索框架...")); });
    ShadLabel* multiLabel = new ShadLabel(this);
    multiLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateMultiLabel = [multiLabel, fruitMultiComboBox, frameworkMultiComboBox]()
    {
        const QString fruitText = fruitMultiComboBox->getCheckedTexts().join(tr("、", "列表分隔符"));
        const QString frameworkText = frameworkMultiComboBox->getCheckedTexts().join(tr("、", "列表分隔符"));
        multiLabel->setText(tr("水果: %1    框架: %2").arg(fruitText.isEmpty() ? tr("无") : fruitText, frameworkText.isEmpty() ? tr("无") : frameworkText));
    };
    connect(fruitMultiComboBox, &ShadComboBox::checkedRowsChanged, multiLabel, updateMultiLabel);
    connect(frameworkMultiComboBox, &ShadComboBox::checkedRowsChanged, multiLabel, updateMultiLabel);
    ExampleI18n::bind(multiLabel, updateMultiLabel);
    addRow(multiCard, {fruitMultiComboBox, frameworkMultiComboBox});
    addItem(multiCard, multiLabel);

    ShadCard* separatorCard = addSection([] { return tr("分隔线与禁用项"); }, [] { return tr("插入分隔线，禁用的选项不能选择"); });
    // 选项文字前加国旗 Emoji，由系统的彩色 Emoji 字体绘制
    const QStringList timezones = {QStringLiteral("🇹🇼 Taipei (UTC+8)"), QStringLiteral("🇯🇵 Tokyo (UTC+9)"), QStringLiteral("🇰🇷 Seoul (UTC+9)"), QStringLiteral("🇬🇧 London (UTC+0)"), QStringLiteral("🇫🇷 Paris (UTC+1)"), QStringLiteral("🇺🇸 New York (UTC-5)"), QStringLiteral("🇺🇸 Los Angeles (UTC-8)")};
    ShadComboBox* timezoneComboBox = createComboBox([] { return tr("选择时区"); }, timezones, this);
    timezoneComboBox->insertSeparator(5);
    timezoneComboBox->insertSeparator(3);
    // 两条分隔线都插在 Seoul 之后，它仍位于第 2 行
    setItemEnabled(timezoneComboBox, 2, false);
    addRow(separatorCard, {timezoneComboBox});

    ShadCard* stateCard = addSection([] { return tr("状态"); }, [] { return tr("错误态边框为危险色，禁用后半透明"); });
    ShadComboBox* invalidComboBox = createComboBox([] { return tr("必填项"); }, fruits, this);
    invalidComboBox->setInvalid(true);
    ShadComboBox* disabledComboBox = createComboBox([] { return tr("禁用"); }, fruits, this);
    disabledComboBox->setCurrentIndex(0);
    disabledComboBox->setEnabled(false);
    addRow(stateCard, {invalidComboBox, disabledComboBox});
}

ComboBoxPage::~ComboBoxPage()
{
}
