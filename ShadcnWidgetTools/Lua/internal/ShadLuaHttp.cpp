#include "ShadLuaHttp.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "ShadLuaEngine.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaInclude.h"

ShadLuaHttp::ShadLuaHttp(ShadLuaEngine* engine)
    : QObject(nullptr), _engine(engine)
{
    _manager = new QNetworkAccessManager(this);
}

ShadLuaHttp::~ShadLuaHttp()
{
}

void ShadLuaHttp::request(const ShadLuaHttpRequest& request, int callbackReference)
{
    QNetworkRequest networkRequest(request.url);
    networkRequest.setTransferTimeout(request.timeoutMs);
    for (const std::pair<QByteArray, QByteArray>& header : request.headers)
    {
        networkRequest.setRawHeader(header.first, header.second);
    }
    QNetworkReply* reply = _manager->sendCustomRequest(networkRequest, request.method, request.body);
    Pending pending;
    pending.callbackReference = callbackReference;
    pending.scriptId = ShadLuaEnginePrivate::get(_engine)->_currentScript;
    _pending.insert(reply, pending);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { finish(reply); });
}

void ShadLuaHttp::cancelScript(int scriptId)
{
    lua_State* L = ShadLuaEnginePrivate::get(_engine)->_state;
    for (auto it = _pending.begin(); it != _pending.end();)
    {
        if (it.value().scriptId != scriptId)
        {
            ++it;
            continue;
        }
        QNetworkReply* reply = it.key();
        luaL_unref(L, LUA_REGISTRYINDEX, it.value().callbackReference);
        it = _pending.erase(it);
        // 先断开再中止，避免 abort 发出的 finished 进入 finish
        QObject::disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
    }
}

void ShadLuaHttp::finish(QNetworkReply* reply)
{
    reply->deleteLater();
    const auto it = _pending.constFind(reply);
    if (it == _pending.constEnd())
    {
        return;
    }
    const Pending pending = it.value();
    _pending.erase(it);

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool hasError = reply->error() != QNetworkReply::NoError;
    QVariantMap headers;
    const QList<QNetworkReply::RawHeaderPair> headerPairs = reply->rawHeaderPairs();
    for (const QNetworkReply::RawHeaderPair& header : headerPairs)
    {
        headers.insert(QString::fromUtf8(header.first), QString::fromUtf8(header.second));
    }
    QVariantMap response;
    response.insert(QStringLiteral("ok"), !hasError && status >= 200 && status < 300);
    response.insert(QStringLiteral("status"), status);
    response.insert(QStringLiteral("url"), reply->url().toString());
    response.insert(QStringLiteral("headers"), headers);
    response.insert(QStringLiteral("body"), reply->readAll());
    if (hasError)
    {
        response.insert(QStringLiteral("error"), reply->errorString());
    }

    // 回调期间的对象与信号记在发起请求的脚本名下
    ShadLuaEnginePrivate* engine = ShadLuaEnginePrivate::get(_engine);
    const int previousScript = engine->_currentScript;
    engine->_currentScript = pending.scriptId;
    engine->callReference(pending.callbackReference, QVariantList{response});
    engine->_currentScript = previousScript;
    luaL_unref(engine->_state, LUA_REGISTRYINDEX, pending.callbackReference);
}
