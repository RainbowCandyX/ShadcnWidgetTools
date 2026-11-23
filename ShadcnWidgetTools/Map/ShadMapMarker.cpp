#include "ShadMapMarker.h"

ShadMapMarker::ShadMapMarker(const ShadMapCoordinate& coordinate, QObject* parent)
    : QObject(parent), _coordinate(coordinate)
{
}

ShadMapMarker::~ShadMapMarker()
{
    delete _content.data();
}

void ShadMapMarker::setCoordinate(const ShadMapCoordinate& coordinate)
{
    if (_coordinate == coordinate)
    {
        return;
    }
    _coordinate = coordinate;
    Q_EMIT changed();
}

ShadMapCoordinate ShadMapMarker::getCoordinate() const
{
    return _coordinate;
}

void ShadMapMarker::setColor(const QColor& color)
{
    if (_color == color)
    {
        return;
    }
    _color = color;
    Q_EMIT changed();
}

QColor ShadMapMarker::getColor() const
{
    return _color;
}

void ShadMapMarker::setRingColor(const QColor& color)
{
    if (_ringColor == color)
    {
        return;
    }
    _ringColor = color;
    Q_EMIT changed();
}

QColor ShadMapMarker::getRingColor() const
{
    return _ringColor;
}

void ShadMapMarker::setLabel(const QString& label)
{
    if (_label == label)
    {
        return;
    }
    _label = label;
    Q_EMIT changed();
}

QString ShadMapMarker::getLabel() const
{
    return _label;
}

void ShadMapMarker::setLabelPosition(ShadMapType::LabelPosition position)
{
    if (_labelPosition == position)
    {
        return;
    }
    _labelPosition = position;
    Q_EMIT changed();
}

ShadMapType::LabelPosition ShadMapMarker::getLabelPosition() const
{
    return _labelPosition;
}

void ShadMapMarker::setText(const QString& text)
{
    if (_text == text)
    {
        return;
    }
    _text = text;
    Q_EMIT changed();
}

QString ShadMapMarker::getText() const
{
    return _text;
}

void ShadMapMarker::setSelected(bool selected)
{
    if (_isSelected == selected)
    {
        return;
    }
    _isSelected = selected;
    Q_EMIT changed();
}

bool ShadMapMarker::isSelected() const
{
    return _isSelected;
}

void ShadMapMarker::setContentWidget(QWidget* widget)
{
    if (_content == widget)
    {
        return;
    }
    if (!_content.isNull())
    {
        _content->hide();
        _content->deleteLater();
    }
    _content = widget;
    if (widget != nullptr)
    {
        widget->setParent(qobject_cast<QWidget*>(parent()));
        widget->lower();
    }
    Q_EMIT changed();
}

QWidget* ShadMapMarker::getContentWidget() const
{
    return _content.data();
}

void ShadMapMarker::setToolTip(const QString& toolTip)
{
    _toolTip = toolTip;
}

QString ShadMapMarker::getToolTip() const
{
    return _toolTip;
}

void ShadMapMarker::setPopupTitle(const QString& title)
{
    if (_popupTitle == title)
    {
        return;
    }
    _popupTitle = title;
    Q_EMIT changed();
}

QString ShadMapMarker::getPopupTitle() const
{
    return _popupTitle;
}

void ShadMapMarker::setPopupText(const QString& text)
{
    if (_popupText == text)
    {
        return;
    }
    _popupText = text;
    Q_EMIT changed();
}

QString ShadMapMarker::getPopupText() const
{
    return _popupText;
}

void ShadMapMarker::setDraggable(bool draggable)
{
    _isDraggable = draggable;
}

bool ShadMapMarker::isDraggable() const
{
    return _isDraggable;
}
