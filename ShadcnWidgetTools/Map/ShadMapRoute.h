#ifndef SHADMAPROUTE_H
#define SHADMAPROUTE_H

#include <QColor>
#include <QList>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "ShadMapDef.h"

class ShadMap;

class SHADCN_MAP_EXPORT ShadMapRoute : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
    Q_PROPERTY(qreal width READ getWidth WRITE setWidth)
    Q_PROPERTY(qreal opacity READ getOpacity WRITE setOpacity)

    public:
        ~ShadMapRoute() override;

        void setCoordinates(const QList<ShadMapCoordinate>& coordinates);
        QList<ShadMapCoordinate> getCoordinates() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setWidth(qreal width);
        qreal getWidth() const;

        void setOpacity(qreal opacity);
        qreal getOpacity() const;

        void setDashPattern(const QList<qreal>& pattern);
        QList<qreal> getDashPattern() const;

    Q_SIGNALS:
        void changed();

    private:
        ShadMapRoute(const QList<ShadMapCoordinate>& coordinates, QObject* parent);

        friend class ShadMap;

        QList<ShadMapCoordinate> _coordinates;
        QColor _color;
        qreal _width{4.0};
        qreal _opacity{1.0};
        QList<qreal> _dashPattern;
};

class SHADCN_MAP_EXPORT ShadMapArc : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
    Q_PROPERTY(qreal width READ getWidth WRITE setWidth)
    Q_PROPERTY(qreal curvature READ getCurvature WRITE setCurvature)
    Q_PROPERTY(QString id READ getId WRITE setId)

    public:
        ~ShadMapArc() override;

        void setId(const QString& id);
        QString getId() const;

        void setEndpoints(const ShadMapCoordinate& from, const ShadMapCoordinate& to);
        ShadMapCoordinate getFrom() const;
        ShadMapCoordinate getTo() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setWidth(qreal width);
        qreal getWidth() const;

        void setCurvature(qreal curvature);
        qreal getCurvature() const;

    Q_SIGNALS:
        void changed();

    private:
        ShadMapArc(const ShadMapCoordinate& from, const ShadMapCoordinate& to, QObject* parent);

        friend class ShadMap;

        QString _id;
        ShadMapCoordinate _from;
        ShadMapCoordinate _to;
        QColor _color;
        qreal _width{2.0};
        qreal _curvature{0.25};
};

#endif // SHADMAPROUTE_H
