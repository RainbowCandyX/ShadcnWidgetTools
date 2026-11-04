#include "ShadDoubleSpinBox.h"

#include <QContextMenuEvent>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

#include "ShadDoubleSpinBoxPrivate.h"
#include "ShadSpinBoxStyle.h"

ShadDoubleSpinBox::ShadDoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent), d_ptr(new ShadDoubleSpinBoxPrivate())
{
    Q_D(ShadDoubleSpinBox);
    d->q_ptr = this;
    d->_style = new ShadSpinBoxStyle(this);
    setStyle(d->_style);
}

ShadDoubleSpinBox::~ShadDoubleSpinBox()
{
}

void ShadDoubleSpinBox::setButtonLayout(ShadSpinBoxType::ButtonLayout layout)
{
    Q_D(ShadDoubleSpinBox);
    d->_style->setButtonLayout(layout);
}

ShadSpinBoxType::ButtonLayout ShadDoubleSpinBox::getButtonLayout() const
{
    Q_D(const ShadDoubleSpinBox);
    return d->_style->getButtonLayout();
}

void ShadDoubleSpinBox::setInvalid(bool invalid)
{
    Q_D(ShadDoubleSpinBox);
    if (d->_style->isInvalid() == invalid)
    {
        return;
    }
    d->_style->setInvalid(invalid);
    Q_EMIT invalidChanged();
}

bool ShadDoubleSpinBox::isInvalid() const
{
    Q_D(const ShadDoubleSpinBox);
    return d->_style->isInvalid();
}

QSize ShadDoubleSpinBox::sizeHint() const
{
    Q_D(const ShadDoubleSpinBox);
    // 最小值、最大值与特殊值文字都能完整显示
    const QStringList texts{prefix() + textFromValue(minimum()) + suffix(), prefix() + textFromValue(maximum()) + suffix(), specialValueText()};
    return d->_style->sizeHintForTexts(texts);
}

QSize ShadDoubleSpinBox::minimumSizeHint() const
{
    return sizeHint();
}

void ShadDoubleSpinBox::contextMenuEvent(QContextMenuEvent* event)
{
    // 键盘打开菜单（菜单键）时在组件中间弹出
    const QPoint globalPos = event->reason() == QContextMenuEvent::Mouse ? event->globalPos() : mapToGlobal(rect().center());
    ShadSpinBoxStyle::showContextMenu(this, lineEdit(), stepEnabled(), globalPos);
    event->accept();
}

void ShadDoubleSpinBox::wheelEvent(QWheelEvent* event)
{
    // 没有焦点时交给父组件滚动页面，避免滚动页面时误改数值
    if (!hasFocus())
    {
        event->ignore();
        return;
    }
    QDoubleSpinBox::wheelEvent(event);
}
