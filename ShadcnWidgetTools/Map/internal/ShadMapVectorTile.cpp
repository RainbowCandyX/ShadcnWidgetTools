#include "ShadMapVectorTile.h"

#include <QByteArrayView>
#include <QPointF>
#include <QSet>
#include <QtEndian>

#include <cstring>
#include <utility>

namespace
{
    constexpr quint32 kWireVarint = 0;
    constexpr quint32 kWireFixed64 = 1;
    constexpr quint32 kWireBytes = 2;
    constexpr quint32 kWireFixed32 = 5;
    constexpr quint32 kCommandMoveTo = 1;
    constexpr quint32 kCommandLineTo = 2;
    constexpr quint32 kCommandClosePath = 7;

    class ProtoReader
    {
        public:
            explicit ProtoReader(QByteArrayView data)
                : _data(data)
            {
            }

            bool next()
            {
                if (_hasError || _position >= _data.size())
                {
                    return false;
                }
                const quint64 key = varint();
                _field = quint32(key >> 3);
                _wireType = quint32(key & 0x7);
                return !_hasError;
            }

            quint32 field() const
            {
                return _field;
            }

            quint32 wireType() const
            {
                return _wireType;
            }

            bool hasError() const
            {
                return _hasError;
            }

            bool atEnd() const
            {
                return _position >= _data.size();
            }

            quint64 varint()
            {
                quint64 result = 0;
                for (int shift = 0; shift < 64; shift += 7)
                {
                    if (_position >= _data.size())
                    {
                        break;
                    }
                    const quint8 byte = quint8(_data.at(_position++));
                    result |= quint64(byte & 0x7f) << shift;
                    if ((byte & 0x80) == 0)
                    {
                        return result;
                    }
                }
                _hasError = true;
                return 0;
            }

            QByteArrayView bytes()
            {
                const quint64 length = varint();
                if (_hasError || length > quint64(_data.size() - _position))
                {
                    _hasError = true;
                    return QByteArrayView();
                }
                const QByteArrayView result = _data.sliced(_position, qsizetype(length));
                _position += qsizetype(length);
                return result;
            }

            quint32 fixed32()
            {
                if (_data.size() - _position < 4)
                {
                    _hasError = true;
                    return 0;
                }
                const quint32 value = qFromLittleEndian<quint32>(_data.data() + _position);
                _position += 4;
                return value;
            }

            quint64 fixed64()
            {
                if (_data.size() - _position < 8)
                {
                    _hasError = true;
                    return 0;
                }
                const quint64 value = qFromLittleEndian<quint64>(_data.data() + _position);
                _position += 8;
                return value;
            }

            void skip()
            {
                switch (_wireType)
                {
                    case kWireVarint:
                        varint();
                        break;
                    case kWireFixed64:
                        fixed64();
                        break;
                    case kWireBytes:
                        bytes();
                        break;
                    case kWireFixed32:
                        fixed32();
                        break;
                    default:
                        _hasError = true;
                        break;
                }
            }

        private:
            QByteArrayView _data;
            qsizetype _position{0};
            quint32 _field{0};
            quint32 _wireType{0};
            bool _hasError{false};
    };

    qint64 zigzag(quint64 value)
    {
        return qint64(value >> 1) ^ -qint64(value & 1);
    }

    const QSet<QString>& wantedLayers()
    {
        static const QSet<QString> layers{QStringLiteral("water"), QStringLiteral("waterway"), QStringLiteral("landcover"), QStringLiteral("landuse"), QStringLiteral("park"), QStringLiteral("building"), QStringLiteral("boundary"), QStringLiteral("transportation"), QStringLiteral("transportation_name"), QStringLiteral("place"), QStringLiteral("water_name")};
        return layers;
    }

    bool readPacked(ProtoReader& reader, QList<quint32>& values)
    {
        if (reader.wireType() == kWireVarint)
        {
            values.append(quint32(reader.varint()));
            return !reader.hasError();
        }
        if (reader.wireType() != kWireBytes)
        {
            reader.skip();
            return !reader.hasError();
        }
        ProtoReader packed(reader.bytes());
        if (reader.hasError())
        {
            return false;
        }
        while (!packed.atEnd())
        {
            values.append(quint32(packed.varint()));
            if (packed.hasError())
            {
                return false;
            }
        }
        return true;
    }

    QVariant decodeValue(QByteArrayView data)
    {
        ProtoReader reader(data);
        QVariant value;
        while (reader.next())
        {
            if (reader.field() == 1 && reader.wireType() == kWireBytes)
            {
                value = QString::fromUtf8(reader.bytes());
            }
            else if (reader.field() == 2 && reader.wireType() == kWireFixed32)
            {
                const quint32 bits = reader.fixed32();
                float number = 0.0F;
                std::memcpy(&number, &bits, sizeof(number));
                value = double(number);
            }
            else if (reader.field() == 3 && reader.wireType() == kWireFixed64)
            {
                const quint64 bits = reader.fixed64();
                double number = 0.0;
                std::memcpy(&number, &bits, sizeof(number));
                value = number;
            }
            else if (reader.field() == 4 && reader.wireType() == kWireVarint)
            {
                value = qint64(reader.varint());
            }
            else if (reader.field() == 5 && reader.wireType() == kWireVarint)
            {
                value = quint64(reader.varint());
            }
            else if (reader.field() == 6 && reader.wireType() == kWireVarint)
            {
                value = zigzag(reader.varint());
            }
            else if (reader.field() == 7 && reader.wireType() == kWireVarint)
            {
                value = reader.varint() != 0;
            }
            else
            {
                reader.skip();
            }
        }
        return value;
    }

    QList<QPolygonF> decodeGeometry(const QList<quint32>& commands, ShadMapVectorTile::GeometryType type)
    {
        QList<QPolygonF> parts;
        QPolygonF current;
        qint64 x = 0;
        qint64 y = 0;
        qsizetype index = 0;
        while (index < commands.size())
        {
            const quint32 command = commands.at(index++);
            const quint32 id = command & 0x7;
            const quint32 count = command >> 3;
            if (id == kCommandClosePath)
            {
                if (!current.isEmpty())
                {
                    current.append(current.first());
                }
                continue;
            }
            if (id != kCommandMoveTo && id != kCommandLineTo)
            {
                break;
            }
            for (quint32 step = 0; step < count; ++step)
            {
                if (commands.size() - index < 2)
                {
                    index = commands.size();
                    break;
                }
                x += zigzag(commands.at(index++));
                y += zigzag(commands.at(index++));
                if (id == kCommandMoveTo && type != ShadMapVectorTile::Point && !current.isEmpty())
                {
                    parts.append(current);
                    current.clear();
                }
                current.append(QPointF(double(x), double(y)));
            }
        }
        if (!current.isEmpty())
        {
            parts.append(current);
        }
        return parts;
    }

    bool decodeFeature(QByteArrayView data, ShadMapVectorTile::Feature& feature)
    {
        ProtoReader reader(data);
        QList<quint32> geometry;
        while (reader.next())
        {
            if (reader.field() == 2)
            {
                if (!readPacked(reader, feature.tags))
                {
                    return false;
                }
            }
            else if (reader.field() == 3 && reader.wireType() == kWireVarint)
            {
                const quint64 type = reader.varint();
                feature.type = type <= ShadMapVectorTile::Polygon ? ShadMapVectorTile::GeometryType(type) : ShadMapVectorTile::Unknown;
            }
            else if (reader.field() == 4)
            {
                if (!readPacked(reader, geometry))
                {
                    return false;
                }
            }
            else
            {
                reader.skip();
            }
        }
        if (reader.hasError())
        {
            return false;
        }
        feature.parts = decodeGeometry(geometry, feature.type);
        return true;
    }

    bool decodeLayer(QByteArrayView data, ShadMapVectorTile::Layer& layer)
    {
        ProtoReader reader(data);
        QList<QByteArrayView> featureData;
        while (reader.next())
        {
            if (reader.field() == 1 && reader.wireType() == kWireBytes)
            {
                layer.name = QString::fromUtf8(reader.bytes());
            }
            else if (reader.field() == 2 && reader.wireType() == kWireBytes)
            {
                featureData.append(reader.bytes());
            }
            else if (reader.field() == 3 && reader.wireType() == kWireBytes)
            {
                layer.keys.append(QString::fromUtf8(reader.bytes()));
            }
            else if (reader.field() == 4 && reader.wireType() == kWireBytes)
            {
                layer.values.append(decodeValue(reader.bytes()));
            }
            else if (reader.field() == 5 && reader.wireType() == kWireVarint)
            {
                const quint64 extent = reader.varint();
                layer.extent = extent > 0 ? double(extent) : 4096.0;
            }
            else
            {
                reader.skip();
            }
        }
        if (reader.hasError())
        {
            return false;
        }
        if (!wantedLayers().contains(layer.name))
        {
            return true;
        }
        layer.features.reserve(featureData.size());
        for (const QByteArrayView& bytes : std::as_const(featureData))
        {
            ShadMapVectorTile::Feature feature;
            if (!decodeFeature(bytes, feature))
            {
                return false;
            }
            if (feature.type != ShadMapVectorTile::Unknown && !feature.parts.isEmpty())
            {
                layer.features.append(feature);
            }
        }
        return true;
    }
} // namespace

QVariant ShadMapVectorTile::Layer::property(const Feature& feature, const QString& key) const
{
    for (qsizetype index = 0; index + 1 < feature.tags.size(); index += 2)
    {
        const quint32 keyIndex = feature.tags.at(index);
        const quint32 valueIndex = feature.tags.at(index + 1);
        if (keyIndex < quint32(keys.size()) && valueIndex < quint32(values.size()) && keys.at(keyIndex) == key)
        {
            return values.at(valueIndex);
        }
    }
    return QVariant();
}

QSharedPointer<const ShadMapVectorTile> ShadMapVectorTile::decode(const QByteArray& data)
{
    if (isCompressed(data))
    {
        return QSharedPointer<const ShadMapVectorTile>();
    }
    QSharedPointer<ShadMapVectorTile> tile(new ShadMapVectorTile());
    ProtoReader reader{QByteArrayView(data)};
    while (reader.next())
    {
        if (reader.field() != 3 || reader.wireType() != kWireBytes)
        {
            reader.skip();
            continue;
        }
        Layer layer;
        if (!decodeLayer(reader.bytes(), layer))
        {
            return QSharedPointer<const ShadMapVectorTile>();
        }
        if (layer.features.isEmpty())
        {
            continue;
        }
        for (const Feature& feature : std::as_const(layer.features))
        {
            tile->_byteCost += qsizetype(sizeof(Feature)) + feature.tags.size() * qsizetype(sizeof(quint32));
            for (const QPolygonF& part : feature.parts)
            {
                tile->_byteCost += part.size() * qsizetype(sizeof(QPointF));
            }
        }
        tile->_layers.insert(layer.name, layer);
    }
    if (reader.hasError())
    {
        return QSharedPointer<const ShadMapVectorTile>();
    }
    return tile;
}

bool ShadMapVectorTile::isCompressed(const QByteArray& data)
{
    return data.size() >= 2 && quint8(data.at(0)) == 0x1f && quint8(data.at(1)) == 0x8b;
}

const ShadMapVectorTile::Layer* ShadMapVectorTile::layer(const QString& name) const
{
    const auto found = _layers.constFind(name);
    return found != _layers.constEnd() ? &found.value() : nullptr;
}

qsizetype ShadMapVectorTile::byteCost() const
{
    return _byteCost;
}
