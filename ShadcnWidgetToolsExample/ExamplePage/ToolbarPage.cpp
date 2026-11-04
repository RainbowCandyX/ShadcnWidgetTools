#include "ToolbarPage.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSlider.h"
#include "ShadToolbar.h"

namespace
{
    using LastAction = std::shared_ptr<QPointer<QAction>>;

    void updateStatusText(ShadLabel* label, const LastAction& lastAction)
    {
        const QAction* action = lastAction->data();
        label->setText(action != nullptr ? QCoreApplication::translate("ToolbarPage", "最近点击: %1").arg(action->text()) : QCoreApplication::translate("ToolbarPage", "最近点击: 无"));
    }

    ShadLabel* createStatusLabel(QWidget* parent, const QList<QAction*>& actions)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        const LastAction lastAction = std::make_shared<QPointer<QAction>>();
        for (QAction* action : actions)
        {
            QObject::connect(action, &QAction::triggered, label, [label, action, lastAction]()
            {
                *lastAction = action;
                updateStatusText(label, lastAction);
            });
        }
        ExampleI18n::bind(label, [label, lastAction]() { updateStatusText(label, lastAction); });
        return label;
    }

    QAction* addCheckableAction(ShadToolbar* toolbar, ShadIconType::IconName icon)
    {
        QAction* action = toolbar->addAction(icon, QString());
        action->setCheckable(true);
        return action;
    }
} // namespace

ToolbarPage::ToolbarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("工具栏"); }, [] { return tr("ShadToolbar：放置常用操作的工具栏。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("按钮、勾选、下拉菜单与分隔线"); });
    ShadToolbar* basicToolbar = new ShadToolbar(this);
    basicToolbar->setBordered(true);
    QAction* undoAction = basicToolbar->addAction(ShadIconType::Undo2, QString());
    QAction* redoAction = basicToolbar->addAction(ShadIconType::Redo2, QString());
    basicToolbar->addSeparator();
    QAction* boldAction = addCheckableAction(basicToolbar, ShadIconType::Bold);
    QAction* italicAction = addCheckableAction(basicToolbar, ShadIconType::Italic);
    QAction* underlineAction = addCheckableAction(basicToolbar, ShadIconType::Underline);
    basicToolbar->addSeparator();
    ShadDropdownMenu* alignMenu = basicToolbar->addMenu(ShadIconType::AlignLeft, QString());
    QActionGroup* alignGroup = new QActionGroup(alignMenu);
    QAction* alignLeftAction = alignMenu->addRadioItem(QString(), alignGroup, true);
    QAction* alignCenterAction = alignMenu->addRadioItem(QString(), alignGroup);
    QAction* alignRightAction = alignMenu->addRadioItem(QString(), alignGroup);
    basicToolbar->addStretch();
    QAction* saveAction = basicToolbar->addAction(ShadIconType::Save, QString());
    ExampleI18n::bind(basicToolbar, [basicToolbar, undoAction, redoAction, boldAction, italicAction, underlineAction, alignMenu, alignLeftAction, alignCenterAction, alignRightAction, saveAction]()
    {
        undoAction->setText(tr("撤销"));
        redoAction->setText(tr("重做"));
        boldAction->setText(tr("加粗"));
        italicAction->setText(tr("斜体"));
        underlineAction->setText(tr("下划线"));
        basicToolbar->setMenuTitle(alignMenu, tr("对齐"));
        alignLeftAction->setText(tr("左对齐"));
        alignCenterAction->setText(tr("居中"));
        alignRightAction->setText(tr("右对齐"));
        saveAction->setText(tr("保存"));
    });
    addStretchItem(basicCard, basicToolbar);
    addItem(basicCard, createStatusLabel(this, {undoAction, redoAction, boldAction, italicAction, underlineAction, alignLeftAction, alignCenterAction, alignRightAction, saveAction}));

    ShadCard* textCard = addSection([] { return tr("显示文字"); }, [] { return tr("图标旁显示文字"); });
    ShadToolbar* textToolbar = new ShadToolbar(this);
    textToolbar->setButtonStyle(ShadToolbarType::TextBesideIcon);
    textToolbar->setBordered(true);
    QAction* newAction = textToolbar->addAction(ShadIconType::FilePlus, QString());
    QAction* openAction = textToolbar->addAction(ShadIconType::FolderOpen, QString());
    QAction* saveFileAction = textToolbar->addAction(ShadIconType::Save, QString());
    textToolbar->addSeparator();
    ShadDropdownMenu* shareMenu = textToolbar->addMenu(ShadIconType::Share2, QString());
    QAction* mailAction = shareMenu->addItem(QString(), ShadIconType::Mail);
    QAction* linkAction = shareMenu->addItem(QString(), ShadIconType::Link);
    ExampleI18n::bind(textToolbar, [textToolbar, newAction, openAction, saveFileAction, shareMenu, mailAction, linkAction]()
    {
        newAction->setText(tr("新建"));
        openAction->setText(tr("打开"));
        saveFileAction->setText(tr("保存"));
        textToolbar->setMenuTitle(shareMenu, tr("分享"));
        mailAction->setText(tr("邮件"));
        linkAction->setText(tr("复制链接"));
    });
    addItem(textCard, textToolbar);
    addItem(textCard, createStatusLabel(this, {newAction, openAction, saveFileAction, mailAction, linkAction}));

    ShadCard* verticalCard = addSection([] { return tr("纵向"); }, [] { return tr("纵向排列，勾选项可以互斥"); });
    ShadToolbar* toolsToolbar = new ShadToolbar(Qt::Vertical, this);
    toolsToolbar->setBordered(true);
    QActionGroup* toolGroup = new QActionGroup(toolsToolbar);
    QList<QAction*> toolActions;
    for (const ShadIconType::IconName icon : {ShadIconType::MousePointer2, ShadIconType::Hand, ShadIconType::Pencil, ShadIconType::Eraser, ShadIconType::Type})
    {
        QAction* action = addCheckableAction(toolsToolbar, icon);
        toolGroup->addAction(action);
        toolActions.append(action);
    }
    toolActions.first()->setChecked(true);
    toolsToolbar->addSeparator();
    QAction* zoomInAction = toolsToolbar->addAction(ShadIconType::ZoomIn, QString());
    QAction* zoomOutAction = toolsToolbar->addAction(ShadIconType::ZoomOut, QString());
    ShadToolbar* editToolbar = new ShadToolbar(Qt::Vertical, this);
    editToolbar->setButtonStyle(ShadToolbarType::TextBesideIcon);
    editToolbar->setBordered(true);
    QAction* copyAction = editToolbar->addAction(ShadIconType::Copy, QString());
    QAction* cutAction = editToolbar->addAction(ShadIconType::Scissors, QString());
    QAction* pasteAction = editToolbar->addAction(ShadIconType::ClipboardPaste, QString());
    editToolbar->addSeparator();
    QAction* deleteAction = editToolbar->addAction(ShadIconType::Trash2, QString());
    ExampleI18n::bind(toolsToolbar, [toolActions, zoomInAction, zoomOutAction, copyAction, cutAction, pasteAction, deleteAction]()
    {
        toolActions.at(0)->setText(tr("选择"));
        toolActions.at(1)->setText(tr("平移"));
        toolActions.at(2)->setText(tr("画笔"));
        toolActions.at(3)->setText(tr("橡皮擦"));
        toolActions.at(4)->setText(tr("文字"));
        zoomInAction->setText(tr("放大"));
        zoomOutAction->setText(tr("缩小"));
        copyAction->setText(tr("复制"));
        cutAction->setText(tr("剪切"));
        pasteAction->setText(tr("粘贴"));
        deleteAction->setText(tr("删除"));
    });
    addRow(verticalCard, {toolsToolbar, editToolbar});
    addItem(verticalCard, createStatusLabel(this, toolActions + QList<QAction*>{zoomInAction, zoomOutAction, copyAction, cutAction, pasteAction, deleteAction}));

    ShadCard* overflowCard = addSection([] { return tr("溢出"); }, [] { return tr("宽度不够时收进更多菜单，拖动滑块调整宽度"); });
    ShadToolbar* overflowToolbar = new ShadToolbar(this);
    overflowToolbar->setBordered(true);
    QAction* overflowUndoAction = overflowToolbar->addAction(ShadIconType::Undo2, QString());
    QAction* overflowRedoAction = overflowToolbar->addAction(ShadIconType::Redo2, QString());
    overflowToolbar->addSeparator();
    QAction* overflowBoldAction = addCheckableAction(overflowToolbar, ShadIconType::Bold);
    QAction* overflowItalicAction = addCheckableAction(overflowToolbar, ShadIconType::Italic);
    QAction* overflowUnderlineAction = addCheckableAction(overflowToolbar, ShadIconType::Underline);
    overflowToolbar->addSeparator();
    QAction* overflowCopyAction = overflowToolbar->addAction(ShadIconType::Copy, QString());
    QAction* overflowCutAction = overflowToolbar->addAction(ShadIconType::Scissors, QString());
    QAction* overflowPasteAction = overflowToolbar->addAction(ShadIconType::ClipboardPaste, QString());
    overflowToolbar->addSeparator();
    QAction* overflowSearchAction = overflowToolbar->addAction(ShadIconType::Search, QString());
    QAction* overflowPrintAction = overflowToolbar->addAction(ShadIconType::Printer, QString());
    QAction* overflowSettingsAction = overflowToolbar->addAction(ShadIconType::Settings, QString());
    ExampleI18n::bind(overflowToolbar, [overflowUndoAction, overflowRedoAction, overflowBoldAction, overflowItalicAction, overflowUnderlineAction, overflowCopyAction, overflowCutAction, overflowPasteAction, overflowSearchAction, overflowPrintAction, overflowSettingsAction]()
    {
        overflowUndoAction->setText(tr("撤销"));
        overflowRedoAction->setText(tr("重做"));
        overflowBoldAction->setText(tr("加粗"));
        overflowItalicAction->setText(tr("斜体"));
        overflowUnderlineAction->setText(tr("下划线"));
        overflowCopyAction->setText(tr("复制"));
        overflowCutAction->setText(tr("剪切"));
        overflowPasteAction->setText(tr("粘贴"));
        overflowSearchAction->setText(tr("搜索"));
        overflowPrintAction->setText(tr("打印"));
        overflowSettingsAction->setText(tr("设置"));
    });
    constexpr int kOverflowInitialWidth = 240;
    overflowToolbar->setFixedWidth(kOverflowInitialWidth);
    ShadSlider* widthSlider = new ShadSlider(Qt::Horizontal, this);
    widthSlider->setRange(120, 520);
    widthSlider->setValue(kOverflowInitialWidth);
    widthSlider->setFixedWidth(200);
    connect(widthSlider, &ShadSlider::valueChanged, overflowToolbar, [overflowToolbar](int value) { overflowToolbar->setFixedWidth(value); });
    addItem(overflowCard, widthSlider);
    addItem(overflowCard, overflowToolbar);
    addItem(overflowCard, createStatusLabel(this, {overflowUndoAction, overflowRedoAction, overflowBoldAction, overflowItalicAction, overflowUnderlineAction, overflowCopyAction, overflowCutAction, overflowPasteAction, overflowSearchAction, overflowPrintAction, overflowSettingsAction}));
}

ToolbarPage::~ToolbarPage()
{
}
