#include "MenubarPage.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QKeySequence>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadLabel.h"
#include "ShadMenubar.h"
#include "ShadSwitch.h"

namespace
{
    // 最近点击的菜单项，状态文字按它的当前文字生成
    using LastAction = std::shared_ptr<QPointer<QAction>>;

    QString shortcut(const QKeySequence& sequence)
    {
        return ShadKbdGroup::shortcutText(sequence);
    }

    // 系统菜单栏只显示 QAction::shortcut；文字由调用方绑定
    QAction* addShortcutItem(ShadDropdownMenu* menu, const QKeySequence& sequence)
    {
        QAction* action = menu->addItem(QString(), ShadIconType::None, shortcut(sequence));
        action->setShortcut(sequence);
        return action;
    }

    void updateStatusText(ShadLabel* label, const LastAction& lastAction)
    {
        const QAction* action = lastAction->data();
        label->setText(action != nullptr ? QCoreApplication::translate("MenubarPage", "最近点击: %1").arg(action->text()) : QCoreApplication::translate("MenubarPage", "最近点击: 无"));
    }

    // 连接 QAction::triggered，系统菜单栏中点击同样生效
    void trackTriggered(ShadDropdownMenu* menu, ShadLabel* label, const LastAction& lastAction)
    {
        for (QAction* action : menu->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly))
        {
            QObject::connect(action, &QAction::triggered, label, [label, action, lastAction]()
            {
                *lastAction = action;
                updateStatusText(label, lastAction);
            });
        }
    }

    ShadLabel* createStatusLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

MenubarPage::MenubarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("菜单栏"); }, [] { return tr("ShadMenubar：常驻的菜单栏。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("子菜单、复选与单选"); });
    ShadMenubar* menubar = new ShadMenubar(this);
    ShadLabel* basicStatus = createStatusLabel(this);
    const LastAction lastAction = std::make_shared<QPointer<QAction>>();

    ShadDropdownMenu* fileMenu = menubar->addMenu(QString());
    QAction* newTabAction = addShortcutItem(fileMenu, QKeySequence(Qt::CTRL | Qt::Key_T));
    QAction* newWindowAction = addShortcutItem(fileMenu, QKeySequence(Qt::CTRL | Qt::Key_N));
    QAction* incognitoAction = fileMenu->addItem(QString());
    incognitoAction->setEnabled(false);
    fileMenu->addSeparator();
    ShadDropdownMenu* shareMenu = fileMenu->addMenu(QString());
    QAction* emailAction = shareMenu->addItem(QString());
    QAction* messagesAction = shareMenu->addItem(QString());
    QAction* notesAction = shareMenu->addItem(QString());
    fileMenu->addSeparator();
    QAction* printAction = addShortcutItem(fileMenu, QKeySequence(Qt::CTRL | Qt::Key_P));
    ExampleI18n::bind(fileMenu, [newTabAction, newWindowAction, incognitoAction, shareMenu, emailAction, messagesAction, notesAction, printAction]()
    {
        newTabAction->setText(tr("新建标签页"));
        newWindowAction->setText(tr("新建窗口"));
        incognitoAction->setText(tr("新建无痕窗口"));
        shareMenu->menuAction()->setText(tr("分享"));
        emailAction->setText(tr("邮件链接"));
        messagesAction->setText(tr("消息"));
        notesAction->setText(tr("备忘录"));
        printAction->setText(tr("打印…"));
    });
    trackTriggered(fileMenu, basicStatus, lastAction);
    trackTriggered(shareMenu, basicStatus, lastAction);

    ShadDropdownMenu* editMenu = menubar->addMenu(QString());
    QAction* undoAction = addShortcutItem(editMenu, QKeySequence(Qt::CTRL | Qt::Key_Z));
    QAction* redoAction = addShortcutItem(editMenu, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    editMenu->addSeparator();
    ShadDropdownMenu* findMenu = editMenu->addMenu(QString());
    QAction* searchWebAction = findMenu->addItem(QString());
    findMenu->addSeparator();
    QAction* findAction = findMenu->addItem(QString());
    QAction* findNextAction = findMenu->addItem(QString());
    QAction* findPreviousAction = findMenu->addItem(QString());
    editMenu->addSeparator();
    QAction* cutAction = editMenu->addItem(QString());
    QAction* copyAction = editMenu->addItem(QString());
    QAction* pasteAction = editMenu->addItem(QString());
    ExampleI18n::bind(editMenu, [undoAction, redoAction, findMenu, searchWebAction, findAction, findNextAction, findPreviousAction, cutAction, copyAction, pasteAction]()
    {
        undoAction->setText(tr("撤销"));
        redoAction->setText(tr("重做"));
        findMenu->menuAction()->setText(tr("查找"));
        searchWebAction->setText(tr("搜索网页"));
        findAction->setText(tr("查找…"));
        findNextAction->setText(tr("查找下一个"));
        findPreviousAction->setText(tr("查找上一个"));
        cutAction->setText(tr("剪切"));
        copyAction->setText(tr("复制"));
        pasteAction->setText(tr("粘贴"));
    });
    trackTriggered(editMenu, basicStatus, lastAction);
    trackTriggered(findMenu, basicStatus, lastAction);

    ShadDropdownMenu* viewMenu = menubar->addMenu(QString());
    QAction* bookmarksAction = viewMenu->addCheckableItem(QString());
    QAction* fullUrlAction = viewMenu->addCheckableItem(QString(), true);
    viewMenu->addSeparator();
    QAction* reloadAction = addShortcutItem(viewMenu, QKeySequence(Qt::CTRL | Qt::Key_R));
    viewMenu->setItemInset(reloadAction, true);
    QAction* forceReloadAction = addShortcutItem(viewMenu, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R));
    forceReloadAction->setEnabled(false);
    viewMenu->setItemInset(forceReloadAction, true);
    viewMenu->addSeparator();
    QAction* fullscreenAction = viewMenu->addItem(QString());
    viewMenu->setItemInset(fullscreenAction, true);
    viewMenu->addSeparator();
    QAction* sidebarAction = viewMenu->addItem(QString());
    viewMenu->setItemInset(sidebarAction, true);
    ExampleI18n::bind(viewMenu, [bookmarksAction, fullUrlAction, reloadAction, forceReloadAction, fullscreenAction, sidebarAction]()
    {
        bookmarksAction->setText(tr("始终显示书签栏"));
        fullUrlAction->setText(tr("始终显示完整网址"));
        reloadAction->setText(tr("重新加载"));
        forceReloadAction->setText(tr("强制重新加载"));
        fullscreenAction->setText(tr("切换全屏"));
        sidebarAction->setText(tr("隐藏侧栏"));
    });
    trackTriggered(viewMenu, basicStatus, lastAction);

    ShadDropdownMenu* profileMenu = menubar->addMenu(QString());
    QActionGroup* profileGroup = new QActionGroup(profileMenu);
    QAction* firstProfileAction = profileMenu->addRadioItem(QString(), profileGroup);
    QAction* secondProfileAction = profileMenu->addRadioItem(QString(), profileGroup, true);
    QAction* thirdProfileAction = profileMenu->addRadioItem(QString(), profileGroup);
    profileMenu->addSeparator();
    QAction* editProfileAction = profileMenu->addItem(QString());
    profileMenu->setItemInset(editProfileAction, true);
    profileMenu->addSeparator();
    QAction* addProfileAction = profileMenu->addItem(QString());
    profileMenu->setItemInset(addProfileAction, true);
    ExampleI18n::bind(profileMenu, [firstProfileAction, secondProfileAction, thirdProfileAction, editProfileAction, addProfileAction]()
    {
        firstProfileAction->setText(tr("张三"));
        secondProfileAction->setText(tr("李四"));
        thirdProfileAction->setText(tr("王五"));
        editProfileAction->setText(tr("编辑…"));
        addProfileAction->setText(tr("添加账户…"));
    });
    trackTriggered(profileMenu, basicStatus, lastAction);
    ExampleI18n::bind(menubar, [menubar, fileMenu, editMenu, viewMenu, profileMenu]()
    {
        menubar->setMenuTitle(fileMenu, tr("文件"));
        menubar->setMenuTitle(editMenu, tr("编辑"));
        menubar->setMenuTitle(viewMenu, tr("视图"));
        menubar->setMenuTitle(profileMenu, tr("账户"));
    });
    // 菜单项文字的绑定先连接，切换语言时状态文字取到的是新的菜单项文字
    ExampleI18n::bind(basicStatus, [basicStatus, lastAction]() { updateStatusText(basicStatus, lastAction); });
    addRow(basicCard, {menubar, basicStatus});
#ifdef Q_OS_MACOS
    ShadSwitch* nativeSwitch = new ShadSwitch(this);
    ExampleI18n::bind(nativeSwitch, [nativeSwitch]() { nativeSwitch->setText(tr("显示在 macOS 菜单栏")); });
    connect(nativeSwitch, &ShadSwitch::toggled, menubar, &ShadMenubar::setNativeMenuBar);
    addRow(basicCard, {nativeSwitch});
#endif

    ShadCard* iconCard = addSection([] { return tr("图标"); }, [] { return tr("带图标与危险操作"); });
    ShadMenubar* iconMenubar = new ShadMenubar(this);
    ShadDropdownMenu* projectMenu = iconMenubar->addMenu(QString());
    QAction* newFileAction = projectMenu->addItem(QString(), ShadIconType::File, shortcut(QKeySequence(Qt::CTRL | Qt::Key_N)));
    QAction* newFolderAction = projectMenu->addItem(QString(), ShadIconType::Folder, shortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N)));
    projectMenu->addSeparator();
    QAction* saveAction = projectMenu->addItem(QString(), ShadIconType::Save, shortcut(QKeySequence(Qt::CTRL | Qt::Key_S)));
    projectMenu->addSeparator();
    QAction* deleteAction = projectMenu->addItem(QString(), ShadIconType::Trash2);
    projectMenu->setItemDestructive(deleteAction, true);
    ExampleI18n::bind(projectMenu, [newFileAction, newFolderAction, saveAction, deleteAction]()
    {
        newFileAction->setText(tr("新建文件"));
        newFolderAction->setText(tr("新建文件夹"));
        saveAction->setText(tr("保存"));
        deleteAction->setText(tr("删除"));
    });
    ShadDropdownMenu* moreMenu = iconMenubar->addMenu(QString());
    const int settingsLabelIndex = moreMenu->addLabel(QString());
    QAction* preferencesAction = moreMenu->addItem(QString(), ShadIconType::Settings);
    QAction* helpAction = moreMenu->addItem(QString(), ShadIconType::LifeBuoy);
    ExampleI18n::bind(moreMenu, [moreMenu, settingsLabelIndex, preferencesAction, helpAction]()
    {
        moreMenu->setLabelText(settingsLabelIndex, tr("设置"));
        preferencesAction->setText(tr("偏好设置"));
        helpAction->setText(tr("帮助"));
    });
    ExampleI18n::bind(iconMenubar, [iconMenubar, projectMenu, moreMenu]()
    {
        iconMenubar->setMenuTitle(projectMenu, tr("项目"));
        iconMenubar->setMenuTitle(moreMenu, tr("更多"));
    });
    addRow(iconCard, {iconMenubar});
}

MenubarPage::~MenubarPage()
{
}
