#ifndef SHADMAPVECTORTILE_H
#define SHADMAPVECTORTILE_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPolygonF>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

class ShadMapVectorTile
{
    public:
        enum GeometryType
        {
            Unknown = 0,
            Point,
            LineString,
            Polygon,
        };

        struct Feature
        {
            GeometryType type{Unknown};
            QList<quint32> tags;
            QList<QPolygonF> parts;
        };

        struct Layer
        {
            QString name;
            double extent{4096.0};
            QStringList keys;
            QVariantList values;
            QList<Feature> features;

            QVariant property(const Feature& feature, const QString& key) const;
        };

        static QSharedPointer<const ShadMapVectorTile> decode(const QByteArray& data);
        static bool isCompressed(const QByteArray& data);

        const Layer* layer(const QString& name) const;
        qsizetype byteCost() const;

    private:
        QHash<QString, Layer> _layers;
        qsizetype _byteCost{0};
};

#endif // SHADMAPVECTORTILE_H
