#include "ContextMenuPage.h"

#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QFont>
#include <QKeySequence>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <functional>
#include <memory>
#include <optional>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadContextMenu.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadLabel.h"
#include "ShadTheme.h"

namespace
{
    // 虚线边框的右键区域
    class ContextArea : public QWidget
    {
        public:
            explicit ContextArea(QWidget* parent)
                : QWidget(parent)
            {
                setFixedSize(300, 150);
                QFont areaFont = font();
                areaFont.setPixelSize(14);
                setFont(areaFont);
                QObject::connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

            void setText(const QString& text)
            {
                _text = text;
                update();
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                QPen pen(shadTheme->getColor(ShadThemeType::Border));
                pen.setStyle(Qt::DashLine);
                painter.setPen(pen);
                const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
                painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
                painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
                painter.drawText(rect(), Qt::AlignCenter, _text);
            }

        private:
            QString _text;
    };

    QString shortcut(const QKeySequence& sequence)
    {
        return ShadKbdGroup::shortcutText(sequence);
    }

    // 菜单中任一菜单项被点击时调用 onTriggered
    void trackTriggered(ShadDropdownMenu * menu, QObject * context, const std::function < void(QAction *) > &onTriggered)
    {
        for (QAction* action : menu->findChildren<QAction*>(QString(), Qt::FindDirectChildrenOnly))
        {
            QObject::connect(action, &QAction::triggered, context, [action, onTriggered]() { onTriggered(action); });
        }
    }

    ShadLabel* createStatusLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

ContextMenuPage::ContextMenuPage(QWidget* parent)
    : ExampleBasePage([] { return tr("右键菜单"); }, [] { return tr("ShadContextMenu：右键菜单。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("子菜单、复选与单选"); });
    ContextArea* area = new ContextArea(this);
    ShadLabel* basicStatus = createStatusLabel(this);
    ShadContextMenu* menu = new ShadContextMenu(area);
    menu->attachTo(area);
    QAction* backAction = menu->addItem(QString(), ShadIconType::None, shortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft)));
    QAction* forwardAction = menu->addItem(QString(), ShadIconType::None, shortcut(QKeySequence(Qt::CTRL | Qt::Key_BracketRight)));
    forwardAction->setEnabled(false);
    QAction* reloadAction = menu->addItem(QString(), ShadIconType::None, shortcut(QKeySequence(Qt::CTRL | Qt::Key_R)));
    ShadDropdownMenu* toolsMenu = menu->addMenu(QString());
    QAction* savePageAction = toolsMenu->addItem(QString());
    QAction* createShortcutAction = toolsMenu->addItem(QString());
    QAction* nameWindowAction = toolsMenu->addItem(QString());
    toolsMenu->addSeparator();
    QAction* developerToolsAction = toolsMenu->addItem(QString());
    toolsMenu->addSeparator();
    QAction* deleteAction = toolsMenu->addItem(QString());
    toolsMenu->setItemDestructive(deleteAction, true);
    menu->addSeparator();
    QAction* bookmarksAction = menu->addCheckableItem(QString(), true);
    QAction* fullUrlsAction = menu->addCheckableItem(QString());
    menu->addSeparator();
    const int memberLabel = menu->addLabel(QString(), true);
    ExampleI18n::bind(menu, [menu, memberLabel]() { menu->setLabelText(memberLabel, tr("成员")); });
    QActionGroup* personGroup = new QActionGroup(menu);
    QAction* firstPersonAction = menu->addRadioItem(QString(), personGroup, true);
    QAction* secondPersonAction = menu->addRadioItem(QString(), personGroup);
    ExampleI18n::bind(menu, [area, backAction, forwardAction, reloadAction, toolsMenu, savePageAction, createShortcutAction, nameWindowAction, developerToolsAction, deleteAction, bookmarksAction, fullUrlsAction, firstPersonAction, secondPersonAction]()
    {
        area->setText(tr("在此处右键"));
        backAction->setText(tr("后退"));
        forwardAction->setText(tr("前进"));
        reloadAction->setText(tr("重新加载"));
        toolsMenu->menuAction()->setText(tr("更多工具"));
        savePageAction->setText(tr("保存页面…"));
        createShortcutAction->setText(tr("创建快捷方式…"));
        nameWindowAction->setText(tr("命名窗口…"));
        developerToolsAction->setText(tr("开发者工具"));
        deleteAction->setText(tr("删除"));
        bookmarksAction->setText(tr("显示书签"));
        fullUrlsAction->setText(tr("显示完整网址"));
        firstPersonAction->setText(tr("张三"));
        secondPersonAction->setText(tr("李四"));
    });
    // 最近点击的菜单项，菜单项文字先于此处刷新（连接顺序在前）
    const std::shared_ptr<QPointer<QAction>> lastAction = std::make_shared<QPointer<QAction>>();
    const auto updateBasicStatus = [basicStatus, lastAction]()
    {
        const QAction* action = lastAction->data();
        basicStatus->setText(action != nullptr ? tr("最近点击: %1").arg(action->text()) : tr("最近点击: 无"));
    };
    const auto onTriggered = [lastAction, updateBasicStatus](QAction* action)
    {
        *lastAction = action;
        updateBasicStatus();
    };
    trackTriggered(menu, basicStatus, onTriggered);
    trackTriggered(toolsMenu, basicStatus, onTriggered);
    ExampleI18n::bind(basicStatus, updateBasicStatus);
    addRow(basicCard, {area, basicStatus});

    ShadCard* positionCard = addSection([] { return tr("按位置更新"); }, [] { return tr("读取右键位置"); });
    ContextArea* positionArea = new ContextArea(this);
    ShadLabel* positionStatus = createStatusLabel(this);
    ShadContextMenu* positionMenu = new ShadContextMenu(positionArea);
    positionMenu->attachTo(positionArea);
    QAction* positionAction = positionMenu->addItem(QString(), ShadIconType::MousePointerClick);
    positionMenu->addSeparator();
    QAction* copyAction = positionMenu->addItem(QString(), ShadIconType::Copy);
    QAction* pasteAction = positionMenu->addItem(QString(), ShadIconType::ClipboardPaste);
    // 最近一次右键的位置，没有右键过时为空
    const std::shared_ptr<std::optional<QPoint>> lastPos = std::make_shared<std::optional<QPoint>>();
    const auto updatePositionTexts = [positionAction, positionStatus, lastPos]()
    {
        if (!lastPos->has_value())
        {
            positionStatus->setText(tr("右键位置: 无"));
            return;
        }
        const QString text = tr("右键位置: %1, %2").arg(lastPos->value().x()).arg(lastPos->value().y());
        positionAction->setText(text);
        positionStatus->setText(text);
    };
    ExampleI18n::bind(positionMenu, [positionArea, copyAction, pasteAction, updatePositionTexts]()
    {
        positionArea->setText(tr("在此处右键"));
        copyAction->setText(tr("复制"));
        pasteAction->setText(tr("粘贴"));
        updatePositionTexts();
    });
    connect(positionMenu, &ShadContextMenu::aboutToPopup, this, [lastPos, updatePositionTexts](QWidget* target, const QPoint& pos)
    {
        Q_UNUSED(target);
        *lastPos = pos;
        updatePositionTexts();
    });
    addRow(positionCard, {positionArea, positionStatus});
}

ContextMenuPage::~ContextMenuPage()
{
}
