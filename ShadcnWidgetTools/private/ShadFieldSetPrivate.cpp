#include "ShadFieldSetPrivate.h"

#include <QBoxLayout>
#include <QFont>
#include <QSizePolicy>
#include <QSpacerItem>

#include "ShadFieldSet.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"

ShadFieldSetPrivate::ShadFieldSetPrivate()
{
}

ShadFieldSetPrivate::~ShadFieldSetPrivate()
{
}

void ShadFieldSetPrivate::applyLegendFont()
{
    ShadStyleHelper::applyFont(_legendLabel, _legendVariant == ShadFieldType::Label ? 14 : 16, QFont::Medium);
}

void ShadFieldSetPrivate::updateHeader()
{
    const bool hasLegend = !_legendLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    _legendLabel->setVisible(hasLegend);
    _descriptionLabel->setVisible(hasDescription);
    int legendGap = 0;
    if (hasLegend)
    {
        legendGap = hasDescription ? kDescriptionGap : kLegendGap;
    }
    const int descriptionGap = hasDescription ? kContentGap : 0;
    _legendSpacer->changeSize(0, legendGap, QSizePolicy::Minimum, QSizePolicy::Fixed);
    _descriptionSpacer->changeSize(0, descriptionGap, QSizePolicy::Minimum, QSizePolicy::Fixed);
    _layout->invalidate();
}
