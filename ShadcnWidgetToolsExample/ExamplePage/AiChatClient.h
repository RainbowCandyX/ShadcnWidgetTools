#ifndef AICHATCLIENT_H
#define AICHATCLIENT_H

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

class QImage;
class QNetworkAccessManager;
class QNetworkReply;

class AiChatClient : public QObject
{
    Q_OBJECT

    public:
        enum class Provider
        {
            OpenAiCompatible = 0,
            Claude,
            DeepSeek,
        };

        struct Image
        {
            QString mediaType;
            QByteArray data;
        };

        struct Message
        {
            QString role;
            QString content;
            QList<Image> images;
        };

        struct Config
        {
            Provider provider{Provider::OpenAiCompatible};
            QString baseUrl;
            QString apiKey;
            QString model;
            QString effort;
            bool isOfficial{true};
        };

        explicit AiChatClient(QObject* parent = nullptr);
        ~AiChatClient() override;

        static QString officialBaseUrl(Provider provider);
        static QString defaultModel(Provider provider);
        static QStringList effortLevels();
        static Image encodeImage(const QImage& image);

        void send(const Config& config, const QList<Message>& messages);
        void abort();
        bool isRunning() const;

    Q_SIGNALS:
        void textReceived(const QString& text);
        void refused();
        void finished();
        void failed(const QString& message);

    private:
        void handleReadyRead();
        void handleFinished();
        void consumeLines(bool isFinal);
        void handlePayload(const QByteArray& payload);
        QString errorFromBody(const QByteArray& body) const;
        int httpStatus() const;

        QNetworkAccessManager* _manager{nullptr};
        QPointer<QNetworkReply> _reply;
        Provider _provider{Provider::OpenAiCompatible};
        QByteArray _buffer;
        QString _streamError;
};

#endif // AICHATCLIENT_H
