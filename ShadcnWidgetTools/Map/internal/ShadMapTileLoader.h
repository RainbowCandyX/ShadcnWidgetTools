#ifndef SHADMAPTILELOADER_H
#define SHADMAPTILELOADER_H

#include <QByteArray>
#include <QCache>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

#include "ShadMapDef.h"

class QNetworkAccessManager;
class QNetworkReply;
class ShadMapVectorTile;

class ShadMapTileLoader : public QObject
{
    Q_OBJECT

    public:
        explicit ShadMapTileLoader(QObject* parent = nullptr);
        ~ShadMapTileLoader() override;

        void setFormat(ShadMapType::TileFormat format);
        ShadMapType::TileFormat format() const;

        void setUrlTemplate(const QString& urlTemplate);
        QString urlTemplate() const;

        void setSubdomains(const QStringList& subdomains);
        QStringList subdomains() const;

        void setHighDpi(bool highDpi);

        bool isSourceReady() const;
        void ensureSource();
        int sourceMinimumZoom() const;
        int sourceMaximumZoom() const;

        static quint64 tileKey(int zoom, int x, int y);

        QPixmap tile(int zoom, int x, int y);
        QPixmap cachedTile(int zoom, int x, int y) const;
        QSharedPointer<const ShadMapVectorTile> vectorTile(int zoom, int x, int y);
        QSharedPointer<const ShadMapVectorTile> cachedVectorTile(int zoom, int x, int y) const;
        void retain(const QSet<quint64>& keys);
        void fetch(const QUrl& url);

    Q_SIGNALS:
        void tileLoaded();
        void sourceChanged();
        void fetched(const QUrl& url, const QByteArray& data, const QString& error);

    private:
        struct Request
        {
            quint64 key;
            int zoom;
            int x;
            int y;
        };

        bool isCached(quint64 key) const;
        void request(int zoom, int x, int y);
        QUrl tileUrl(int zoom, int x, int y) const;
        void reset();
        void abortTileJson();
        void requestTileJson();
        void handleTileJson(QNetworkReply* reply);
        void startNext();
        void handleFinished(QNetworkReply* reply, quint64 key);

        QNetworkAccessManager* _manager{nullptr};
        ShadMapType::TileFormat _format{ShadMapType::Raster};
        QString _urlTemplate;
        QString _resolvedTemplate;
        QStringList _subdomains;
        int _tileJsonMinimumZoom{-1};
        int _tileJsonMaximumZoom{-1};
        QPointer<QNetworkReply> _tileJsonReply;
        qint64 _tileJsonFailedAt{-1};
        bool _isHighDpi{false};
        bool _hasWarnedCompressed{false};
        QCache<quint64, QPixmap> _cache;
        QCache<quint64, QSharedPointer<const ShadMapVectorTile>> _vectorCache;
        QList<Request> _queue;
        QHash<quint64, QNetworkReply*> _active;
        QHash<quint64, qint64> _failedAt;
        QSet<QNetworkReply*> _fetches;
        QElapsedTimer _clock;
};

#endif // SHADMAPTILELOADER_H
