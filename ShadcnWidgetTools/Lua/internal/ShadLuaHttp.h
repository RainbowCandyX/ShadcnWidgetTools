#ifndef SHADLUAHTTP_H
#define SHADLUAHTTP_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QUrl>

#include <utility>

class QNetworkAccessManager;
class QNetworkReply;
class ShadLuaEngine;

struct ShadLuaHttpRequest
{
    QUrl url;
    QByteArray method;
    QList<std::pair<QByteArray, QByteArray>> headers;
    QByteArray body;
    int timeoutMs{0};
};

// shad.http 的实现，每个 ShadLuaEngine 一个，不对外导出
class ShadLuaHttp : public QObject
{
    public:
        explicit ShadLuaHttp(ShadLuaEngine* engine);
        ~ShadLuaHttp() override;

        // callbackReference 在请求结束后调用一次并释放
        void request(const ShadLuaHttpRequest& request, int callbackReference);
        // 中止后回调不再调用
        void cancelScript(int scriptId);

    private:
        struct Pending
        {
            int callbackReference{0};
            int scriptId{0};
        };

        void finish(QNetworkReply* reply);

        ShadLuaEngine* _engine{nullptr};
        QNetworkAccessManager* _manager{nullptr};
        QHash<QNetworkReply*, Pending> _pending;
};

#endif // SHADLUAHTTP_H
