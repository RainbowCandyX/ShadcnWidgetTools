#include "ShadPropertyGrid.h"

#include <QAbstractButton>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFont>
#include <QLineEdit>
#include <QMetaType>
#include <QObject>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadButton.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadPropertyGridGroupHeader.h"
#include "ShadPropertyGridPrivate.h"
#include "ShadPropertyGridRow.h"
#include "ShadScrollArea.h"
#include "ShadSeparator.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kTextFontSize = 14;
} // namespace

ShadPropertyGrid::ShadPropertyGrid(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadPropertyGridPrivate())
{
    Q_D(ShadPropertyGrid);
    d->q_ptr = this;
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    d->_scrollArea = new ShadScrollArea(this);
    d->_contentWidget = new QWidget(d->_scrollArea);
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(0);
    d->_contentLayout->addStretch(1);
    d->_scrollArea->setWidget(d->_contentWidget);
    layout->addWidget(d->_scrollArea, 1);

    d->_descriptionSeparator = new ShadSeparator(Qt::Horizontal, this);
    layout->addWidget(d->_descriptionSeparator);
    d->_descriptionArea = new QWidget(this);
    d->_descriptionArea->setFixedHeight(ShadPropertyGridPrivate::kDescriptionHeight);
    QVBoxLayout* descriptionLayout = new QVBoxLayout(d->_descriptionArea);
    const int padding = ShadPropertyGridPrivate::kDescriptionPadding;
    descriptionLayout->setContentsMargins(padding, padding, padding, padding);
    descriptionLayout->setSpacing(ShadPropertyGridPrivate::kDescriptionSpacing);
    d->_descriptionTitle = new ShadLabel(d->_descriptionArea);
    d->_descriptionTitle->setTextFormat(Qt::PlainText);
    ShadStyleHelper::applyFont(d->_descriptionTitle, kTextFontSize, QFont::Medium);
    d->_descriptionText = new ShadLabel(d->_descriptionArea);
    d->_descriptionText->setTextFormat(Qt::PlainText);
    d->_descriptionText->setWordWrap(true);
    d->_descriptionText->setAlignment(Qt::AlignLeading | Qt::AlignTop);
    d->_descriptionText->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(d->_descriptionText, kTextFontSize, QFont::Normal);
    descriptionLayout->addWidget(d->_descriptionTitle);
    descriptionLayout->addWidget(d->_descriptionText, 1);
    layout->addWidget(d->_descriptionArea);

    d->_focusConnection = connect(qApp, &QApplication::focusChanged, this, [d](QWidget* old, QWidget* now)
    {
        Q_UNUSED(old);
        d->handleFocusChanged(now);
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->refreshColorButtons(); });
}

ShadPropertyGrid::~ShadPropertyGrid()
{
    Q_D(ShadPropertyGrid);
    QObject::disconnect(d->_focusConnection);
    d->disconnectChildren();
}

bool ShadPropertyGrid::addProperty(const QString& key, const QString& label, const QVariant& value, const QString& group)
{
    Q_D(ShadPropertyGrid);
    using PropertyType = ShadPropertyGridPrivate::PropertyType;
    if (hasProperty(key))
    {
        return false;
    }
    PropertyType type = PropertyType::String;
    switch (value.typeId())
    {
        case QMetaType::QString:
            type = PropertyType::String;
            break;
        case QMetaType::Int:
            type = PropertyType::Int;
            break;
        case QMetaType::Double:
            type = PropertyType::Double;
            break;
        case QMetaType::Bool:
            type = PropertyType::Bool;
            break;
        case QMetaType::QColor:
            type = PropertyType::Color;
            break;
        default:
            return false;
    }
    QWidget* editor = d->createEditor(type, value);
    d->insertProperty(key, label, type, editor, group);
    if (type == PropertyType::Color)
    {
        d->_properties.last().color = value.value<QColor>();
    }
    return true;
}

bool ShadPropertyGrid::addEnumProperty(const QString& key, const QString& label, const QStringList& options, int currentIndex, const QString& group)
{
    Q_D(ShadPropertyGrid);
    if (hasProperty(key))
    {
        return false;
    }
    ShadComboBox* comboBox = new ShadComboBox();
    comboBox->addItems(options);
    comboBox->setCurrentIndex(currentIndex);
    comboBox->setSizePolicy(QSizePolicy::Ignored, comboBox->sizePolicy().verticalPolicy());
    return d->insertProperty(key, label, ShadPropertyGridPrivate::PropertyType::Enum, comboBox, group);
}

bool ShadPropertyGrid::addWidgetProperty(const QString& key, const QString& label, QWidget* editor, const QString& group)
{
    Q_D(ShadPropertyGrid);
    if (editor == nullptr || hasProperty(key))
    {
        return false;
    }
    return d->insertProperty(key, label, ShadPropertyGridPrivate::PropertyType::Widget, editor, group);
}

void ShadPropertyGrid::removeProperty(const QString& key)
{
    Q_D(ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    if (d->_currentKey == key)
    {
        d->setCurrentKey(QString());
    }
    const ShadPropertyGridPrivate::Property property = d->_properties.takeAt(index);
    QObject::disconnect(property.editor, nullptr, this, nullptr);
    QObject::disconnect(property.row, nullptr, this, nullptr);
    ShadStyleHelper::clearFocusWithin(property.row);
    property.row->hide();
    property.row->deleteLater();
    for (const ShadPropertyGridPrivate::Property& other : std::as_const(d->_properties))
    {
        if (other.group == property.group)
        {
            return;
        }
    }
    const int groupIndex = d->groupIndex(property.group);
    if (groupIndex >= 0)
    {
        d->removeGroupAt(groupIndex);
    }
}

void ShadPropertyGrid::clear()
{
    Q_D(ShadPropertyGrid);
    d->setCurrentKey(QString());
    d->disconnectChildren();
    d->_properties.clear();
    while (!d->_groups.isEmpty())
    {
        d->removeGroupAt(d->_groups.size() - 1);
    }
}

bool ShadPropertyGrid::hasProperty(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    return d->propertyIndex(key) >= 0;
}

QStringList ShadPropertyGrid::getKeys() const
{
    Q_D(const ShadPropertyGrid);
    QStringList keys;
    keys.reserve(d->_properties.size());
    for (const ShadPropertyGridPrivate::Property& property : std::as_const(d->_properties))
    {
        keys.append(property.key);
    }
    return keys;
}

void ShadPropertyGrid::setValue(const QString& key, const QVariant& value)
{
    Q_D(ShadPropertyGrid);
    using PropertyType = ShadPropertyGridPrivate::PropertyType;
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    ShadPropertyGridPrivate::Property& property = d->_properties[index];
    const QSignalBlocker blocker(property.editor);
    switch (property.type)
    {
        case PropertyType::String:
            static_cast<QLineEdit*>(property.editor)->setText(value.toString());
            break;
        case PropertyType::Int:
            static_cast<QSpinBox*>(property.editor)->setValue(value.toInt());
            break;
        case PropertyType::Double:
            static_cast<QDoubleSpinBox*>(property.editor)->setValue(value.toDouble());
            break;
        case PropertyType::Bool:
            static_cast<QAbstractButton*>(property.editor)->setChecked(value.toBool());
            break;
        case PropertyType::Color:
            {
                const QColor color = value.value<QColor>();
                if (color.isValid())
                {
                    property.color = color;
                    d->updateColorButton(static_cast<ShadButton*>(property.editor), color);
                }
                break;
            }
        case PropertyType::Enum:
            static_cast<QComboBox*>(property.editor)->setCurrentIndex(value.toInt());
            break;
        case PropertyType::Widget:
            break;
    }
}

QVariant ShadPropertyGrid::getValue(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    using PropertyType = ShadPropertyGridPrivate::PropertyType;
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return QVariant();
    }
    const ShadPropertyGridPrivate::Property& property = d->_properties.at(index);
    switch (property.type)
    {
        case PropertyType::String:
            return static_cast<const QLineEdit*>(property.editor)->text();
        case PropertyType::Int:
            return static_cast<const QSpinBox*>(property.editor)->value();
        case PropertyType::Double:
            return static_cast<const QDoubleSpinBox*>(property.editor)->value();
        case PropertyType::Bool:
            return static_cast<const QAbstractButton*>(property.editor)->isChecked();
        case PropertyType::Color:
            return property.color;
        case PropertyType::Enum:
            return static_cast<const QComboBox*>(property.editor)->currentIndex();
        case PropertyType::Widget:
            break;
    }
    return QVariant();
}

void ShadPropertyGrid::setPropertyLabel(const QString& key, const QString& label)
{
    Q_D(ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    d->_properties.at(index).row->setLabel(label);
    if (d->_currentKey == key)
    {
        d->updateDescription();
    }
}

QString ShadPropertyGrid::getPropertyLabel(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    return index >= 0 ? d->_properties.at(index).row->getLabel() : QString();
}

void ShadPropertyGrid::setPropertyDescription(const QString& key, const QString& description)
{
    Q_D(ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    d->_properties[index].description = description;
    if (d->_currentKey == key)
    {
        d->updateDescription();
    }
}

QString ShadPropertyGrid::getPropertyDescription(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    return index >= 0 ? d->_properties.at(index).description : QString();
}

void ShadPropertyGrid::setPropertyReadOnly(const QString& key, bool readOnly)
{
    Q_D(ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    if (index < 0 || d->_properties.at(index).isReadOnly == readOnly)
    {
        return;
    }
    d->_properties[index].isReadOnly = readOnly;
    d->applyReadOnly(d->_properties.at(index));
}

bool ShadPropertyGrid::isPropertyReadOnly(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    return index >= 0 && d->_properties.at(index).isReadOnly;
}

void ShadPropertyGrid::setPropertyRange(const QString& key, const QVariant& minimum, const QVariant& maximum)
{
    Q_D(ShadPropertyGrid);
    using PropertyType = ShadPropertyGridPrivate::PropertyType;
    const int index = d->propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    const ShadPropertyGridPrivate::Property& property = d->_properties.at(index);
    if (property.type == PropertyType::Int)
    {
        static_cast<QSpinBox*>(property.editor)->setRange(minimum.toInt(), maximum.toInt());
    }
    else if (property.type == PropertyType::Double)
    {
        static_cast<QDoubleSpinBox*>(property.editor)->setRange(minimum.toDouble(), maximum.toDouble());
    }
}

void ShadPropertyGrid::setEnumOptions(const QString& key, const QStringList& options)
{
    Q_D(ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    if (index < 0 || d->_properties.at(index).type != ShadPropertyGridPrivate::PropertyType::Enum)
    {
        return;
    }
    QComboBox* comboBox = static_cast<QComboBox*>(d->_properties.at(index).editor);
    const QSignalBlocker blocker(comboBox);
    const int currentIndex = comboBox->currentIndex();
    comboBox->clear();
    comboBox->addItems(options);
    comboBox->setCurrentIndex(qMin(currentIndex, static_cast<int>(options.size()) - 1));
}

QWidget* ShadPropertyGrid::getEditor(const QString& key) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->propertyIndex(key);
    return index >= 0 ? d->_properties.at(index).editor : nullptr;
}

void ShadPropertyGrid::setGroupTitle(const QString& group, const QString& title)
{
    Q_D(ShadPropertyGrid);
    const int index = d->groupIndex(group);
    if (index >= 0 && d->_groups.at(index).header != nullptr)
    {
        d->_groups.at(index).header->setText(title);
    }
}

QString ShadPropertyGrid::getGroupTitle(const QString& group) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->groupIndex(group);
    if (index < 0 || d->_groups.at(index).header == nullptr)
    {
        return QString();
    }
    return d->_groups.at(index).header->text();
}

void ShadPropertyGrid::setGroupExpanded(const QString& group, bool expanded)
{
    Q_D(ShadPropertyGrid);
    const int index = d->groupIndex(group);
    if (index >= 0 && d->_groups.at(index).header != nullptr)
    {
        d->_groups.at(index).header->setChecked(expanded);
    }
}

bool ShadPropertyGrid::isGroupExpanded(const QString& group) const
{
    Q_D(const ShadPropertyGrid);
    const int index = d->groupIndex(group);
    if (index < 0 || d->_groups.at(index).header == nullptr)
    {
        return true;
    }
    return d->_groups.at(index).header->isChecked();
}

void ShadPropertyGrid::setLabelWidth(int width)
{
    Q_D(ShadPropertyGrid);
    const int labelWidth = qMax(ShadPropertyGridRow::kMinLabelWidth, width);
    if (d->_labelWidth == labelWidth)
    {
        return;
    }
    d->_labelWidth = labelWidth;
    for (const ShadPropertyGridPrivate::Property& property : std::as_const(d->_properties))
    {
        property.row->setLabelWidth(labelWidth);
    }
}

int ShadPropertyGrid::getLabelWidth() const
{
    Q_D(const ShadPropertyGrid);
    return d->_labelWidth;
}

void ShadPropertyGrid::setDescriptionVisible(bool visible)
{
    Q_D(ShadPropertyGrid);
    if (d->_isDescriptionVisible == visible)
    {
        return;
    }
    d->_isDescriptionVisible = visible;
    d->_descriptionSeparator->setVisible(visible);
    d->_descriptionArea->setVisible(visible);
}

bool ShadPropertyGrid::isDescriptionVisible() const
{
    Q_D(const ShadPropertyGrid);
    return d->_isDescriptionVisible;
}

QString ShadPropertyGrid::getCurrentKey() const
{
    Q_D(const ShadPropertyGrid);
    return d->_currentKey;
}

bool ShadPropertyGrid::event(QEvent* event)
{
    Q_D(ShadPropertyGrid);
    if (event->type() == QEvent::Show)
    {
        d->refreshColorButtons();
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    else if (event->type() == QEvent::DevicePixelRatioChange)
    {
        d->refreshColorButtons();
    }
#endif
    return QWidget::event(event);
}
