#include "ShadQuestionnairePrivate.h"

#include <QApplication>
#include <QChar>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadQuestionnaire.h"
#include "ShadQuestionnaireChoice.h"
#include "ShadQuestionnaireItemView.h"
#include "ShadStackedWidget.h"

namespace
{
    constexpr int kLetterCount = 26;
    constexpr int kNumberCount = 9;
} // namespace

ShadQuestionnairePrivate::ShadQuestionnairePrivate()
{
}

ShadQuestionnairePrivate::~ShadQuestionnairePrivate()
{
}

ShadQuestionnairePrivate::Item* ShadQuestionnairePrivate::findItem(const QString& name)
{
    for (Item& item : _items)
    {
        if (item.name == name)
        {
            return &item;
        }
    }
    return nullptr;
}

const ShadQuestionnairePrivate::Item* ShadQuestionnairePrivate::findItem(const QString& name) const
{
    for (const Item& item : _items)
    {
        if (item.name == name)
        {
            return &item;
        }
    }
    return nullptr;
}

QList<ShadQuestionnairePrivate::Item*> ShadQuestionnairePrivate::logicalItems()
{
    QList<Item*> items;
    for (Item& item : _items)
    {
        if (!item.isDisabled)
        {
            items.append(&item);
        }
    }
    return items;
}

QList<const ShadQuestionnairePrivate::Item*> ShadQuestionnairePrivate::logicalItems() const
{
    QList<const Item*> items;
    for (const Item& item : _items)
    {
        if (!item.isDisabled)
        {
            items.append(&item);
        }
    }
    return items;
}

ShadQuestionnairePrivate::Item* ShadQuestionnairePrivate::currentItem()
{
    Item* item = findItem(_currentName);
    return item != nullptr && !item->isDisabled ? item : nullptr;
}

int ShadQuestionnairePrivate::currentIndex() const
{
    const QList<const Item*> items = logicalItems();
    for (int index = 0; index < items.size(); ++index)
    {
        if (items.at(index)->name == _currentName)
        {
            return index;
        }
    }
    return -1;
}

ShadQuestionnaireType::ItemStatus ShadQuestionnairePrivate::status(const Item& item) const
{
    if (item.isSkipped)
    {
        return ShadQuestionnaireType::Skipped;
    }
    return answers(item).isEmpty() ? ShadQuestionnaireType::Unanswered : ShadQuestionnaireType::Answered;
}

bool ShadQuestionnairePrivate::isValid(const Item& item) const
{
    if (item.isDisabled || (item.isSkipped && !item.isRequired))
    {
        return true;
    }
    return item.error.isEmpty() && status(item) == ShadQuestionnaireType::Answered;
}

QStringList ShadQuestionnairePrivate::answers(const Item& item) const
{
    QStringList values;
    for (const Choice& choice : item.choices)
    {
        if (choice.widget->isEnabled() && choice.widget->isChecked())
        {
            values.append(choice.value);
        }
    }
    const ShadInput* input = item.view->getInput();
    if (input != nullptr && input->isEnabled() && item.isInputSelected)
    {
        values.append(input->text());
    }
    return values;
}

QList<QWidget*> ShadQuestionnairePrivate::answerControls(const Item& item) const
{
    QList<QWidget*> controls;
    for (const Choice& choice : item.choices)
    {
        if (choice.widget->isEnabled())
        {
            controls.append(choice.widget);
        }
    }
    ShadInput* input = item.view->getInput();
    if (input != nullptr && input->isEnabled())
    {
        controls.append(input);
    }
    return controls;
}

void ShadQuestionnairePrivate::createItemView(Item& item)
{
    item.view = new ShadQuestionnaireItemView(_stack);
    _stack->addWidget(item.view);
}

void ShadQuestionnairePrivate::onChoiceClicked(Item& item, ShadQuestionnaireChoice* widget)
{
    if (_isSyncing)
    {
        return;
    }
    if (!item.isMultiple && widget->isChecked())
    {
        for (const Choice& choice : item.choices)
        {
            if (choice.widget != widget)
            {
                choice.widget->setChecked(false);
            }
        }
        item.isInputSelected = false;
    }
    item.isSkipped = false;
    refreshItem(item);
}

void ShadQuestionnairePrivate::onInputChanged(Item& item, const QString& text)
{
    if (_isSyncing)
    {
        return;
    }
    const bool isFilled = !text.trimmed().isEmpty();
    if (isFilled && !item.isMultiple)
    {
        for (const Choice& choice : item.choices)
        {
            choice.widget->setChecked(false);
        }
    }
    if (isFilled)
    {
        item.isSkipped = false;
    }
    item.isInputSelected = isFilled;
    refreshItem(item);
}

void ShadQuestionnairePrivate::clearSelection(Item& item)
{
    for (const Choice& choice : item.choices)
    {
        choice.widget->setChecked(false);
    }
    item.isInputSelected = false;
}

void ShadQuestionnairePrivate::refreshItem(Item& item)
{
    Q_Q(ShadQuestionnaire);
    const bool isSkippedOptional = item.isSkipped && !item.isRequired;
    const bool isInvalid = !item.isDisabled && !isSkippedOptional && (!item.error.isEmpty() || (item.isValidationAttempted && !isValid(item)));
    QString message;
    if (isInvalid)
    {
        if (!item.error.isEmpty())
        {
            message = item.error;
        }
        else
        {
            message = item.isRequired ? ShadQuestionnaire::tr("请选择一个答案。") : ShadQuestionnaire::tr("请选择一个答案或跳过此题。");
        }
    }
    item.view->setError(message);
    for (const Choice& choice : item.choices)
    {
        choice.widget->setInvalid(isInvalid);
    }
    if (ShadInput* input = item.view->getInput())
    {
        input->setInvalid(isInvalid);
    }
    const ShadQuestionnaireType::ItemStatus current = status(item);
    if (current != item.lastStatus)
    {
        item.lastStatus = current;
        Q_EMIT q->itemStatusChanged(item.name, current);
    }
    const QStringList values = answers(item);
    if (values != item.lastAnswers)
    {
        item.lastAnswers = values;
        Q_EMIT q->answersChanged(item.name);
    }
}

bool ShadQuestionnairePrivate::validate(Item& item)
{
    item.isValidationAttempted = true;
    refreshItem(item);
    return isValid(item);
}

void ShadQuestionnairePrivate::focusFirstAnswer(const Item& item)
{
    const QList<QWidget*> controls = answerControls(item);
    if (!controls.isEmpty())
    {
        controls.first()->setFocus(Qt::OtherFocusReason);
    }
}

void ShadQuestionnairePrivate::updateShortcuts(Item& item)
{
    const int limit = _shortcuts == ShadQuestionnaireType::Letters ? kLetterCount : (_shortcuts == ShadQuestionnaireType::Numbers ? kNumberCount : 0);
    int index = 0;
    for (const Choice& choice : item.choices)
    {
        QString text;
        if (choice.widget->isEnabled() && index < limit)
        {
            text = _shortcuts == ShadQuestionnaireType::Letters ? QString(QChar(u'A' + index)) : QString::number(index + 1);
            ++index;
        }
        choice.widget->setShortcutText(text);
    }
}

void ShadQuestionnairePrivate::setCurrent(const QString& name, bool moveFocus)
{
    Q_Q(ShadQuestionnaire);
    if (_currentName == name)
    {
        updateNavigation();
        return;
    }
    Item* item = findItem(name);
    if (item == nullptr)
    {
        return;
    }
    // 焦点在问卷内时才移动
    QWidget* focus = QApplication::focusWidget();
    const bool hadFocus = moveFocus && focus != nullptr && (focus == q || q->isAncestorOf(focus));
    _currentName = name;
    _stack->setCurrentWidget(item->view);
    updateNavigation();
    if (hadFocus)
    {
        focusFirstAnswer(*item);
    }
    Q_EMIT q->currentItemChanged(name);
}

void ShadQuestionnairePrivate::ensureCurrent()
{
    Q_Q(ShadQuestionnaire);
    if (currentItem() != nullptr)
    {
        return;
    }
    int start = 0;
    for (int index = 0; index < _items.size(); ++index)
    {
        if (_items.at(index).name == _currentName)
        {
            start = index;
            break;
        }
    }
    for (int index = start; index < _items.size(); ++index)
    {
        if (!_items.at(index).isDisabled)
        {
            setCurrent(_items.at(index).name, true);
            return;
        }
    }
    for (int index = start - 1; index >= 0; --index)
    {
        if (!_items.at(index).isDisabled)
        {
            setCurrent(_items.at(index).name, true);
            return;
        }
    }
    if (!_currentName.isEmpty())
    {
        _currentName.clear();
        updateNavigation();
        Q_EMIT q->currentItemChanged(QString());
    }
}

void ShadQuestionnairePrivate::updateNavigation()
{
    const int total = static_cast<int>(logicalItems().size());
    const int index = currentIndex();
    const bool isLast = index >= 0 && index == total - 1;
    const Item* current = currentItem();
    _previousButton->setVisible(total > 1 && index > 0);
    _skipButton->setVisible(current != nullptr && !current->isRequired);
    _nextButton->setVisible(total > 1 && index >= 0 && !isLast);
    _submitButton->setVisible(total > 0 && isLast);
    _progressLabel->setVisible(_isProgressVisible && total > 0);
    _progressLabel->setText(_progressFormat.arg(index + 1).arg(total));
}

void ShadQuestionnairePrivate::confirm()
{
    Q_Q(ShadQuestionnaire);
    Item* current = currentItem();
    if (current == nullptr)
    {
        return;
    }
    if (!validate(*current))
    {
        focusFirstAnswer(*current);
        return;
    }
    const int index = currentIndex();
    if (index == logicalItems().size() - 1)
    {
        q->submit();
        return;
    }
    q->next();
}

bool ShadQuestionnairePrivate::handleShortcut(const QString& key)
{
    Item* current = currentItem();
    if (current == nullptr || _shortcuts == ShadQuestionnaireType::NoShortcuts || key.isEmpty())
    {
        return false;
    }
    const QString normalized = key.toUpper();
    for (const Choice& choice : current->choices)
    {
        if (choice.widget->isEnabled() && choice.widget->getShortcutText() == normalized)
        {
            choice.widget->setFocus(Qt::ShortcutFocusReason);
            choice.widget->click();
            return true;
        }
    }
    return false;
}

bool ShadQuestionnairePrivate::moveAnswerFocus(bool forward)
{
    Item* current = currentItem();
    if (current == nullptr)
    {
        return false;
    }
    const QList<QWidget*> controls = answerControls(*current);
    const int index = static_cast<int>(controls.indexOf(QApplication::focusWidget()));
    if (index < 0)
    {
        return false;
    }
    const int target = index + (forward ? 1 : -1);
    if (target < 0 || target >= controls.size())
    {
        return false;
    }
    controls.at(target)->setFocus(Qt::TabFocusReason);
    return true;
}
