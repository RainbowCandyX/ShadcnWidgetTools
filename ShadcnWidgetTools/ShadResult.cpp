#include "ShadResult.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadButton.h"
#include "ShadLabel.h"
#include "ShadResultIcon.h"
#include "ShadResultPrivate.h"
#include "ShadStyleHelper.h"

ShadResult::ShadResult(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadResultPrivate())
{
    Q_D(ShadResult);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(ShadResultPrivate::kPaddingX, ShadResultPrivate::kPaddingY, ShadResultPrivate::kPaddingX, ShadResultPrivate::kPaddingY);
    mainLayout->setSpacing(0);
    QWidget* column = new QWidget(this);
    column->setMaximumWidth(ShadResultPrivate::kMaxContentWidth);
    mainLayout->addStretch(1);
    mainLayout->addWidget(column, ShadResultPrivate::kColumnStretch);
    mainLayout->addStretch(1);

    QVBoxLayout* columnLayout = new QVBoxLayout(column);
    d->_columnLayout = columnLayout;
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(0);
    d->_icon = new ShadResultIcon(column);
    columnLayout->addWidget(d->_icon, 0, Qt::AlignHCenter);

    d->_titleLabel = new ShadLabel(column);
    d->_titleLabel->setTextFormat(Qt::PlainText);
    d->_titleLabel->setWordWrap(true);
    d->_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    d->_titleLabel->setContentsMargins(0, ShadResultPrivate::kTitleGap, 0, 0);
    ShadStyleHelper::applyFont(d->_titleLabel, ShadResultPrivate::kTitleFontSize, QFont::DemiBold);
    d->_titleLabel->hide();
    columnLayout->addWidget(d->_titleLabel);

    d->_descriptionLabel = new ShadLabel(column);
    d->_descriptionLabel->setTextFormat(Qt::PlainText);
    d->_descriptionLabel->setWordWrap(true);
    d->_descriptionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    d->_descriptionLabel->setContentsMargins(0, ShadResultPrivate::kDescriptionGap, 0, 0);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(d->_descriptionLabel, ShadResultPrivate::kTextFontSize, QFont::Normal);
    d->_descriptionLabel->hide();
    columnLayout->addWidget(d->_descriptionLabel);

    d->_contentWidget = new QWidget(column);
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, ShadResultPrivate::kSectionGap, 0, 0);
    d->_contentLayout->setSpacing(ShadResultPrivate::kContentSpacing);
    d->_contentWidget->hide();
    columnLayout->addWidget(d->_contentWidget);

    d->_actionsWidget = new QWidget(column);
    d->_actionsLayout = new QHBoxLayout(d->_actionsWidget);
    d->_actionsLayout->setContentsMargins(0, ShadResultPrivate::kSectionGap, 0, 0);
    d->_actionsLayout->setSpacing(ShadResultPrivate::kActionSpacing);
    d->_actionsLayout->addStretch(1);
    d->_actionsLayout->addStretch(1);
    d->_actionsWidget->hide();
    columnLayout->addWidget(d->_actionsWidget);
}

ShadResult::ShadResult(ShadResultType::Status status, const QString& title, const QString& description, QWidget* parent)
    : ShadResult(parent)
{
    setStatus(status);
    setTitle(title);
    setDescription(description);
}

ShadResult::~ShadResult()
{
}

void ShadResult::setStatus(ShadResultType::Status status)
{
    Q_D(ShadResult);
    d->_icon->setStatus(status);
}

ShadResultType::Status ShadResult::getStatus() const
{
    Q_D(const ShadResult);
    return d->_icon->getStatus();
}

void ShadResult::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadResult);
    d->_icon->setIcon(icon);
}

ShadIconType::IconName ShadResult::getShadIcon() const
{
    Q_D(const ShadResult);
    return d->_icon->getIcon();
}

void ShadResult::setColor(const QColor& color)
{
    Q_D(ShadResult);
    d->_icon->setColor(color);
}

QColor ShadResult::getColor() const
{
    Q_D(const ShadResult);
    return d->_icon->getColor();
}

void ShadResult::setMediaWidget(QWidget* widget)
{
    Q_D(ShadResult);
    if (d->_mediaWidget == widget)
    {
        return;
    }
    if (!d->_mediaWidget.isNull())
    {
        ShadStyleHelper::clearFocusWithin(d->_mediaWidget);
        d->_columnLayout->removeWidget(d->_mediaWidget);
        d->_mediaWidget->hide();
        d->_mediaWidget->deleteLater();
    }
    d->_mediaWidget = widget;
    if (widget != nullptr)
    {
        d->_columnLayout->insertWidget(0, widget, 0, Qt::AlignHCenter);
        widget->show();
    }
    d->_icon->setVisible(widget == nullptr);
}

QWidget* ShadResult::getMediaWidget() const
{
    Q_D(const ShadResult);
    return d->_mediaWidget.data();
}

void ShadResult::setTitle(const QString& title)
{
    Q_D(ShadResult);
    d->_titleLabel->setText(title);
    d->_titleLabel->setVisible(!title.isEmpty());
}

QString ShadResult::getTitle() const
{
    Q_D(const ShadResult);
    return d->_titleLabel->text();
}

void ShadResult::setDescription(const QString& description)
{
    Q_D(ShadResult);
    d->_descriptionLabel->setText(description);
    d->_descriptionLabel->setVisible(!description.isEmpty());
}

QString ShadResult::getDescription() const
{
    Q_D(const ShadResult);
    return d->_descriptionLabel->text();
}

ShadButton* ShadResult::addButton(const QString& text, ShadButtonType::Variant variant)
{
    Q_D(ShadResult);
    ShadButton* button = new ShadButton(text, d->_actionsWidget);
    button->setVariant(variant);
    addActionWidget(button);
    return button;
}

void ShadResult::addActionWidget(QWidget* widget)
{
    Q_D(ShadResult);
    if (widget == nullptr)
    {
        return;
    }
    d->_actionsLayout->insertWidget(d->_actionsLayout->count() - 1, widget);
    d->_actionsWidget->show();
}

void ShadResult::addContentWidget(QWidget* widget)
{
    Q_D(ShadResult);
    if (widget == nullptr)
    {
        return;
    }
    d->_contentLayout->addWidget(widget);
    d->_contentWidget->show();
}
