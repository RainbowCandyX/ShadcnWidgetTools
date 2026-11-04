#include "ShadRadioGroup.h"

#include <QAbstractButton>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QLayoutItem>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadRadioButton.h"
#include "ShadRadioGroupPrivate.h"

ShadRadioGroup::ShadRadioGroup(QWidget* parent)
    : ShadRadioGroup(Qt::Vertical, parent)
{
}

ShadRadioGroup::ShadRadioGroup(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadRadioGroupPrivate())
{
    Q_D(ShadRadioGroup);
    d->q_ptr = this;
    d->_orientation = orientation;
    d->_buttonGroup = new QButtonGroup(this);
    d->_buttonGroup->setExclusive(true);
    d->_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->applyOrientation();
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(d->_buttonGroup, &QButtonGroup::idToggled, this, [this](int id, bool checked)
    {
        if (checked)
        {
            Q_EMIT currentIndexChanged(id);
        }
    });
}

ShadRadioGroup::~ShadRadioGroup()
{
}

ShadRadioButton* ShadRadioGroup::addItem(const QString& text)
{
    Q_D(ShadRadioGroup);
    ShadRadioButton* button = new ShadRadioButton(text, this);
    const int index = d->_buttonGroup->buttons().size();
    d->_buttonGroup->addButton(button, index);
    d->_layout->addWidget(button, 0, d->_orientation == Qt::Vertical ? Qt::Alignment(Qt::AlignLeft) : Qt::Alignment());
    return button;
}

void ShadRadioGroup::addItems(const QStringList& texts)
{
    for (const QString& text : texts)
    {
        addItem(text);
    }
}

ShadRadioButton* ShadRadioGroup::itemAt(int index) const
{
    Q_D(const ShadRadioGroup);
    return qobject_cast<ShadRadioButton*>(d->_buttonGroup->button(index));
}

int ShadRadioGroup::count() const
{
    Q_D(const ShadRadioGroup);
    return d->_buttonGroup->buttons().size();
}

void ShadRadioGroup::setCurrentIndex(int index)
{
    Q_D(ShadRadioGroup);
    if (index == getCurrentIndex())
    {
        return;
    }
    if (index < 0)
    {
        // 互斥模式下无法直接取消选中，临时关闭互斥
        QAbstractButton* checkedButton = d->_buttonGroup->checkedButton();
        d->_buttonGroup->setExclusive(false);
        checkedButton->setChecked(false);
        d->_buttonGroup->setExclusive(true);
        Q_EMIT currentIndexChanged(-1);
        return;
    }
    QAbstractButton* button = d->_buttonGroup->button(index);
    if (button == nullptr)
    {
        qWarning("ShadRadioGroup::setCurrentIndex 索引越界: %d，当前选项数: %d", index, count());
        return;
    }
    button->setChecked(true);
}

int ShadRadioGroup::getCurrentIndex() const
{
    Q_D(const ShadRadioGroup);
    return d->_buttonGroup->checkedId();
}

void ShadRadioGroup::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadRadioGroup);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    d->applyOrientation();
    for (int i = 0; i < d->_layout->count(); ++i)
    {
        d->_layout->setAlignment(d->_layout->itemAt(i)->widget(), orientation == Qt::Vertical ? Qt::Alignment(Qt::AlignLeft) : Qt::Alignment());
    }
}

Qt::Orientation ShadRadioGroup::getOrientation() const
{
    Q_D(const ShadRadioGroup);
    return d->_orientation;
}
