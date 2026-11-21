#include "ShadTextContextMenu.h"

#include <QAction>
#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <functional>

#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"

namespace
{
    struct TextActions
    {
        bool isEditable{false};
        bool canUndo{false};
        bool canRedo{false};
        bool canCut{false};
        bool canCopy{false};
        bool canPaste{false};
        bool canDelete{false};
        bool canSelectAll{false};
        QString link;
        std::function<void()> undo;
        std::function<void()> redo;
        std::function<void()> cut;
        std::function<void()> copy;
        std::function<void()> paste;
        std::function<void()> remove;
        std::function<void()> selectAll;
    };

    void copyToClipboard(const QString& text)
    {
        QClipboard* clipboard = QGuiApplication::clipboard();
        if (clipboard != nullptr)
        {
            clipboard->setText(text);
        }
    }

    bool hasClipboardText()
    {
        const QClipboard* clipboard = QGuiApplication::clipboard();
        return clipboard != nullptr && !clipboard->text().isEmpty();
    }

    int labelTextLength(const QLabel* label)
    {
        const QString text = label->text();
        const bool isRichText = label->textFormat() == Qt::RichText || (label->textFormat() == Qt::AutoText && Qt::mightBeRichText(text));
        if (!isRichText)
        {
            return int(text.size());
        }
        QTextDocument document;
        document.setHtml(text);
        return qMax(0, document.characterCount() - 1);
    }

    void popup(QWidget* owner, const TextActions& actions, const QPoint& globalPos)
    {
        ShadDropdownMenu* menu = new ShadDropdownMenu(owner);
        QObject::connect(menu, &ShadDropdownMenu::aboutToHide, menu, &QObject::deleteLater);
        if (actions.isEditable)
        {
            QAction* undoAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "撤销"), ShadIconType::Undo2, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Undo)));
            undoAction->setEnabled(actions.canUndo);
            QObject::connect(undoAction, &QAction::triggered, owner, actions.undo);
            QAction* redoAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "恢复"), ShadIconType::Redo2, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Redo)));
            redoAction->setEnabled(actions.canRedo);
            QObject::connect(redoAction, &QAction::triggered, owner, actions.redo);
            menu->addSeparator();
            QAction* cutAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "剪切"), ShadIconType::Scissors, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Cut)));
            cutAction->setEnabled(actions.canCut);
            QObject::connect(cutAction, &QAction::triggered, owner, actions.cut);
        }
        QAction* copyAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "复制"), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Copy)));
        copyAction->setEnabled(actions.canCopy);
        QObject::connect(copyAction, &QAction::triggered, owner, actions.copy);
        if (!actions.link.isEmpty())
        {
            const QString link = actions.link;
            QAction* linkAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "复制链接地址"), ShadIconType::Link);
            QObject::connect(linkAction, &QAction::triggered, owner, [link]() { copyToClipboard(link); });
        }
        if (actions.isEditable)
        {
            QAction* pasteAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "粘贴"), ShadIconType::ClipboardPaste, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Paste)));
            pasteAction->setEnabled(actions.canPaste);
            QObject::connect(pasteAction, &QAction::triggered, owner, actions.paste);
            QAction* deleteAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "删除"), ShadIconType::Delete);
            deleteAction->setEnabled(actions.canDelete);
            QObject::connect(deleteAction, &QAction::triggered, owner, actions.remove);
        }
        menu->addSeparator();
        QAction* selectAllAction = menu->addItem(QCoreApplication::translate("ShadTextContextMenu", "全选"), ShadIconType::TextSelect, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::SelectAll)));
        selectAllAction->setEnabled(actions.canSelectAll);
        QObject::connect(selectAllAction, &QAction::triggered, owner, actions.selectAll);
        menu->popupAt(globalPos);
    }
} // namespace

namespace ShadTextContextMenu
{
    void show(QLineEdit* edit, const QPoint& globalPos)
    {
        if (edit == nullptr)
        {
            return;
        }
        const bool hasSelection = edit->hasSelectedText();
        const bool isPlainEcho = edit->echoMode() == QLineEdit::Normal;
        TextActions actions;
        actions.isEditable = !edit->isReadOnly();
        actions.canUndo = edit->isUndoAvailable();
        actions.canRedo = edit->isRedoAvailable();
        actions.canCut = hasSelection && isPlainEcho;
        actions.canCopy = hasSelection && isPlainEcho;
        actions.canPaste = hasClipboardText();
        actions.canDelete = hasSelection;
        actions.canSelectAll = !edit->text().isEmpty() && edit->selectedText() != edit->text();
        actions.undo = [edit]() { edit->undo(); };
        actions.redo = [edit]() { edit->redo(); };
        actions.cut = [edit]() { edit->cut(); };
        actions.copy = [edit]() { edit->copy(); };
        actions.paste = [edit]() { edit->paste(); };
        actions.remove = [edit]() { edit->del(); };
        actions.selectAll = [edit]() { edit->selectAll(); };
        popup(edit, actions, globalPos);
    }

    void show(QPlainTextEdit* edit, const QPoint& globalPos)
    {
        if (edit == nullptr)
        {
            return;
        }
        const bool hasSelection = edit->textCursor().hasSelection();
        TextActions actions;
        actions.isEditable = !edit->isReadOnly();
        actions.canUndo = edit->document()->isUndoAvailable();
        actions.canRedo = edit->document()->isRedoAvailable();
        actions.canCut = hasSelection;
        actions.canCopy = hasSelection;
        actions.canPaste = edit->canPaste();
        actions.canDelete = hasSelection;
        actions.canSelectAll = !edit->document()->isEmpty();
        actions.undo = [edit]() { edit->undo(); };
        actions.redo = [edit]() { edit->redo(); };
        actions.cut = [edit]() { edit->cut(); };
        actions.copy = [edit]() { edit->copy(); };
        actions.paste = [edit]() { edit->paste(); };
        actions.remove = [edit]() { edit->textCursor().removeSelectedText(); };
        actions.selectAll = [edit]() { edit->selectAll(); };
        popup(edit, actions, globalPos);
    }

    void show(QTextEdit* edit, const QPoint& globalPos, const QString& link)
    {
        if (edit == nullptr)
        {
            return;
        }
        const bool hasSelection = edit->textCursor().hasSelection();
        TextActions actions;
        actions.isEditable = !edit->isReadOnly();
        actions.canUndo = edit->document()->isUndoAvailable();
        actions.canRedo = edit->document()->isRedoAvailable();
        actions.canCut = hasSelection;
        actions.canCopy = hasSelection;
        actions.canPaste = edit->canPaste();
        actions.canDelete = hasSelection;
        actions.canSelectAll = !edit->document()->isEmpty();
        actions.link = link;
        actions.undo = [edit]() { edit->undo(); };
        actions.redo = [edit]() { edit->redo(); };
        actions.cut = [edit]() { edit->cut(); };
        actions.copy = [edit]() { edit->copy(); };
        actions.paste = [edit]() { edit->paste(); };
        actions.remove = [edit]() { edit->textCursor().removeSelectedText(); };
        actions.selectAll = [edit]() { edit->selectAll(); };
        popup(edit, actions, globalPos);
    }

    void show(QLabel* label, const QPoint& globalPos)
    {
        if (label == nullptr || !label->textInteractionFlags().testFlag(Qt::TextSelectableByMouse))
        {
            return;
        }
        const int length = labelTextLength(label);
        TextActions actions;
        actions.canCopy = label->hasSelectedText();
        actions.canSelectAll = length > 0;
        actions.copy = [label]() { copyToClipboard(label->selectedText()); };
        actions.selectAll = [label, length]() { label->setSelection(0, length); };
        popup(label, actions, globalPos);
    }

    void install(QLineEdit* edit)
    {
        edit->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(edit, &QWidget::customContextMenuRequested, edit, [edit](const QPoint& pos) { show(edit, edit->mapToGlobal(pos)); });
    }

    void install(QLabel* label)
    {
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(label, &QWidget::customContextMenuRequested, label, [label](const QPoint& pos) { show(label, label->mapToGlobal(pos)); });
    }
} // namespace ShadTextContextMenu
