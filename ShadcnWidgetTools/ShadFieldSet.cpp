#include "ShadFieldSet.h"

#include <QBoxLayout>
#include <QFont>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadFieldSetPrivate.h"
#include "ShadLabel.h"
#include "ShadMaxWidthBoxLayout.h"
#include "ShadStyleHelper.h"

ShadFieldSet::ShadFieldSet(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadFieldSetPrivate())
{
    Q_D(ShadFieldSet);
    d->q_ptr = this;
    // 设置最大宽度后，自动换行的说明文字按实际宽度计算高度
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);

    d->_legendLabel = new ShadLabel(this);
    d->_legendLabel->setWordWrap(true);
    d->applyLegendFont();
    d->_descriptionLabel = new ShadLabel(this);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    d->_descriptionLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    d->_content = new QWidget(this);
    d->_contentLayout = new QVBoxLayout(d->_content);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(ShadFieldSetPrivate::kContentGap);

    d->_legendSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    d->_descriptionSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    d->_layout->addWidget(d->_legendLabel);
    d->_layout->addItem(d->_legendSpacer);
    d->_layout->addWidget(d->_descriptionLabel);
    d->_layout->addItem(d->_descriptionSpacer);
    d->_layout->addWidget(d->_content);
    d->updateHeader();
}

ShadFieldSet::ShadFieldSet(const QString& legend, const QString& description, QWidget* parent)
    : ShadFieldSet(parent)
{
    setLegend(legend);
    setDescription(description);
}

ShadFieldSet::~ShadFieldSet()
{
}

void ShadFieldSet::setLegend(const QString& legend)
{
    Q_D(ShadFieldSet);
    d->_legendLabel->setText(legend);
    d->updateHeader();
}

QString ShadFieldSet::getLegend() const
{
    Q_D(const ShadFieldSet);
    return d->_legendLabel->text();
}

void ShadFieldSet::setLegendVariant(ShadFieldType::LegendVariant variant)
{
    Q_D(ShadFieldSet);
    d->_legendVariant = variant;
    d->applyLegendFont();
}

ShadFieldType::LegendVariant ShadFieldSet::getLegendVariant() const
{
    Q_D(const ShadFieldSet);
    return d->_legendVariant;
}

void ShadFieldSet::setDescription(const QString& description)
{
    Q_D(ShadFieldSet);
    d->_descriptionLabel->setText(description);
    d->updateHeader();
}

QString ShadFieldSet::getDescription() const
{
    Q_D(const ShadFieldSet);
    return d->_descriptionLabel->text();
}

void ShadFieldSet::addWidget(QWidget* widget)
{
    Q_D(ShadFieldSet);
    if (widget != nullptr)
    {
        d->_contentLayout->addWidget(widget);
    }
}

QVBoxLayout* ShadFieldSet::contentLayout() const
{
    Q_D(const ShadFieldSet);
    return d->_contentLayout;
}
