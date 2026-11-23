#include "ShadMapTileLoader.h"

#include <QByteArray>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QVariant>

#include <utility>

#include "ShadMapVectorTile.h"

namespace
{
    constexpr int kMaxCachedTiles = 512;
    constexpr int kVectorCacheKilobytes = 64 * 1024;
    constexpr int kMaxConcurrentRequests = 6;
    constexpr int kTransferTimeoutMs = 15000;
    constexpr qint64 kRetryDelayMs = 10000;
    constexpr qint64 kDiskCacheBytes = 100 * 1024 * 1024;
    constexpr int kRasterMaximumZoom = 22;
    constexpr int kVectorMaximumZoom = 14;
    constexpr qint64 kFetchLimitBytes = 10 * 1024 * 1024;
    constexpr char kOversizeProperty[] = "_shadMapOversize";

    bool isHttpUrl(const QUrl& url)
    {
        return url.isValid() && (url.scheme() == QStringLiteral("https") || url.scheme() == QStringLiteral("http"));
    }

    QNetworkRequest createRequest(const QUrl& url)
    {
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ShadcnWidgetTools/" SHADCN_MAP_VERSION));
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        request.setTransferTimeout(kTransferTimeoutMs);
        return request;
    }
} // namespace

ShadMapTileLoader::ShadMapTileLoader(QObject* parent)
    : QObject(parent), _cache(kMaxCachedTiles), _vectorCache(kVectorCacheKilobytes)
{
    _manager = new QNetworkAccessManager(this);
    const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cacheRoot.isEmpty())
    {
        QNetworkDiskCache* diskCache = new QNetworkDiskCache(_manager);
        diskCache->setCacheDirectory(QDir(cacheRoot).filePath(QStringLiteral("ShadcnWidgetToolsMap")));
        diskCache->setMaximumCacheSize(kDiskCacheBytes);
        _manager->setCache(diskCache);
    }
    _clock.start();
}

ShadMapTileLoader::~ShadMapTileLoader()
{
    abortTileJson();
    reset();
    const QSet<QNetworkReply*> fetches = _fetches;
    _fetches.clear();
    for (QNetworkReply* reply : fetches)
    {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}

void ShadMapTileLoader::setFormat(ShadMapType::TileFormat format)
{
    if (_format == format)
    {
        return;
    }
    _format = format;
    reset();
}

ShadMapType::TileFormat ShadMapTileLoader::format() const
{
    return _format;
}

void ShadMapTileLoader::setUrlTemplate(const QString& urlTemplate)
{
    if (_urlTemplate == urlTemplate)
    {
        return;
    }
    _urlTemplate = urlTemplate;
    abortTileJson();
    _resolvedTemplate.clear();
    _tileJsonMinimumZoom = -1;
    _tileJsonMaximumZoom = -1;
    _tileJsonFailedAt = -1;
    reset();
    if (urlTemplate.contains(QStringLiteral("{z}")))
    {
        _resolvedTemplate = urlTemplate;
    }
    else if (!urlTemplate.isEmpty())
    {
        requestTileJson();
    }
    Q_EMIT sourceChanged();
}

QString ShadMapTileLoader::urlTemplate() const
{
    return _urlTemplate;
}

void ShadMapTileLoader::setSubdomains(const QStringList& subdomains)
{
    _subdomains = subdomains;
}

QStringList ShadMapTileLoader::subdomains() const
{
    return _subdomains;
}

void ShadMapTileLoader::setHighDpi(bool highDpi)
{
    if (_isHighDpi == highDpi)
    {
        return;
    }
    _isHighDpi = highDpi;
    if (_resolvedTemplate.contains(QStringLiteral("{r}")))
    {
        reset();
    }
}

bool ShadMapTileLoader::isSourceReady() const
{
    return !_resolvedTemplate.isEmpty();
}

void ShadMapTileLoader::ensureSource()
{
    if (_resolvedTemplate.isEmpty())
    {
        requestTileJson();
    }
}

int ShadMapTileLoader::sourceMinimumZoom() const
{
    return qMax(0, _tileJsonMinimumZoom);
}

int ShadMapTileLoader::sourceMaximumZoom() const
{
    if (_tileJsonMaximumZoom >= 0)
    {
        return qMax(sourceMinimumZoom(), _tileJsonMaximumZoom);
    }
    return _format == ShadMapType::Vector ? kVectorMaximumZoom : kRasterMaximumZoom;
}

quint64 ShadMapTileLoader::tileKey(int zoom, int x, int y)
{
    return (quint64(zoom) << 58) | (quint64(x) << 29) | quint64(y);
}

QPixmap ShadMapTileLoader::tile(int zoom, int x, int y)
{
    const quint64 key = tileKey(zoom, x, y);
    if (const QPixmap* cached = _cache.object(key))
    {
        return *cached;
    }
    request(zoom, x, y);
    return QPixmap();
}

QPixmap ShadMapTileLoader::cachedTile(int zoom, int x, int y) const
{
    const QPixmap* cached = _cache.object(tileKey(zoom, x, y));
    return cached != nullptr ? *cached : QPixmap();
}

QSharedPointer<const ShadMapVectorTile> ShadMapTileLoader::vectorTile(int zoom, int x, int y)
{
    const quint64 key = tileKey(zoom, x, y);
    if (const QSharedPointer<const ShadMapVectorTile>* cached = _vectorCache.object(key))
    {
        return *cached;
    }
    request(zoom, x, y);
    return QSharedPointer<const ShadMapVectorTile>();
}

QSharedPointer<const ShadMapVectorTile> ShadMapTileLoader::cachedVectorTile(int zoom, int x, int y) const
{
    const QSharedPointer<const ShadMapVectorTile>* cached = _vectorCache.object(tileKey(zoom, x, y));
    return cached != nullptr ? *cached : QSharedPointer<const ShadMapVectorTile>();
}

void ShadMapTileLoader::fetch(const QUrl& url)
{
    if (!isHttpUrl(url))
    {
        Q_EMIT fetched(url, QByteArray(), tr("只支持 http 与 https 地址"));
        return;
    }
    QNetworkReply* reply = _manager->get(createRequest(url));
    _fetches.insert(reply);
    connect(reply, &QNetworkReply::downloadProgress, this, [reply](qint64 received)
    {
        if (received > kFetchLimitBytes)
        {
            reply->setProperty(kOversizeProperty, true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, url]()
    {
        reply->deleteLater();
        _fetches.remove(reply);
        if (reply->property(kOversizeProperty).toBool())
        {
            Q_EMIT fetched(url, QByteArray(), tr("文件超过 10 MB"));
        }
        else if (reply->error() != QNetworkReply::NoError)
        {
            Q_EMIT fetched(url, QByteArray(), reply->errorString());
        }
        else
        {
            Q_EMIT fetched(url, reply->readAll(), QString());
        }
    });
}

void ShadMapTileLoader::retain(const QSet<quint64>& keys)
{
    for (int index = int(_queue.size()) - 1; index >= 0; --index)
    {
        if (!keys.contains(_queue.at(index).key))
        {
            _queue.removeAt(index);
        }
    }
    const QList<quint64> activeKeys = _active.keys();
    for (const quint64 key : activeKeys)
    {
        QNetworkReply* reply = _active.value(key);
        if (reply != nullptr && !keys.contains(key))
        {
            reply->abort();
        }
    }
}

bool ShadMapTileLoader::isCached(quint64 key) const
{
    return _format == ShadMapType::Vector ? _vectorCache.contains(key) : _cache.contains(key);
}

void ShadMapTileLoader::request(int zoom, int x, int y)
{
    if (_resolvedTemplate.isEmpty())
    {
        requestTileJson();
        return;
    }
    const quint64 key = tileKey(zoom, x, y);
    if (isCached(key) || _active.contains(key))
    {
        return;
    }
    const auto failed = _failedAt.constFind(key);
    if (failed != _failedAt.constEnd() && _clock.elapsed() - failed.value() < kRetryDelayMs)
    {
        return;
    }
    for (const Request& queued : std::as_const(_queue))
    {
        if (queued.key == key)
        {
            return;
        }
    }
    _queue.append(Request{key, zoom, x, y});
    startNext();
}

QUrl ShadMapTileLoader::tileUrl(int zoom, int x, int y) const
{
    QString text = _resolvedTemplate;
    text.replace(QStringLiteral("{z}"), QString::number(zoom));
    text.replace(QStringLiteral("{x}"), QString::number(x));
    text.replace(QStringLiteral("{y}"), QString::number(y));
    text.replace(QStringLiteral("{r}"), _isHighDpi ? QStringLiteral("@2x") : QString());
    if (!_subdomains.isEmpty())
    {
        text.replace(QStringLiteral("{s}"), _subdomains.at((x + y) % int(_subdomains.size())));
    }
    const QUrl url(text);
    return isHttpUrl(url) ? url : QUrl();
}

void ShadMapTileLoader::reset()
{
    _queue.clear();
    const QList<QNetworkReply*> replies = _active.values();
    _active.clear();
    for (QNetworkReply* reply : replies)
    {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    _cache.clear();
    _vectorCache.clear();
    _failedAt.clear();
}

void ShadMapTileLoader::abortTileJson()
{
    if (_tileJsonReply.isNull())
    {
        return;
    }
    QNetworkReply* reply = _tileJsonReply.data();
    _tileJsonReply = nullptr;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

void ShadMapTileLoader::requestTileJson()
{
    if (_urlTemplate.isEmpty() || !_tileJsonReply.isNull())
    {
        return;
    }
    if (_tileJsonFailedAt >= 0 && _clock.elapsed() - _tileJsonFailedAt < kRetryDelayMs)
    {
        return;
    }
    const QUrl url(_urlTemplate);
    if (!isHttpUrl(url))
    {
        _tileJsonFailedAt = _clock.elapsed();
        return;
    }
    QNetworkReply* reply = _manager->get(createRequest(url));
    _tileJsonReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleTileJson(reply); });
}

void ShadMapTileLoader::handleTileJson(QNetworkReply* reply)
{
    reply->deleteLater();
    if (_tileJsonReply != reply)
    {
        return;
    }
    _tileJsonReply = nullptr;
    QString resolved;
    if (reply->error() == QNetworkReply::NoError)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
        const QJsonObject object = document.object();
        const QJsonArray tiles = object.value(QStringLiteral("tiles")).toArray();
        if (parseError.error == QJsonParseError::NoError && !tiles.isEmpty())
        {
            resolved = tiles.first().toString();
            _tileJsonMinimumZoom = object.value(QStringLiteral("minzoom")).toInt(-1);
            _tileJsonMaximumZoom = object.value(QStringLiteral("maxzoom")).toInt(-1);
        }
    }
    if (!resolved.contains(QStringLiteral("{z}")))
    {
        qWarning("地图 TileJSON 加载失败或缺少瓦片地址：%s", qUtf8Printable(_urlTemplate));
        _tileJsonFailedAt = _clock.elapsed();
        return;
    }
    _resolvedTemplate = resolved;
    Q_EMIT sourceChanged();
}

void ShadMapTileLoader::startNext()
{
    while (_active.size() < kMaxConcurrentRequests && !_queue.isEmpty())
    {
        const Request next = _queue.takeFirst();
        const QUrl url = tileUrl(next.zoom, next.x, next.y);
        if (url.isEmpty())
        {
            _failedAt.insert(next.key, _clock.elapsed());
            continue;
        }
        QNetworkReply* reply = _manager->get(createRequest(url));
        const quint64 key = next.key;
        _active.insert(key, reply);
        connect(reply, &QNetworkReply::finished, this, [this, reply, key]() { handleFinished(reply, key); });
    }
}

void ShadMapTileLoader::handleFinished(QNetworkReply* reply, quint64 key)
{
    reply->deleteLater();
    if (_active.value(key) != reply)
    {
        return;
    }
    _active.remove(key);
    bool isLoaded = false;
    if (reply->error() == QNetworkReply::NoError)
    {
        const QByteArray data = reply->readAll();
        if (_format == ShadMapType::Vector)
        {
            const QSharedPointer<const ShadMapVectorTile> vector = ShadMapVectorTile::decode(data);
            if (!vector.isNull())
            {
                _vectorCache.insert(key, new QSharedPointer<const ShadMapVectorTile>(vector), int(qMax<qsizetype>(1, vector->byteCost() / 1024)));
                isLoaded = true;
            }
            else if (ShadMapVectorTile::isCompressed(data) && !_hasWarnedCompressed)
            {
                _hasWarnedCompressed = true;
                qWarning("矢量瓦片内容为 gzip 压缩数据，当前只支持通过 HTTP Content-Encoding 压缩的瓦片：%s", qUtf8Printable(reply->url().toString()));
            }
        }
        else
        {
            QPixmap pixmap;
            if (pixmap.loadFromData(data))
            {
                _cache.insert(key, new QPixmap(pixmap));
                isLoaded = true;
            }
        }
    }
    if (!isLoaded && reply->error() != QNetworkReply::OperationCanceledError)
    {
        _failedAt.insert(key, _clock.elapsed());
    }
    startNext();
    if (isLoaded)
    {
        Q_EMIT tileLoaded();
    }
}
