#include "TreeViewPage.h"

#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QList>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTreeView.h"

namespace
{
    // 各示例中树形视图的高度，超出时在内部滚动
    constexpr int kTreeHeight = 300;
    constexpr int kShortTreeHeight = 220;
    // 多列示例的列宽
    constexpr int kNameColumnWidth = 280;
    constexpr int kSizeColumnWidth = 100;

    // 文字由调用方按当前状态绑定
    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    QStandardItem* createItem(const QString& text, ShadIconType::IconName icon)
    {
        QStandardItem* item = new QStandardItem(text);
        item->setData(static_cast<int>(icon), ShadTreeView::IconRole);
        item->setEditable(false);
        return item;
    }

    // 树形视图上方的操作按钮：outline、sm、带图标
    ShadButton* createToolButton(ExampleI18n::TextFunction text, ShadIconType::IconName icon, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        button->setButtonSize(ShadButtonType::Small);
        button->setShadIcon(icon);
        return button;
    }

    ShadTreeView* createTree(int height, QWidget* parent)
    {
        ShadTreeView* tree = new ShadTreeView(parent);
        tree->setFixedHeight(height);
        return tree;
    }

    // 项目目录结构
    QStandardItemModel* createFileModel(QObject* parent)
    {
        QStandardItemModel* model = new QStandardItemModel(parent);
        QStandardItem* library = createItem(QStringLiteral("ShadcnWidgetTools"), ShadIconType::Folder);
        QStandardItem* internalFolder = createItem(QStringLiteral("internal"), ShadIconType::Folder);
        internalFolder->appendRow(createItem(QStringLiteral("ShadStyleHelper.h"), ShadIconType::FileCode));
        internalFolder->appendRow(createItem(QStringLiteral("ShadStyleHelper.cpp"), ShadIconType::FileCode));
        QStandardItem* privateFolder = createItem(QStringLiteral("private"), ShadIconType::Folder);
        privateFolder->appendRow(createItem(QStringLiteral("ShadButtonPrivate.h"), ShadIconType::FileCode));
        QStandardItem* resourceFolder = createItem(QStringLiteral("Resource"), ShadIconType::Folder);
        resourceFolder->appendRow(createItem(QStringLiteral("lucide.ttf"), ShadIconType::File));
        library->appendRow(internalFolder);
        library->appendRow(privateFolder);
        library->appendRow(resourceFolder);
        library->appendRow(createItem(QStringLiteral("ShadButton.h"), ShadIconType::FileCode));
        library->appendRow(createItem(QStringLiteral("ShadButton.cpp"), ShadIconType::FileCode));
        library->appendRow(createItem(QStringLiteral("CMakeLists.txt"), ShadIconType::File));
        QStandardItem* example = createItem(QStringLiteral("ShadcnWidgetToolsExample"), ShadIconType::Folder);
        example->appendRow(createItem(QStringLiteral("main.cpp"), ShadIconType::FileCode));
        example->appendRow(createItem(QStringLiteral("MainWindow.cpp"), ShadIconType::FileCode));
        model->appendRow(library);
        model->appendRow(example);
        model->appendRow(createItem(QStringLiteral("README.md"), ShadIconType::FileText));
        model->appendRow(createItem(QStringLiteral("LICENSE"), ShadIconType::File));
        return model;
    }

    // 从根节点到 index 的路径，例如 "ShadcnWidgetTools / internal / ShadStyleHelper.h"
    QString itemPath(const QModelIndex& index)
    {
        QStringList parts;
        for (QModelIndex current = index.siblingAtColumn(0); current.isValid(); current = current.parent())
        {
            parts.prepend(current.data(Qt::DisplayRole).toString());
        }
        return parts.join(QStringLiteral(" / "));
    }

    // 权限分组：父节点自动三态，子节点可勾选；切换语言时只更新文字，勾选状态不变
    void appendPermissionGroup(QStandardItemModel* model, ExampleI18n::TextFunction text, ShadIconType::IconName icon, const QList<ExampleI18n::TextFunction>& permissions, const QList<int>& checkedRows)
    {
        QStandardItem* group = createItem(QString(), icon);
        group->setCheckable(true);
        group->setAutoTristate(true);
        for (int row = 0; row < permissions.size(); ++row)
        {
            QStandardItem* item = new QStandardItem();
            item->setEditable(false);
            item->setCheckable(true);
            group->appendRow(item);
        }
        model->appendRow(group);
        ExampleI18n::bind(model, [group, text, permissions]()
        {
            group->setText(text());
            for (int row = 0; row < permissions.size(); ++row)
            {
                group->child(row)->setText(permissions.at(row)());
            }
        });
        for (const int row : checkedRows)
        {
            group->child(row)->setCheckState(Qt::Checked);
        }
        // 初始状态由代码设置，父节点按子节点设为选中、半选或未选中；之后的点击由 ShadTreeView 联动
        Qt::CheckState groupState = Qt::PartiallyChecked;
        if (checkedRows.isEmpty())
        {
            groupState = Qt::Unchecked;
        }
        else if (checkedRows.size() == permissions.size())
        {
            groupState = Qt::Checked;
        }
        group->setCheckState(groupState);
    }

    // 文件列表的一行：名称、大小、修改时间；名称随语言刷新，绑定由 model 持有
    QList<QStandardItem*> createFileRow(QStandardItemModel* model, ExampleI18n::TextFunction name, ShadIconType::IconName icon, const QString& size, const QString& modified)
    {
        QStandardItem* nameItem = createItem(QString(), icon);
        ExampleI18n::bind(model, [nameItem, name]() { nameItem->setText(name()); });
        QStandardItem* sizeItem = new QStandardItem(size);
        sizeItem->setEditable(false);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QStandardItem* modifiedItem = new QStandardItem(modified);
        modifiedItem->setEditable(false);
        return {nameItem, sizeItem, modifiedItem};
    }
} // namespace

TreeViewPage::TreeViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("树形视图"); }, [] { return tr("ShadTreeView：继承 QTreeView 的树形视图，支持图标、复选框、多列与拖放。"); }, parent)
{
    // 基础用法
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("IconRole 指定 Lucide 图标，文件夹展开时自动切换图标"); });
    ShadTreeView* fileTree = createTree(kTreeHeight, this);
    fileTree->setModel(createFileModel(fileTree));
    fileTree->expand(fileTree->model()->index(0, 0));
    ShadLabel* pathLabel = createMutedText(this);
    const auto updatePathLabel = [fileTree, pathLabel]()
    {
        const QModelIndex current = fileTree->selectionModel()->currentIndex();
        pathLabel->setText(current.isValid() ? itemPath(current) : tr("未选择"));
    };
    connect(fileTree->selectionModel(), &QItemSelectionModel::currentChanged, pathLabel, updatePathLabel);
    ExampleI18n::bind(pathLabel, updatePathLabel);
    addStretchItem(basicCard, fileTree);
    addItem(basicCard, pathLabel);

    // 复选框
    ShadCard* checkCard = addSection([] { return tr("复选框"); }, [] { return tr("勾选父节点全选子节点，子节点部分勾选时父节点半选"); });
    ShadTreeView* permissionTree = createTree(kTreeHeight, this);
    QStandardItemModel* permissionModel = new QStandardItemModel(permissionTree);
    appendPermissionGroup(permissionModel, [] { return tr("用户管理"); }, ShadIconType::Users, {[] { return tr("查看用户"); }, [] { return tr("编辑用户"); }, [] { return tr("删除用户"); }}, {0, 1});
    appendPermissionGroup(permissionModel, [] { return tr("内容管理"); }, ShadIconType::FileText, {[] { return tr("发布文章"); }, [] { return tr("审核评论"); }}, {0, 1});
    appendPermissionGroup(permissionModel, [] { return tr("系统设置"); }, ShadIconType::Settings, {[] { return tr("修改主题"); }, [] { return tr("备份数据"); }}, {});
    permissionTree->setModel(permissionModel);
    permissionTree->expandAll();
    // 全部展开 / 全部折叠：QTreeView::expandAll 与 collapseAll
    ShadButton* expandAllButton = createToolButton([] { return tr("全部展开"); }, ShadIconType::ChevronsUpDown, this);
    ShadButton* collapseAllButton = createToolButton([] { return tr("全部折叠"); }, ShadIconType::ChevronsDownUp, this);
    connect(expandAllButton, &ShadButton::clicked, permissionTree, &ShadTreeView::expandAll);
    connect(collapseAllButton, &ShadButton::clicked, permissionTree, &ShadTreeView::collapseAll);
    addRow(checkCard, {expandAllButton, collapseAllButton});
    addStretchItem(checkCard, permissionTree);

    // 多列与表头
    ShadCard* columnsCard = addSection([] { return tr("多列与表头"); }, [] { return tr("显示表头后与表格的表头外观一致"); });
    ShadTreeView* columnTree = createTree(kTreeHeight, this);
    QStandardItemModel* columnModel = new QStandardItemModel(0, 3, columnTree);
    ExampleI18n::bind(columnModel, [columnModel]()
    {
        columnModel->setHeaderData(0, Qt::Horizontal, tr("名称"));
        columnModel->setHeaderData(1, Qt::Horizontal, tr("大小"));
        columnModel->setHeaderData(2, Qt::Horizontal, tr("修改时间"));
    });
    QList<QStandardItem*> documents = createFileRow(columnModel, [] { return tr("文档"); }, ShadIconType::Folder, QString(), QStringLiteral("2026-10-20"));
    documents.first()->appendRow(createFileRow(columnModel, [] { return tr("季度报告.pdf"); }, ShadIconType::FileText, QStringLiteral("2.4 MB"), QStringLiteral("2026-10-18")));
    documents.first()->appendRow(createFileRow(columnModel, [] { return tr("会议纪要.docx"); }, ShadIconType::FileText, QStringLiteral("86 KB"), QStringLiteral("2026-10-12")));
    QList<QStandardItem*> pictures = createFileRow(columnModel, [] { return tr("图片"); }, ShadIconType::Folder, QString(), QStringLiteral("2026-10-22"));
    pictures.first()->appendRow(createFileRow(columnModel, [] { return tr("封面.png"); }, ShadIconType::Image, QStringLiteral("1.1 MB"), QStringLiteral("2026-10-22")));
    columnModel->appendRow(documents);
    columnModel->appendRow(pictures);
    columnModel->appendRow(createFileRow(columnModel, [] { return tr("安装包.zip"); }, ShadIconType::Package, QStringLiteral("48 MB"), QStringLiteral("2026-09-30")));
    columnTree->setModel(columnModel);
    columnTree->setHeaderHidden(false);
    columnTree->setColumnWidth(0, kNameColumnWidth);
    columnTree->setColumnWidth(1, kSizeColumnWidth);
    columnTree->expandAll();
    addStretchItem(columnsCard, columnTree);

    // 拖放排序
    ShadCard* dragCard = addSection([] { return tr("拖放排序"); }, [] { return tr("InternalMove：拖动节点调整顺序或移入文件夹"); });
    ShadTreeView* dragTree = createTree(kTreeHeight, this);
    dragTree->setModel(createFileModel(dragTree));
    dragTree->setDragDropMode(QAbstractItemView::InternalMove);
    dragTree->setDefaultDropAction(Qt::MoveAction);
    dragTree->setDropIndicatorShown(true);
    dragTree->expandAll();
    addStretchItem(dragCard, dragTree);

    // 样式选项
    ShadCard* styleCard = addSection([] { return tr("样式选项"); }, [] { return tr("关闭缩进线，行高改为 28"); });
    ShadTreeView* compactTree = createTree(kShortTreeHeight, this);
    compactTree->setModel(createFileModel(compactTree));
    compactTree->setIndentGuidesVisible(false);
    compactTree->setItemHeight(28);
    compactTree->expandAll();
    addStretchItem(styleCard, compactTree);

    // RTL
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左时缩进、箭头与图标位于右侧"); });
    ShadTreeView* rtlTree = createTree(kShortTreeHeight, this);
    rtlTree->setLayoutDirection(Qt::RightToLeft);
    rtlTree->setModel(createFileModel(rtlTree));
    rtlTree->expand(rtlTree->model()->index(0, 0));
    addStretchItem(rtlCard, rtlTree);
}

TreeViewPage::~TreeViewPage()
{
}
