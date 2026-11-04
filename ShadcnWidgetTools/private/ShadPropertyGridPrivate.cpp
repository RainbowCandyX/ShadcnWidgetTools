#include "ShadPropertyGridPrivate.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QIcon>
#include <QLineEdit>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QRectF>
#include <QSize>
#include <QSizePolicy>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <limits>
#include <utility>

#include "ShadButton.h"
#include "ShadColorDialog.h"
#include "ShadDef.h"
#include "ShadDoubleSpinBox.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPropertyGrid.h"
#include "ShadPropertyGridGroupHeader.h"
#include "ShadPropertyGridRow.h"
#include "ShadSpinBox.h"
#include "ShadStyleHelper.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"

ShadPropertyGridPrivate::ShadPropertyGridPrivate()
{
}

ShadPropertyGridPrivate::~ShadPropertyGridPrivate()
{
}

int ShadPropertyGridPrivate::propertyIndex(const QString& key) const
{
    for (int i = 0; i < _properties.size(); ++i)
    {
        if (_properties.at(i).key == key)
        {
            return i;
        }
    }
    return -1;
}

int ShadPropertyGridPrivate::groupIndex(const QString& group) const
{
    for (int i = 0; i < _groups.size(); ++i)
    {
        if (_groups.at(i).key == group)
        {
            return i;
        }
    }
    return -1;
}

int ShadPropertyGridPrivate::ensureGroup(const QString& group)
{
    Q_Q(ShadPropertyGrid);
    const int existing = groupIndex(group);
    if (existing >= 0)
    {
        return existing;
    }
    Group entry;
    entry.key = group;
    entry.container = new QWidget(_contentWidget);
    QVBoxLayout* containerLayout = new QVBoxLayout(entry.container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    entry.body = new QWidget(entry.container);
    entry.bodyLayout = new QVBoxLayout(entry.body);
    entry.bodyLayout->setContentsMargins(0, 0, 0, 0);
    entry.bodyLayout->setSpacing(0);
    if (!group.isEmpty())
    {
        entry.header = new ShadPropertyGridGroupHeader(entry.container);
        entry.header->setText(group);
        containerLayout->addWidget(entry.header);
        QWidget* body = entry.body;
        QObject::connect(entry.header, &QAbstractButton::toggled, q, [body](bool checked) { body->setVisible(checked); });
    }
    containerLayout->addWidget(entry.body);
    if (group.isEmpty())
    {
        _contentLayout->insertWidget(0, entry.container);
        _groups.prepend(entry);
        return 0;
    }
    _contentLayout->insertWidget(_contentLayout->count() - 1, entry.container);
    _groups.append(entry);
    return _groups.size() - 1;
}

void ShadPropertyGridPrivate::removeGroupAt(int index)
{
    Q_Q(ShadPropertyGrid);
    const Group group = _groups.takeAt(index);
    if (group.header != nullptr)
    {
        QObject::disconnect(group.header, nullptr, q, nullptr);
    }
    ShadStyleHelper::clearFocusWithin(group.container);
    group.container->hide();
    group.container->deleteLater();
}

bool ShadPropertyGridPrivate::insertProperty(const QString& key, const QString& label, PropertyType type, QWidget* editor, const QString& group)
{
    Q_Q(ShadPropertyGrid);
    const int index = ensureGroup(group);
    ShadPropertyGridRow* row = new ShadPropertyGridRow(_groups.at(index).body);
    row->setLabel(label);
    row->setLabelWidth(_labelWidth);
    row->setEditor(editor);
    _groups.at(index).bodyLayout->addWidget(row);
    QObject::connect(row, &ShadPropertyGridRow::pressed, q, [this, key]() { setCurrentKey(key); });
    QObject::connect(row, &ShadPropertyGridRow::labelWidthDragged, q, [q](int width) { q->setLabelWidth(width); });
    Property property;
    property.key = key;
    property.group = group;
    property.type = type;
    property.editor = editor;
    property.row = row;
    _properties.append(property);
    connectEditor(key, type, editor);
    return true;
}

QWidget* ShadPropertyGridPrivate::createEditor(PropertyType type, const QVariant& value)
{
    QWidget* editor = nullptr;
    switch (type)
    {
        case PropertyType::String:
            {
                ShadInput* input = new ShadInput();
                input->setText(value.toString());
                editor = input;
                break;
            }
        case PropertyType::Int:
            {
                ShadSpinBox* spinBox = new ShadSpinBox();
                spinBox->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                spinBox->setValue(value.toInt());
                editor = spinBox;
                break;
            }
        case PropertyType::Double:
            {
                ShadDoubleSpinBox* spinBox = new ShadDoubleSpinBox();
                spinBox->setRange(-kDoubleLimit, kDoubleLimit);
                spinBox->setValue(value.toDouble());
                editor = spinBox;
                break;
            }
        case PropertyType::Bool:
            {
                ShadSwitch* switchButton = new ShadSwitch();
                switchButton->setChecked(value.toBool());
                switchButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                return switchButton;
            }
        case PropertyType::Color:
            {
                ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline);
                button->setContentAlignment(Qt::AlignLeft);
                updateColorButton(button, value.value<QColor>());
                editor = button;
                break;
            }
        case PropertyType::Enum:
        case PropertyType::Widget:
            return nullptr;
    }
    if (editor != nullptr)
    {
        editor->setSizePolicy(QSizePolicy::Ignored, editor->sizePolicy().verticalPolicy());
    }
    return editor;
}

void ShadPropertyGridPrivate::connectEditor(const QString& key, PropertyType type, QWidget* editor)
{
    Q_Q(ShadPropertyGrid);
    switch (type)
    {
        case PropertyType::String:
            QObject::connect(static_cast<QLineEdit*>(editor), &QLineEdit::textChanged, q, [q, key](const QString& text) { Q_EMIT q->valueChanged(key, text); });
            break;
        case PropertyType::Int:
            QObject::connect(static_cast<QSpinBox*>(editor), &QSpinBox::valueChanged, q, [q, key](int value) { Q_EMIT q->valueChanged(key, value); });
            break;
        case PropertyType::Double:
            QObject::connect(static_cast<QDoubleSpinBox*>(editor), &QDoubleSpinBox::valueChanged, q, [q, key](double value) { Q_EMIT q->valueChanged(key, value); });
            break;
        case PropertyType::Bool:
            QObject::connect(static_cast<QAbstractButton*>(editor), &QAbstractButton::toggled, q, [q, key](bool checked) { Q_EMIT q->valueChanged(key, checked); });
            break;
        case PropertyType::Color:
            QObject::connect(static_cast<QAbstractButton*>(editor), &QAbstractButton::clicked, q, [this, key]() { pickColor(key); });
            break;
        case PropertyType::Enum:
            QObject::connect(static_cast<QComboBox*>(editor), &QComboBox::currentIndexChanged, q, [q, key](int index) { Q_EMIT q->valueChanged(key, index); });
            break;
        case PropertyType::Widget:
            break;
    }
}

void ShadPropertyGridPrivate::applyReadOnly(const Property& property)
{
    switch (property.type)
    {
        case PropertyType::String:
            static_cast<QLineEdit*>(property.editor)->setReadOnly(property.isReadOnly);
            break;
        case PropertyType::Int:
        case PropertyType::Double:
            static_cast<QAbstractSpinBox*>(property.editor)->setReadOnly(property.isReadOnly);
            break;
        case PropertyType::Bool:
        case PropertyType::Color:
        case PropertyType::Enum:
        case PropertyType::Widget:
            property.editor->setEnabled(!property.isReadOnly);
            break;
    }
}

void ShadPropertyGridPrivate::updateColorButton(ShadButton* button, const QColor& color) const
{
    button->setText(color.name(QColor::HexRgb).toUpper());
    button->setIcon(QIcon(swatchPixmap(color)));
}

QPixmap ShadPropertyGridPrivate::swatchPixmap(const QColor& color) const
{
    Q_Q(const ShadPropertyGrid);
    const qreal devicePixelRatio = q->devicePixelRatioF();
    QPixmap pixmap(QSize(kSwatchSize, kSwatchSize) * devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF swatchRect(0.5, 0.5, kSwatchSize - 1.0, kSwatchSize - 1.0);
    ShadStyleHelper::fillRoundedRect(&painter, swatchRect, kSwatchRadius, color);
    ShadStyleHelper::strokeRoundedRect(&painter, swatchRect, kSwatchRadius, shadTheme->getColor(ShadThemeType::Border));
    return pixmap;
}

void ShadPropertyGridPrivate::refreshColorButtons()
{
    for (const Property& property : std::as_const(_properties))
    {
        if (property.type == PropertyType::Color)
        {
            updateColorButton(static_cast<ShadButton*>(property.editor), property.color);
        }
    }
}

void ShadPropertyGridPrivate::pickColor(const QString& key)
{
    Q_Q(ShadPropertyGrid);
    const int index = propertyIndex(key);
    if (index < 0)
    {
        return;
    }
    setCurrentKey(key);
    const QPointer<ShadPropertyGrid> guard(q);
    const QColor color = ShadColorDialog::getColor(_properties.at(index).color, q, QString());
    if (guard.isNull() || !color.isValid())
    {
        return;
    }
    const int current = propertyIndex(key);
    if (current < 0 || _properties.at(current).color == color)
    {
        return;
    }
    _properties[current].color = color;
    updateColorButton(static_cast<ShadButton*>(_properties.at(current).editor), color);
    Q_EMIT q->valueChanged(key, color);
}

void ShadPropertyGridPrivate::setCurrentKey(const QString& key)
{
    Q_Q(ShadPropertyGrid);
    if (_currentKey == key)
    {
        return;
    }
    _currentKey = key;
    for (const Property& property : std::as_const(_properties))
    {
        property.row->setCurrent(property.key == key);
    }
    updateDescription();
    Q_EMIT q->currentKeyChanged(key);
}

void ShadPropertyGridPrivate::updateDescription()
{
    const int index = propertyIndex(_currentKey);
    if (index < 0)
    {
        _descriptionTitle->clear();
        _descriptionText->clear();
        return;
    }
    const Property& property = _properties.at(index);
    _descriptionTitle->setText(property.row->getLabel());
    _descriptionText->setText(property.description);
}

void ShadPropertyGridPrivate::handleFocusChanged(QWidget* now)
{
    Q_Q(ShadPropertyGrid);
    for (QWidget* widget = now; widget != nullptr && widget != q; widget = widget->parentWidget())
    {
        for (const Property& property : std::as_const(_properties))
        {
            if (property.row == widget)
            {
                setCurrentKey(property.key);
                return;
            }
        }
    }
}

void ShadPropertyGridPrivate::disconnectChildren()
{
    Q_Q(ShadPropertyGrid);
    for (const Property& property : std::as_const(_properties))
    {
        QObject::disconnect(property.editor, nullptr, q, nullptr);
        QObject::disconnect(property.row, nullptr, q, nullptr);
    }
    for (const Group& group : std::as_const(_groups))
    {
        if (group.header != nullptr)
        {
            QObject::disconnect(group.header, nullptr, q, nullptr);
        }
    }
}
