#include "ShadSpinBox.h"

#include <QContextMenuEvent>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

#include "ShadSpinBoxPrivate.h"
#include "ShadSpinBoxStyle.h"

ShadSpinBox::ShadSpinBox(QWidget* parent)
    : QSpinBox(parent), d_ptr(new ShadSpinBoxPrivate())
{
    Q_D(ShadSpinBox);
    d->q_ptr = this;
    d->_style = new ShadSpinBoxStyle(this);
    setStyle(d->_style);
}

ShadSpinBox::~ShadSpinBox()
{
}

void ShadSpinBox::setButtonLayout(ShadSpinBoxType::ButtonLayout layout)
{
    Q_D(ShadSpinBox);
    d->_style->setButtonLayout(layout);
}

ShadSpinBoxType::ButtonLayout ShadSpinBox::getButtonLayout() const
{
    Q_D(const ShadSpinBox);
    return d->_style->getButtonLayout();
}

void ShadSpinBox::setInvalid(bool invalid)
{
    Q_D(ShadSpinBox);
    if (d->_style->isInvalid() == invalid)
    {
        return;
    }
    d->_style->setInvalid(invalid);
    Q_EMIT invalidChanged();
}

bool ShadSpinBox::isInvalid() const
{
    Q_D(const ShadSpinBox);
    return d->_style->isInvalid();
}

QSize ShadSpinBox::sizeHint() const
{
    Q_D(const ShadSpinBox);
    // 最小值、最大值与特殊值文字都能完整显示
    const QStringList texts{prefix() + textFromValue(minimum()) + suffix(), prefix() + textFromValue(maximum()) + suffix(), specialValueText()};
    return d->_style->sizeHintForTexts(texts);
}

QSize ShadSpinBox::minimumSizeHint() const
{
    return sizeHint();
}

void ShadSpinBox::contextMenuEvent(QContextMenuEvent* event)
{
    // 键盘打开菜单（菜单键）时在组件中间弹出
    const QPoint globalPos = event->reason() == QContextMenuEvent::Mouse ? event->globalPos() : mapToGlobal(rect().center());
    ShadSpinBoxStyle::showContextMenu(this, lineEdit(), stepEnabled(), globalPos);
    event->accept();
}

void ShadSpinBox::wheelEvent(QWheelEvent* event)
{
    // 没有焦点时交给父组件滚动页面，避免滚动页面时误改数值
    if (!hasFocus())
    {
        event->ignore();
        return;
    }
    QSpinBox::wheelEvent(event);
}
