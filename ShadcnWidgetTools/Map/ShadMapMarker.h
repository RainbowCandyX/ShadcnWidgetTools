#ifndef SHADMAPMARKER_H
#define SHADMAPMARKER_H

#include <QColor>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QWidget>

#include "ShadMapDef.h"

class ShadMap;

class SHADCN_MAP_EXPORT ShadMapMarker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
    Q_PROPERTY(QColor ringColor READ getRingColor WRITE setRingColor)
    Q_PROPERTY(QString label READ getLabel WRITE setLabel)
    Q_PROPERTY(ShadMapType::LabelPosition labelPosition READ getLabelPosition WRITE setLabelPosition)
    Q_PROPERTY(QString toolTip READ getToolTip WRITE setToolTip)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected)
    Q_PROPERTY(QString popupTitle READ getPopupTitle WRITE setPopupTitle)
    Q_PROPERTY(QString popupText READ getPopupText WRITE setPopupText)
    Q_PROPERTY(bool draggable READ isDraggable WRITE setDraggable)

    public:
        ~ShadMapMarker() override;

        void setCoordinate(const ShadMapCoordinate& coordinate);
        ShadMapCoordinate getCoordinate() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setRingColor(const QColor& color);
        QColor getRingColor() const;

        void setLabel(const QString& label);
        QString getLabel() const;

        void setLabelPosition(ShadMapType::LabelPosition position);
        ShadMapType::LabelPosition getLabelPosition() const;

        void setText(const QString& text);
        QString getText() const;

        void setSelected(bool selected);
        bool isSelected() const;

        void setContentWidget(QWidget* widget);
        QWidget* getContentWidget() const;

        void setToolTip(const QString& toolTip);
        QString getToolTip() const;

        void setPopupTitle(const QString& title);
        QString getPopupTitle() const;

        void setPopupText(const QString& text);
        QString getPopupText() const;

        void setDraggable(bool draggable);
        bool isDraggable() const;

    Q_SIGNALS:
        void changed();
        void clicked();
        void dragEnded(const ShadMapCoordinate& coordinate);

    private:
        ShadMapMarker(const ShadMapCoordinate& coordinate, QObject* parent);

        friend class ShadMap;

        ShadMapCoordinate _coordinate;
        QColor _color;
        QColor _ringColor;
        QString _label;
        ShadMapType::LabelPosition _labelPosition{ShadMapType::Top};
        QString _text;
        QPointer<QWidget> _content;
        QString _toolTip;
        QString _popupTitle;
        QString _popupText;
        bool _isSelected{false};
        bool _isDraggable{false};
};

#endif // SHADMAPMARKER_H
