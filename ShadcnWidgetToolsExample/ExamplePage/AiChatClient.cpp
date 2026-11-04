#include "AiChatClient.h"

#include <QBuffer>
#include <QIODevice>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLatin1Char>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

namespace
{
    constexpr int kTransferTimeoutMs = 60000;
    constexpr int kClaudeMaxTokens = 64000;
    constexpr int kFirstErrorStatus = 400;
    constexpr int kMaxErrorLength = 300;
    constexpr int kMaxImageEdge = 1568;
    constexpr int kJpegQuality = 90;
    const char* const kClaudeVersion = "2026-10-21";
    const char* const kClaudeFallbackBeta = "server-side-fallback-2026-07-01";

    QString trimmedBaseUrl(const QString& baseUrl)
    {
        QString url = baseUrl.trimmed();
        while (url.endsWith(QLatin1Char('/')))
        {
            url.chop(1);
        }
        return url;
    }

    QList<AiChatClient::Message> mergedMessages(const QList<AiChatClient::Message>& messages)
    {
        QList<AiChatClient::Message> result;
        for (const AiChatClient::Message& message : messages)
        {
            if (message.content.isEmpty() && message.images.isEmpty())
            {
                continue;
            }
            if (!result.isEmpty() && result.last().role == message.role)
            {
                AiChatClient::Message& previous = result.last();
                if (!previous.content.isEmpty() && !message.content.isEmpty())
                {
                    previous.content += QStringLiteral("\n\n");
                }
                previous.content += message.content;
                previous.images += message.images;
                continue;
            }
            result.append(message);
        }
        return result;
    }

    QJsonValue messageContent(const AiChatClient::Message& message, AiChatClient::Provider provider)
    {
        if (message.images.isEmpty())
        {
            return message.content;
        }
        QJsonObject textBlock;
        textBlock.insert(QStringLiteral("type"), QStringLiteral("text"));
        textBlock.insert(QStringLiteral("text"), message.content);
        QJsonArray blocks;
        if (provider != AiChatClient::Provider::Claude && !message.content.isEmpty())
        {
            blocks.append(textBlock);
        }
        for (const AiChatClient::Image& image : message.images)
        {
            QJsonObject imageBlock;
            if (provider == AiChatClient::Provider::Claude)
            {
                QJsonObject source;
                source.insert(QStringLiteral("type"), QStringLiteral("base64"));
                source.insert(QStringLiteral("media_type"), image.mediaType);
                source.insert(QStringLiteral("data"), QString::fromLatin1(image.data));
                imageBlock.insert(QStringLiteral("type"), QStringLiteral("image"));
                imageBlock.insert(QStringLiteral("source"), source);
            }
            else
            {
                QJsonObject imageUrl;
                imageUrl.insert(QStringLiteral("url"), QStringLiteral("data:%1;base64,%2").arg(image.mediaType, QString::fromLatin1(image.data)));
                imageBlock.insert(QStringLiteral("type"), QStringLiteral("image_url"));
                imageBlock.insert(QStringLiteral("image_url"), imageUrl);
            }
            blocks.append(imageBlock);
        }
        if (provider == AiChatClient::Provider::Claude && !message.content.isEmpty())
        {
            blocks.append(textBlock);
        }
        return blocks;
    }

    QString errorMessage(const QJsonObject& object)
    {
        const QJsonValue error = object.value(QStringLiteral("error"));
        if (error.isObject())
        {
            return error.toObject().value(QStringLiteral("message")).toString();
        }
        return error.toString();
    }
}

AiChatClient::AiChatClient(QObject* parent)
    : QObject(parent), _manager(new QNetworkAccessManager(this))
{
}

AiChatClient::~AiChatClient()
{
    abort();
}

QString AiChatClient::officialBaseUrl(Provider provider)
{
    switch (provider)
    {
        case Provider::Claude:
            return QStringLiteral("https://api.anthropic.com");
        case Provider::DeepSeek:
            return QStringLiteral("https://api.deepseek.com");
        case Provider::OpenAiCompatible:
            break;
    }
    return QStringLiteral("https://api.openai.com/v1");
}

QString AiChatClient::defaultModel(Provider provider)
{
    switch (provider)
    {
        case Provider::Claude:
            return QStringLiteral("claude-opus-5-5");
        case Provider::DeepSeek:
            return QStringLiteral("deepseek-flash");
        case Provider::OpenAiCompatible:
            break;
    }
    return QStringLiteral("gpt-6.1-sol");
}

QStringList AiChatClient::effortLevels()
{
    return {QStringLiteral("low"), QStringLiteral("medium"), QStringLiteral("high"), QStringLiteral("xhigh"), QStringLiteral("max")};
}

AiChatClient::Image AiChatClient::encodeImage(const QImage& image)
{
    QImage scaled = image;
    if (qMax(image.width(), image.height()) > kMaxImageEdge)
    {
        scaled = image.scaled(kMaxImageEdge, kMaxImageEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    const bool hasAlpha = scaled.hasAlphaChannel();
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    scaled.save(&buffer, hasAlpha ? "PNG" : "JPEG", hasAlpha ? -1 : kJpegQuality);
    Image result;
    result.mediaType = hasAlpha ? QStringLiteral("image/png") : QStringLiteral("image/jpeg");
    result.data = bytes.toBase64();
    return result;
}

void AiChatClient::send(const Config& config, const QList<Message>& messages)
{
    abort();
    _provider = config.provider;
    _buffer.clear();
    _streamError.clear();
    const QString baseUrl = trimmedBaseUrl(config.baseUrl);
    const QString path = config.provider == Provider::Claude ? QStringLiteral("/v1/messages") : QStringLiteral("/chat/completions");
    const QUrl url(baseUrl + path);
    if (baseUrl.isEmpty() || !url.isValid() || (url.scheme() != QStringLiteral("https") && url.scheme() != QStringLiteral("http")))
    {
        Q_EMIT failed(tr("接口地址无效：%1").arg(config.baseUrl));
        return;
    }
    QJsonArray messageArray;
    for (const Message& message : mergedMessages(messages))
    {
        QJsonObject item;
        item.insert(QStringLiteral("role"), message.role);
        item.insert(QStringLiteral("content"), messageContent(message, config.provider));
        messageArray.append(item);
    }
    QJsonObject body;
    body.insert(QStringLiteral("model"), config.model);
    body.insert(QStringLiteral("stream"), true);
    body.insert(QStringLiteral("messages"), messageArray);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Accept", "text/event-stream");
    request.setTransferTimeout(kTransferTimeoutMs);
    if (config.provider == Provider::Claude)
    {
        request.setRawHeader("x-api-key", config.apiKey.toUtf8());
        request.setRawHeader("anthropic-version", kClaudeVersion);
        body.insert(QStringLiteral("max_tokens"), kClaudeMaxTokens);
        if (!config.effort.isEmpty())
        {
            QJsonObject outputConfig;
            outputConfig.insert(QStringLiteral("effort"), config.effort);
            body.insert(QStringLiteral("output_config"), outputConfig);
        }
        if (config.isOfficial)
        {
            request.setRawHeader("anthropic-beta", kClaudeFallbackBeta);
            body.insert(QStringLiteral("fallbacks"), QStringLiteral("default"));
        }
    }
    else
    {
        request.setRawHeader("Authorization", QByteArrayLiteral("Bearer ") + config.apiKey.toUtf8());
        if (!config.effort.isEmpty())
        {
            body.insert(QStringLiteral("reasoning_effort"), config.effort);
        }
        if (config.provider == Provider::DeepSeek)
        {
            QJsonObject thinking;
            thinking.insert(QStringLiteral("type"), config.effort.isEmpty() ? QStringLiteral("disabled") : QStringLiteral("enabled"));
            body.insert(QStringLiteral("thinking"), thinking);
        }
    }
    QNetworkReply* reply = _manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    _reply = reply;
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]()
    {
        if (reply == _reply)
        {
            handleReadyRead();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        if (reply == _reply)
        {
            handleFinished();
        }
        reply->deleteLater();
    });
}

void AiChatClient::abort()
{
    if (_reply.isNull())
    {
        return;
    }
    QNetworkReply* reply = _reply;
    _reply = nullptr;
    reply->abort();
}

bool AiChatClient::isRunning() const
{
    return !_reply.isNull();
}

void AiChatClient::handleReadyRead()
{
    _buffer += _reply->readAll();
    if (httpStatus() >= kFirstErrorStatus)
    {
        return;
    }
    consumeLines(false);
}

void AiChatClient::handleFinished()
{
    QNetworkReply* reply = _reply;
    _buffer += reply->readAll();
    const int status = httpStatus();
    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    _reply = nullptr;
    if (error == QNetworkReply::OperationCanceledError)
    {
        Q_EMIT failed(tr("请求超时：%1 秒内没有收到数据").arg(kTransferTimeoutMs / 1000));
        return;
    }
    if (status >= kFirstErrorStatus)
    {
        Q_EMIT failed(tr("HTTP %1：%2").arg(status).arg(errorFromBody(_buffer)));
        return;
    }
    if (error != QNetworkReply::NoError)
    {
        Q_EMIT failed(errorText);
        return;
    }
    consumeLines(true);
    if (!_streamError.isEmpty())
    {
        Q_EMIT failed(_streamError);
        return;
    }
    Q_EMIT finished();
}

void AiChatClient::consumeLines(bool isFinal)
{
    while (true)
    {
        const qsizetype end = _buffer.indexOf('\n');
        if (end < 0)
        {
            break;
        }
        QByteArray line = _buffer.left(end);
        _buffer.remove(0, end + 1);
        if (line.endsWith('\r'))
        {
            line.chop(1);
        }
        if (line.startsWith("data:"))
        {
            handlePayload(line.mid(5).trimmed());
        }
    }
    if (isFinal)
    {
        if (_buffer.startsWith("data:"))
        {
            handlePayload(_buffer.mid(5).trimmed());
        }
        _buffer.clear();
    }
}

void AiChatClient::handlePayload(const QByteArray& payload)
{
    if (payload.isEmpty() || payload == "[DONE]")
    {
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return;
    }
    const QJsonObject object = document.object();
    if (object.contains(QStringLiteral("error")))
    {
        _streamError = errorMessage(object);
        return;
    }
    if (_provider == Provider::Claude)
    {
        const QString type = object.value(QStringLiteral("type")).toString();
        const QJsonObject delta = object.value(QStringLiteral("delta")).toObject();
        if (type == QStringLiteral("content_block_delta") && delta.value(QStringLiteral("type")).toString() == QStringLiteral("text_delta"))
        {
            const QString text = delta.value(QStringLiteral("text")).toString();
            if (!text.isEmpty())
            {
                Q_EMIT textReceived(text);
            }
        }
        else if (type == QStringLiteral("message_delta") && delta.value(QStringLiteral("stop_reason")).toString() == QStringLiteral("refusal"))
        {
            Q_EMIT refused();
        }
        return;
    }
    const QJsonArray choices = object.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty())
    {
        return;
    }
    const QString text = choices.first().toObject().value(QStringLiteral("delta")).toObject().value(QStringLiteral("content")).toString();
    if (!text.isEmpty())
    {
        Q_EMIT textReceived(text);
    }
}

QString AiChatClient::errorFromBody(const QByteArray& body) const
{
    const QJsonDocument document = QJsonDocument::fromJson(body);
    if (document.isObject())
    {
        const QString message = errorMessage(document.object());
        if (!message.isEmpty())
        {
            return message;
        }
    }
    const QString text = QString::fromUtf8(body).trimmed();
    return text.isEmpty() ? tr("没有返回错误信息") : text.left(kMaxErrorLength);
}

int AiChatClient::httpStatus() const
{
    return _reply.isNull() ? 0 : _reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
}
