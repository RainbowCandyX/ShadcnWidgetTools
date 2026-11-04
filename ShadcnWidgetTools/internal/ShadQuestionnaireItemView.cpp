#include "ShadQuestionnaireItemView.h"

#include <QFont>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadQuestionnaireChoice.h"
#include "ShadStyleHelper.h"

namespace
{
    constexpr int kItemGap = 20;
    constexpr int kChoiceGap = 12;
} // namespace

ShadQuestionnaireItemView::ShadQuestionnaireItemView(QWidget* parent)
    : QWidget(parent)
{
    const int ring = ShadStyleHelper::kFocusRingWidth;
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kItemGap - ring);

    QWidget* header = new QWidget(this);
    QVBoxLayout* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(ring, 0, ring, 0);
    headerLayout->setSpacing(0);
    _titleLabel = new ShadLabel(header);
    _titleLabel->setColorRole(ShadThemeType::Foreground);
    _titleLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(_titleLabel, 16, QFont::DemiBold);
    _descriptionLabel = new ShadLabel(header);
    _descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    _descriptionLabel->setWordWrap(true);
    _descriptionLabel->setVisible(false);
    ShadStyleHelper::applyFont(_descriptionLabel, 14, QFont::Normal);
    headerLayout->addWidget(_titleLabel);
    headerLayout->addWidget(_descriptionLabel);
    layout->addWidget(header);

    QWidget* choices = new QWidget(this);
    _choicesLayout = new QVBoxLayout(choices);
    _choicesLayout->setContentsMargins(0, 0, 0, 0);
    _choicesLayout->setSpacing(kChoiceGap - 2 * ring);
    layout->addWidget(choices);

    _errorLabel = new ShadLabel(this);
    _errorLabel->setColorRole(ShadThemeType::Destructive);
    _errorLabel->setWordWrap(true);
    _errorLabel->setContentsMargins(ring, 0, ring, 0);
    _errorLabel->setVisible(false);
    ShadStyleHelper::applyFont(_errorLabel, 14, QFont::Normal);
    layout->addWidget(_errorLabel);
    layout->addStretch(1);
}

ShadQuestionnaireItemView::~ShadQuestionnaireItemView()
{
}

void ShadQuestionnaireItemView::setTitle(const QString& title)
{
    _titleLabel->setText(title);
}

void ShadQuestionnaireItemView::setDescription(const QString& description)
{
    _descriptionLabel->setText(description);
    _descriptionLabel->setVisible(!description.isEmpty());
}

ShadQuestionnaireChoice* ShadQuestionnaireItemView::addChoice(const QString& label, const QString& description)
{
    ShadQuestionnaireChoice* choice = new ShadQuestionnaireChoice(this);
    choice->setLabel(label);
    choice->setDescription(description);
    const int index = _input != nullptr ? _choicesLayout->indexOf(_input) : _choicesLayout->count();
    _choicesLayout->insertWidget(index, choice);
    return choice;
}

ShadInput* ShadQuestionnaireItemView::ensureInput()
{
    if (_input == nullptr)
    {
        _input = new ShadInput(this);
        _choicesLayout->addWidget(_input);
    }
    return _input;
}

ShadInput* ShadQuestionnaireItemView::getInput() const
{
    return _input;
}

void ShadQuestionnaireItemView::setError(const QString& error)
{
    _errorLabel->setText(error);
    _errorLabel->setVisible(!error.isEmpty());
}
