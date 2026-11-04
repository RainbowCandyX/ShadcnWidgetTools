#include "ShadQuestionnaire.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QList>
#include <QObject>
#include <QSize>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadButton.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadQuestionnaireChoice.h"
#include "ShadQuestionnaireItemView.h"
#include "ShadQuestionnairePrivate.h"
#include "ShadStackedWidget.h"
#include "ShadStyleHelper.h"

namespace
{
    // 高度跟随当前题
    class QuestionnaireStack : public ShadStackedWidget
    {
        public:
            explicit QuestionnaireStack(QWidget* parent)
                : ShadStackedWidget(parent)
            {
                connect(this, &QStackedWidget::currentChanged, this, [this]() { updateGeometry(); });
            }

            QSize sizeHint() const override
            {
                const QWidget* page = currentWidget();
                return page != nullptr ? page->sizeHint() : QSize();
            }

            QSize minimumSizeHint() const override
            {
                const QWidget* page = currentWidget();
                return page != nullptr ? page->minimumSizeHint() : QSize();
            }

            bool hasHeightForWidth() const override
            {
                const QWidget* page = currentWidget();
                return page != nullptr && page->hasHeightForWidth();
            }

            int heightForWidth(int width) const override
            {
                const QWidget* page = currentWidget();
                return page != nullptr ? page->heightForWidth(width) : -1;
            }
    };
} // namespace

ShadQuestionnaire::ShadQuestionnaire(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadQuestionnairePrivate())
{
    Q_D(ShadQuestionnaire);
    d->q_ptr = this;
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    const int ring = ShadStyleHelper::kFocusRingWidth;

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(ShadQuestionnairePrivate::kSectionGap - 2 * ring);

    d->_progressLabel = new ShadLabel(this);
    d->_progressLabel->setColorRole(ShadThemeType::MutedForeground);
    d->_progressLabel->setContentsMargins(ring, 0, ring, 2 * ring);
    ShadStyleHelper::applyFont(d->_progressLabel, 12, QFont::Medium);
    layout->addWidget(d->_progressLabel);

    d->_stack = new QuestionnaireStack(this);
    layout->addWidget(d->_stack);

    d->_actions = new QWidget(this);
    QHBoxLayout* actionsLayout = new QHBoxLayout(d->_actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(ShadQuestionnairePrivate::kActionGap - 2 * ring);
    d->_previousButton = new ShadButton(QString(), ShadButtonType::Outline, d->_actions);
    d->_skipButton = new ShadButton(QString(), ShadButtonType::Outline, d->_actions);
    d->_nextButton = new ShadButton(QString(), ShadButtonType::Default, d->_actions);
    d->_submitButton = new ShadButton(QString(), ShadButtonType::Default, d->_actions);
    actionsLayout->addWidget(d->_previousButton);
    actionsLayout->addStretch(1);
    actionsLayout->addWidget(d->_skipButton);
    actionsLayout->addWidget(d->_nextButton);
    actionsLayout->addWidget(d->_submitButton);
    layout->addWidget(d->_actions);
    layout->addStretch(1);

    connect(d->_previousButton, &ShadButton::clicked, this, &ShadQuestionnaire::previous);
    connect(d->_skipButton, &ShadButton::clicked, this, &ShadQuestionnaire::skip);
    connect(d->_nextButton, &ShadButton::clicked, this, &ShadQuestionnaire::next);
    connect(d->_submitButton, &ShadButton::clicked, this, &ShadQuestionnaire::submit);
    // setProgressFormat 会调用 updateNavigation
    retranslate();
}

void ShadQuestionnaire::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadQuestionnaire::retranslate()
{
    Q_D(ShadQuestionnaire);
    ShadStyleHelper::applyDefaultText(this, "previousText", getPreviousText(), tr("上一题"), [this](const QString& text) { setPreviousText(text); });
    ShadStyleHelper::applyDefaultText(this, "skipText", getSkipText(), tr("跳过"), [this](const QString& text) { setSkipText(text); });
    ShadStyleHelper::applyDefaultText(this, "nextText", getNextText(), tr("下一题"), [this](const QString& text) { setNextText(text); });
    ShadStyleHelper::applyDefaultText(this, "submitText", getSubmitText(), tr("提交"), [this](const QString& text) { setSubmitText(text); });
    ShadStyleHelper::applyDefaultText(this, "progressFormat", d->_progressFormat, tr("第 %1 题，共 %2 题"), [this](const QString& text) { setProgressFormat(text); });
}

ShadQuestionnaire::~ShadQuestionnaire()
{
}

void ShadQuestionnaire::addItem(const QString& name, const QString& title, const QString& description)
{
    Q_D(ShadQuestionnaire);
    if (name.isEmpty() || d->findItem(name) != nullptr)
    {
        return;
    }
    ShadQuestionnairePrivate::Item item;
    item.name = name;
    d->_items.append(item);
    ShadQuestionnairePrivate::Item& stored = d->_items.last();
    d->createItemView(stored);
    stored.view->setTitle(title);
    stored.view->setDescription(description);
    d->ensureCurrent();
    d->updateNavigation();
}

QStringList ShadQuestionnaire::getItems() const
{
    Q_D(const ShadQuestionnaire);
    QStringList names;
    for (const ShadQuestionnairePrivate::Item& item : d->_items)
    {
        names.append(item.name);
    }
    return names;
}

void ShadQuestionnaire::setItemRequired(const QString& name, bool required)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr || item->isRequired == required)
    {
        return;
    }
    item->isRequired = required;
    if (required)
    {
        item->isSkipped = false;
    }
    d->refreshItem(*item);
    d->updateNavigation();
}

bool ShadQuestionnaire::isItemRequired(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr && item->isRequired;
}

void ShadQuestionnaire::setItemMultiple(const QString& name, bool multiple)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr || item->isMultiple == multiple)
    {
        return;
    }
    item->isMultiple = multiple;
    bool hasChecked = false;
    for (const ShadQuestionnairePrivate::Choice& choice : item->choices)
    {
        choice.widget->setRadio(!multiple);
        if (!multiple && choice.widget->isChecked())
        {
            choice.widget->setChecked(!hasChecked);
            hasChecked = true;
        }
    }
    if (!multiple && hasChecked)
    {
        item->isInputSelected = false;
    }
    d->refreshItem(*item);
}

bool ShadQuestionnaire::isItemMultiple(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr && item->isMultiple;
}

void ShadQuestionnaire::setItemDisabled(const QString& name, bool disabled)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr || item->isDisabled == disabled)
    {
        return;
    }
    item->isDisabled = disabled;
    d->refreshItem(*item);
    d->ensureCurrent();
    d->updateNavigation();
}

bool ShadQuestionnaire::isItemDisabled(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr && item->isDisabled;
}

void ShadQuestionnaire::setItemTitle(const QString& name, const QString& title)
{
    Q_D(ShadQuestionnaire);
    if (ShadQuestionnairePrivate::Item* item = d->findItem(name))
    {
        item->view->setTitle(title);
    }
}

void ShadQuestionnaire::setItemDescription(const QString& name, const QString& description)
{
    Q_D(ShadQuestionnaire);
    if (ShadQuestionnairePrivate::Item* item = d->findItem(name))
    {
        item->view->setDescription(description);
    }
}

void ShadQuestionnaire::setChoiceLabel(const QString& name, const QString& value, const QString& label)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    for (const ShadQuestionnairePrivate::Choice& choice : std::as_const(item->choices))
    {
        if (choice.value == value)
        {
            choice.widget->setLabel(label);
            return;
        }
    }
}

void ShadQuestionnaire::setChoiceDescription(const QString& name, const QString& value, const QString& description)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    for (const ShadQuestionnairePrivate::Choice& choice : std::as_const(item->choices))
    {
        if (choice.value == value)
        {
            choice.widget->setDescription(description);
            return;
        }
    }
}

void ShadQuestionnaire::addChoice(const QString& name, const QString& value, const QString& label, const QString& description)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    for (const ShadQuestionnairePrivate::Choice& choice : item->choices)
    {
        if (choice.value == value)
        {
            return;
        }
    }
    ShadQuestionnaireChoice* widget = item->view->addChoice(label, description);
    widget->setRadio(!item->isMultiple);
    item->choices.append({value, widget});
    connect(widget, &QAbstractButton::clicked, this, [d, name, widget]()
    {
        if (ShadQuestionnairePrivate::Item* target = d->findItem(name))
        {
            d->onChoiceClicked(*target, widget);
        }
    });
    d->updateShortcuts(*item);
}

void ShadQuestionnaire::setChoiceDisabled(const QString& name, const QString& value, bool disabled)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    for (const ShadQuestionnairePrivate::Choice& choice : item->choices)
    {
        if (choice.value != value)
        {
            continue;
        }
        choice.widget->setEnabled(!disabled);
        if (disabled)
        {
            choice.widget->setChecked(false);
        }
        d->updateShortcuts(*item);
        d->refreshItem(*item);
        return;
    }
}

void ShadQuestionnaire::setItemInput(const QString& name, const QString& placeholder)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    const bool isNew = item->view->getInput() == nullptr;
    ShadInput* input = item->view->ensureInput();
    input->setPlaceholderText(placeholder);
    input->setAccessibleName(placeholder);
    if (isNew)
    {
        connect(input, &QLineEdit::textChanged, this, [d, name](const QString& text)
        {
            if (ShadQuestionnairePrivate::Item* target = d->findItem(name))
            {
                d->onInputChanged(*target, text);
            }
        });
    }
}

void ShadQuestionnaire::setShortcuts(ShadQuestionnaireType::Shortcuts shortcuts)
{
    Q_D(ShadQuestionnaire);
    d->_shortcuts = shortcuts;
    for (ShadQuestionnairePrivate::Item& item : d->_items)
    {
        d->updateShortcuts(item);
    }
}

ShadQuestionnaireType::Shortcuts ShadQuestionnaire::getShortcuts() const
{
    Q_D(const ShadQuestionnaire);
    return d->_shortcuts;
}

void ShadQuestionnaire::setProgressVisible(bool visible)
{
    Q_D(ShadQuestionnaire);
    d->_isProgressVisible = visible;
    d->updateNavigation();
}

bool ShadQuestionnaire::isProgressVisible() const
{
    Q_D(const ShadQuestionnaire);
    return d->_isProgressVisible;
}

void ShadQuestionnaire::setProgressFormat(const QString& format)
{
    Q_D(ShadQuestionnaire);
    d->_progressFormat = format;
    d->updateNavigation();
}

QString ShadQuestionnaire::getProgressFormat() const
{
    Q_D(const ShadQuestionnaire);
    return d->_progressFormat;
}

void ShadQuestionnaire::setPreviousText(const QString& text)
{
    Q_D(ShadQuestionnaire);
    d->_previousButton->setText(text);
}

QString ShadQuestionnaire::getPreviousText() const
{
    Q_D(const ShadQuestionnaire);
    return d->_previousButton->text();
}

void ShadQuestionnaire::setSkipText(const QString& text)
{
    Q_D(ShadQuestionnaire);
    d->_skipButton->setText(text);
}

QString ShadQuestionnaire::getSkipText() const
{
    Q_D(const ShadQuestionnaire);
    return d->_skipButton->text();
}

void ShadQuestionnaire::setNextText(const QString& text)
{
    Q_D(ShadQuestionnaire);
    d->_nextButton->setText(text);
}

QString ShadQuestionnaire::getNextText() const
{
    Q_D(const ShadQuestionnaire);
    return d->_nextButton->text();
}

void ShadQuestionnaire::setSubmitText(const QString& text)
{
    Q_D(ShadQuestionnaire);
    d->_submitButton->setText(text);
}

QString ShadQuestionnaire::getSubmitText() const
{
    Q_D(const ShadQuestionnaire);
    return d->_submitButton->text();
}

void ShadQuestionnaire::setCurrentItem(const QString& name)
{
    Q_D(ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr || item->isDisabled)
    {
        return;
    }
    d->setCurrent(name, false);
}

QString ShadQuestionnaire::getCurrentItem() const
{
    Q_D(const ShadQuestionnaire);
    return d->_currentName;
}

int ShadQuestionnaire::getCurrentIndex() const
{
    Q_D(const ShadQuestionnaire);
    return d->currentIndex();
}

int ShadQuestionnaire::getItemCount() const
{
    Q_D(const ShadQuestionnaire);
    return static_cast<int>(d->logicalItems().size());
}

ShadQuestionnaireType::ItemStatus ShadQuestionnaire::getItemStatus(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr ? d->status(*item) : ShadQuestionnaireType::Unanswered;
}

QStringList ShadQuestionnaire::getAnswers(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr ? d->answers(*item) : QStringList();
}

void ShadQuestionnaire::setAnswers(const QString& name, const QStringList& answers)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    d->_isSyncing = true;
    d->clearSelection(*item);
    bool hasChoice = false;
    QStringList others;
    for (const QString& answer : answers)
    {
        ShadQuestionnaireChoice* matched = nullptr;
        for (const ShadQuestionnairePrivate::Choice& choice : item->choices)
        {
            if (choice.value == answer)
            {
                matched = choice.widget;
                break;
            }
        }
        if (matched == nullptr)
        {
            others.append(answer);
        }
        else if (matched->isEnabled() && (item->isMultiple || !hasChoice))
        {
            matched->setChecked(true);
            hasChoice = true;
        }
    }
    if (ShadInput* input = item->view->getInput())
    {
        const QString text = !others.isEmpty() && (item->isMultiple || !hasChoice) ? others.first() : QString();
        input->setText(text);
        item->isInputSelected = !text.trimmed().isEmpty();
    }
    item->isSkipped = false;
    d->_isSyncing = false;
    d->refreshItem(*item);
}

QHash<QString, QStringList> ShadQuestionnaire::getAllAnswers() const
{
    Q_D(const ShadQuestionnaire);
    QHash<QString, QStringList> result;
    for (const ShadQuestionnairePrivate::Item* item : d->logicalItems())
    {
        const QStringList values = d->answers(*item);
        if (!values.isEmpty())
        {
            result.insert(item->name, values);
        }
    }
    return result;
}

void ShadQuestionnaire::setItemError(const QString& name, const QString& message)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* item = d->findItem(name);
    if (item == nullptr)
    {
        return;
    }
    item->error = message;
    d->refreshItem(*item);
}

QString ShadQuestionnaire::getItemError(const QString& name) const
{
    Q_D(const ShadQuestionnaire);
    const ShadQuestionnairePrivate::Item* item = d->findItem(name);
    return item != nullptr ? item->error : QString();
}

void ShadQuestionnaire::previous()
{
    Q_D(ShadQuestionnaire);
    const QList<ShadQuestionnairePrivate::Item*> items = d->logicalItems();
    const int index = d->currentIndex();
    if (index > 0)
    {
        d->setCurrent(items.at(index - 1)->name, true);
    }
}

void ShadQuestionnaire::next()
{
    Q_D(ShadQuestionnaire);
    const QList<ShadQuestionnairePrivate::Item*> items = d->logicalItems();
    const int index = d->currentIndex();
    if (index < 0 || index >= items.size() - 1)
    {
        return;
    }
    ShadQuestionnairePrivate::Item* current = items.at(index);
    if (!d->validate(*current))
    {
        d->focusFirstAnswer(*current);
        return;
    }
    d->setCurrent(items.at(index + 1)->name, true);
}

void ShadQuestionnaire::skip()
{
    Q_D(ShadQuestionnaire);
    const QList<ShadQuestionnairePrivate::Item*> items = d->logicalItems();
    const int index = d->currentIndex();
    if (index < 0 || items.at(index)->isRequired)
    {
        return;
    }
    ShadQuestionnairePrivate::Item* current = items.at(index);
    d->_isSyncing = true;
    d->clearSelection(*current);
    d->_isSyncing = false;
    current->isSkipped = true;
    d->refreshItem(*current);
    if (index < items.size() - 1)
    {
        d->setCurrent(items.at(index + 1)->name, true);
        return;
    }
    submit();
}

void ShadQuestionnaire::submit()
{
    Q_D(ShadQuestionnaire);
    for (ShadQuestionnairePrivate::Item* item : d->logicalItems())
    {
        if (!d->validate(*item))
        {
            d->setCurrent(item->name, true);
            d->focusFirstAnswer(*item);
            return;
        }
    }
    Q_EMIT submitted();
}

void ShadQuestionnaire::reset()
{
    Q_D(ShadQuestionnaire);
    d->_isSyncing = true;
    for (ShadQuestionnairePrivate::Item& item : d->_items)
    {
        d->clearSelection(item);
        if (ShadInput* input = item.view->getInput())
        {
            input->clear();
        }
        item.isSkipped = false;
        item.isValidationAttempted = false;
        item.error.clear();
    }
    d->_isSyncing = false;
    for (ShadQuestionnairePrivate::Item& item : d->_items)
    {
        d->refreshItem(item);
    }
    const QList<ShadQuestionnairePrivate::Item*> items = d->logicalItems();
    if (!items.isEmpty())
    {
        d->setCurrent(items.first()->name, false);
    }
}

void ShadQuestionnaire::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadQuestionnaire);
    ShadQuestionnairePrivate::Item* current = d->currentItem();
    if (current == nullptr)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    QWidget* focus = QApplication::focusWidget();
    const bool isTextEntry = qobject_cast<QLineEdit*>(focus) != nullptr;
    const Qt::KeyboardModifiers modifiers = event->modifiers() & ~Qt::KeypadModifier;
    const int key = event->key();
    const bool isEnter = key == Qt::Key_Return || key == Qt::Key_Enter;

    // macOS 上 Qt::ControlModifier 对应 Command
    if (isEnter && modifiers == Qt::ControlModifier)
    {
        if (!event->isAutoRepeat())
        {
            d->confirm();
        }
        event->accept();
        return;
    }
    if ((modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) != 0)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    if ((key == Qt::Key_Up || key == Qt::Key_Down) && d->moveAnswerFocus(key == Qt::Key_Down))
    {
        event->accept();
        return;
    }
    if ((key == Qt::Key_Left || key == Qt::Key_Right) && !isTextEntry)
    {
        event->accept();
        if (event->isAutoRepeat())
        {
            return;
        }
        if (key == Qt::Key_Left)
        {
            previous();
        }
        else if (d->status(*current) != ShadQuestionnaireType::Unanswered)
        {
            next();
        }
        return;
    }
    if (isEnter)
    {
        const QList<QWidget*> controls = d->answerControls(*current);
        if (!controls.contains(focus))
        {
            QWidget::keyPressEvent(event);
            return;
        }
        event->accept();
        const QAbstractButton* choice = qobject_cast<QAbstractButton*>(focus);
        const QLineEdit* input = qobject_cast<QLineEdit*>(focus);
        const bool isFilled = (choice != nullptr && choice->isChecked()) || (input != nullptr && !input->text().trimmed().isEmpty());
        if (!event->isAutoRepeat() && isFilled)
        {
            d->confirm();
        }
        return;
    }
    if (!isTextEntry && !event->isAutoRepeat() && d->handleShortcut(event->text()))
    {
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
