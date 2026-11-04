#include "TreeSelectPage.h"

#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QStandardItem>
#include <QString>
#include <QStringList>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadTreeSelect.h"

namespace
{
    constexpr int kTreeSelectWidth = 246;
    // 多选触发框更宽，能显示更多徽章
    constexpr int kMultiTreeSelectWidth = 326;

    // 示例数据中后面要用到的节点
    struct RegionItems
    {
        QStandardItem* taipei{nullptr};
        QStandardItem* daan{nullptr};
        QStandardItem* xitun{nullptr};
    };

    // 添加随语言刷新文字的节点
    QStandardItem* addRegion(ShadTreeSelect* treeSelect, ExampleI18n::TextFunction text, QStandardItem* parent)
    {
        QStandardItem* item = treeSelect->addItem(QString(), parent);
        ExampleI18n::bind(treeSelect, [item, text]() { item->setText(text()); });
        return item;
    }

    // 三级地区示例数据
    RegionItems fillRegions(ShadTreeSelect* treeSelect)
    {
        RegionItems items;
        QStandardItem* north = addRegion(treeSelect, [] { return TreeSelectPage::tr("北部"); }, nullptr);
        items.taipei = addRegion(treeSelect, [] { return TreeSelectPage::tr("台北市"); }, north);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("中正区"); }, items.taipei);
        items.daan = addRegion(treeSelect, [] { return TreeSelectPage::tr("大安区"); }, items.taipei);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("信义区"); }, items.taipei);
        QStandardItem* newTaipei = addRegion(treeSelect, [] { return TreeSelectPage::tr("新北市"); }, north);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("板桥区"); }, newTaipei);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("淡水区"); }, newTaipei);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("三重区"); }, newTaipei);
        QStandardItem* central = addRegion(treeSelect, [] { return TreeSelectPage::tr("中部"); }, nullptr);
        QStandardItem* taichung = addRegion(treeSelect, [] { return TreeSelectPage::tr("台中市"); }, central);
        items.xitun = addRegion(treeSelect, [] { return TreeSelectPage::tr("西屯区"); }, taichung);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("北屯区"); }, taichung);
        QStandardItem* changhua = addRegion(treeSelect, [] { return TreeSelectPage::tr("彰化县"); }, central);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("彰化市"); }, changhua);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("鹿港镇"); }, changhua);
        QStandardItem* south = addRegion(treeSelect, [] { return TreeSelectPage::tr("南部"); }, nullptr);
        QStandardItem* kaohsiung = addRegion(treeSelect, [] { return TreeSelectPage::tr("高雄市"); }, south);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("苓雅区"); }, kaohsiung);
        addRegion(treeSelect, [] { return TreeSelectPage::tr("左营区"); }, kaohsiung);
        return items;
    }

    ShadTreeSelect* createTreeSelect(ShadComboBoxType::SelectionMode mode, ExampleI18n::TextFunction placeholder, int width, QWidget* parent)
    {
        ShadTreeSelect* treeSelect = new ShadTreeSelect(mode, parent);
        ExampleI18n::bind(treeSelect, [treeSelect, placeholder]() { treeSelect->setPlaceholderText(placeholder()); });
        treeSelect->setFixedWidth(width);
        return treeSelect;
    }

    ShadLabel* createValueLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 单选的当前值标签，随选择与语言刷新
    void bindCurrentLabel(ShadTreeSelect* treeSelect, ShadLabel* label)
    {
        const auto updateLabel = [treeSelect, label]()
        {
            const QString text = treeSelect->getCurrentText();
            label->setText(text.isEmpty() ? TreeSelectPage::tr("当前选择: 无") : TreeSelectPage::tr("当前选择: %1").arg(text));
        };
        QObject::connect(treeSelect, &ShadTreeSelect::currentIndexChanged, label, updateLabel);
        // 节点文字随语言变化，标签在节点刷新之后重新读取
        ExampleI18n::bind(label, updateLabel);
    }

    // 多选的勾选结果标签，name 为策略名称
    void bindCheckedLabel(ShadTreeSelect* treeSelect, ShadLabel* label, const QString& name)
    {
        const auto updateLabel = [treeSelect, label, name]()
        {
            const QString text = treeSelect->getCheckedTexts().join(TreeSelectPage::tr("、", "列表分隔符"));
            label->setText(QStringLiteral("%1: %2").arg(name, text.isEmpty() ? TreeSelectPage::tr("无") : text));
        };
        QObject::connect(treeSelect, &ShadTreeSelect::checkedIndexesChanged, label, updateLabel);
        ExampleI18n::bind(label, updateLabel);
    }
} // namespace

TreeSelectPage::TreeSelectPage(QWidget* parent)
    : ExampleBasePage([] { return tr("树形选择"); }, [] { return tr("ShadTreeSelect：在下拉的树中选择节点，支持单选、多选、父子勾选联动与搜索。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击节点即选中并关闭弹出层，方向键导航，左右键折叠展开"); });
    ShadTreeSelect* basicTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return tr("选择地区"); }, kTreeSelectWidth, this);
    fillRegions(basicTreeSelect);
    ShadLabel* basicLabel = createValueLabel(this);
    bindCurrentLabel(basicTreeSelect, basicLabel);
    addRow(basicCard, {basicTreeSelect, basicLabel});

    ShadCard* leafCard = addSection([] { return tr("只选叶子节点"); }, [] { return tr("开启 leafOnly 后点击地区、县市只展开或折叠，触发框显示完整路径"); });
    ShadTreeSelect* leafTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return tr("选择区县"); }, kTreeSelectWidth, this);
    const RegionItems leafItems = fillRegions(leafTreeSelect);
    leafTreeSelect->setLeafOnly(true);
    leafTreeSelect->setFullPathVisible(true);
    leafTreeSelect->setCurrentIndex(leafItems.daan->index());
    ShadLabel* leafLabel = createValueLabel(this);
    bindCurrentLabel(leafTreeSelect, leafLabel);
    addRow(leafCard, {leafTreeSelect, leafLabel});

    ShadCard* searchCard = addSection([] { return tr("搜索"); }, [] { return tr("按模糊匹配过滤，匹配节点的祖先自动展开，匹配节点的下级全部保留"); });
    ShadTreeSelect* searchTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return tr("选择地区"); }, kTreeSelectWidth, this);
    fillRegions(searchTreeSelect);
    searchTreeSelect->setSearchEnabled(true);
    ExampleI18n::bind(searchTreeSelect, [searchTreeSelect]()
    {
        searchTreeSelect->setSearchPlaceholderText(tr("搜索地区..."));
        searchTreeSelect->setEmptyText(tr("没有找到地区"));
    });
    ShadLabel* searchLabel = createValueLabel(this);
    bindCurrentLabel(searchTreeSelect, searchLabel);
    addRow(searchCard, {searchTreeSelect, searchLabel});

    ShadCard* multiCard = addSection([] { return tr("多选"); }, [] { return tr("勾选父节点同步全部下级，三种回显策略对比：ShowChild 只显示叶子节点，ShowParent 下级全选时只显示父节点，ShowAll 显示全部勾选节点"); });
    const QList<ShadTreeSelectType::CheckedStrategy> strategies = {ShadTreeSelectType::ShowChild, ShadTreeSelectType::ShowParent, ShadTreeSelectType::ShowAll};
    const QStringList strategyNames = {QStringLiteral("ShowChild"), QStringLiteral("ShowParent"), QStringLiteral("ShowAll")};
    for (int index = 0; index < int(strategies.size()); ++index)
    {
        ShadTreeSelect* multiTreeSelect = createTreeSelect(ShadComboBoxType::MultiSelection, [] { return tr("选择地区"); }, kMultiTreeSelectWidth, this);
        const RegionItems multiItems = fillRegions(multiTreeSelect);
        multiTreeSelect->setCheckedStrategy(strategies.at(index));
        // 第一个开启搜索，演示多选与搜索组合
        if (index == 0)
        {
            multiTreeSelect->setSearchEnabled(true);
            ExampleI18n::bind(multiTreeSelect, [multiTreeSelect]() { multiTreeSelect->setSearchPlaceholderText(tr("搜索地区...")); });
        }
        multiTreeSelect->setIndexChecked(multiItems.taipei->index(), true);
        multiTreeSelect->setIndexChecked(multiItems.xitun->index(), true);
        ShadLabel* multiLabel = createValueLabel(this);
        bindCheckedLabel(multiTreeSelect, multiLabel, strategyNames.at(index));
        addRow(multiCard, {multiTreeSelect, multiLabel});
    }

    ShadCard* stateCard = addSection([] { return tr("尺寸与状态"); }, [] { return tr("Small 尺寸为 24px，错误态边框为危险色，禁用后半透明"); });
    ShadTreeSelect* smallTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return QStringLiteral("Small"); }, kTreeSelectWidth, this);
    fillRegions(smallTreeSelect);
    smallTreeSelect->setTreeSelectSize(ShadComboBoxType::Small);
    ShadTreeSelect* invalidTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return tr("必填项"); }, kTreeSelectWidth, this);
    fillRegions(invalidTreeSelect);
    invalidTreeSelect->setInvalid(true);
    ShadTreeSelect* disabledTreeSelect = createTreeSelect(ShadComboBoxType::SingleSelection, [] { return tr("禁用"); }, kTreeSelectWidth, this);
    const RegionItems disabledItems = fillRegions(disabledTreeSelect);
    disabledTreeSelect->setCurrentIndex(disabledItems.taipei->index());
    disabledTreeSelect->setEnabled(false);
    addRow(stateCard, {smallTreeSelect, invalidTreeSelect, disabledTreeSelect});
}

TreeSelectPage::~TreeSelectPage()
{
}
