#ifndef AICHATPAGE_H
#define AICHATPAGE_H

#include <QImage>
#include <QList>
#include <QString>

#include <array>

#include "AiChatClient.h"
#include "ExampleBasePage.h"
#include "ShadDef.h"
#include "ShadIconType.h"

class QEvent;
class QObject;
class QVBoxLayout;
class QWidget;
class ShadAttachment;
class ShadAttachmentGroup;
class ShadButton;
class ShadComboBox;
class ShadImageViewer;
class ShadInput;
class ShadLabel;
class ShadMarkdownView;
class ShadMarker;
class ShadScrollArea;
class ShadTextarea;

class AiChatPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AiChatPage(QWidget* parent = nullptr);
        ~AiChatPage() override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        struct ProviderState
        {
            QString customUrl;
            QString model;
            QString apiKey;
            QString effort{QStringLiteral("medium")};
            bool isOfficial{true};
        };

        struct PendingImage
        {
            AiChatClient::Image image;
            QImage source;
            QImage preview;
            ShadAttachment* attachment{nullptr};
        };

        AiChatClient::Provider currentProvider() const;
        void loadProviderState();
        void fillEffortItems();
        QString effortLabel(const QString& level) const;
        void updateEndpointInput();
        void updateControls();
        void sendMessage();
        void stopReply();
        void clearConversation();
        void appendWidget(QWidget* widget);
        void appendUserMessage(const QString& text, const QList<QImage>& images);
        void showImages(const QList<QImage>& images, int index);
        void chooseImages();
        void addPendingImage(const QImage& image, const QString& name);
        void removePendingImage(ShadAttachment* attachment);
        void clearPendingImages();
        void appendError(const QString& message);
        void appendNotice(ShadIconType::IconName icon, const QString& text, ShadMarkerType::Variant variant);
        void removePendingMarker();
        void finishReply();

        AiChatClient* _client{nullptr};
        ShadComboBox* _providerComboBox{nullptr};
        ShadComboBox* _endpointComboBox{nullptr};
        ShadInput* _urlInput{nullptr};
        ShadInput* _modelInput{nullptr};
        ShadInput* _keyInput{nullptr};
        ShadComboBox* _effortComboBox{nullptr};
        ShadScrollArea* _messageArea{nullptr};
        QWidget* _messageContainer{nullptr};
        QVBoxLayout* _messageColumn{nullptr};
        ShadLabel* _emptyLabel{nullptr};
        ShadTextarea* _promptInput{nullptr};
        ShadAttachmentGroup* _attachmentGroup{nullptr};
        ShadImageViewer* _imageViewer{nullptr};
        ShadButton* _attachButton{nullptr};
        ShadButton* _sendButton{nullptr};
        ShadButton* _stopButton{nullptr};
        ShadButton* _clearButton{nullptr};
        ShadMarker* _pendingMarker{nullptr};
        ShadMarkdownView* _replyView{nullptr};
        QString _replyText;
        bool _isRefused{false};
        bool _isFollowing{true};
        QList<AiChatClient::Message> _history;
        QList<PendingImage> _pendingImages;
        std::array<ProviderState, 3> _providerStates;
        int _providerIndex{0};
};

#endif // AICHATPAGE_H
