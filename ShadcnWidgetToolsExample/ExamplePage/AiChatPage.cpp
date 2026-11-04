#include "AiChatPage.h"

#include <QColor>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMimeData>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStringList>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <functional>
#include <initializer_list>
#include <utility>

#include "ExampleI18n.h"
#include "ShadAlert.h"
#include "ShadAttachment.h"
#include "ShadAttachmentGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadField.h"
#include "ShadImageViewer.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadMarkdownView.h"
#include "ShadMarker.h"
#include "ShadScrollArea.h"
#include "ShadTextarea.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kMessageAreaHeight = 440;
    constexpr int kControlWidth = 360;
    constexpr int kMessageSpacing = 16;
    constexpr int kBubbleMaxWidth = 520;
    constexpr int kBubblePaddingX = 12;
    constexpr int kBubblePaddingY = 8;
    constexpr int kPromptMaxHeight = 160;
    constexpr int kFollowThreshold = 4;
    constexpr int kOpenAiIndex = 0;
    constexpr const char* kApiKeyVariables[] = {"OPENAI_API_KEY", "ANTHROPIC_API_KEY", "DEEPSEEK_API_KEY"};
    constexpr int kOfficialIndex = 0;
    constexpr int kOffEffortIndex = 0;
    constexpr int kMaxImages = 5;
    constexpr int kPreviewEdge = 320;
    constexpr int kThumbnailEdge = 88;
    constexpr int kThumbnailSpacing = 6;

    bool isImageFile(const QUrl& url)
    {
        static const QStringList suffixes = {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"), QStringLiteral("gif"), QStringLiteral("webp"), QStringLiteral("bmp")};
        return url.isLocalFile() && suffixes.contains(QFileInfo(url.toLocalFile()).suffix().toLower());
    }

    bool hasImageFile(const QMimeData* source)
    {
        if (!source->hasUrls())
        {
            return false;
        }
        for (const QUrl& url : source->urls())
        {
            if (isImageFile(url))
            {
                return true;
            }
        }
        return false;
    }

    class PromptTextarea : public ShadTextarea
    {
        public:
            using ImageHandler = std::function<void(const QImage&, const QString&)>;

            explicit PromptTextarea(QWidget* parent)
                : ShadTextarea(parent)
            {
            }

            void setImageHandler(const ImageHandler& handler)
            {
                _imageHandler = handler;
            }

        protected:
            bool canInsertFromMimeData(const QMimeData* source) const override
            {
                return source->hasImage() || hasImageFile(source) || ShadTextarea::canInsertFromMimeData(source);
            }

            void insertFromMimeData(const QMimeData* source) override
            {
                if (_imageHandler && source->hasImage())
                {
                    _imageHandler(qvariant_cast<QImage>(source->imageData()), QString());
                    return;
                }
                if (_imageHandler && hasImageFile(source))
                {
                    for (const QUrl& url : source->urls())
                    {
                        if (isImageFile(url))
                        {
                            _imageHandler(QImage(url.toLocalFile()), QFileInfo(url.toLocalFile()).fileName());
                        }
                    }
                    return;
                }
                ShadTextarea::insertFromMimeData(source);
            }

        private:
            ImageHandler _imageHandler;
    };

    class UserBubble : public QWidget
    {
        public:
            using ImageClickHandler = std::function<void(int)>;

            UserBubble(const QString& text, const QList<QImage>& images, const ImageClickHandler& onImageClicked, QWidget* parent)
                : QWidget(parent), _onImageClicked(onImageClicked)
            {
                QVBoxLayout* layout = new QVBoxLayout(this);
                layout->setContentsMargins(kBubblePaddingX, kBubblePaddingY, kBubblePaddingX, kBubblePaddingY);
                layout->setSpacing(kBubblePaddingY);
                if (!images.isEmpty())
                {
                    QHBoxLayout* imageRow = new QHBoxLayout();
                    imageRow->setSpacing(kThumbnailSpacing);
                    const qreal ratio = qGuiApp->devicePixelRatio();
                    const int edge = qRound(kThumbnailEdge * ratio);
                    for (const QImage& image : images)
                    {
                        QPixmap pixmap = QPixmap::fromImage(image.scaled(edge, edge, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        pixmap.setDevicePixelRatio(ratio);
                        QLabel* thumbnail = new QLabel(this);
                        thumbnail->setPixmap(pixmap);
                        thumbnail->setCursor(Qt::PointingHandCursor);
                        thumbnail->installEventFilter(this);
                        _thumbnails.append(thumbnail);
                        imageRow->addWidget(thumbnail);
                    }
                    imageRow->addStretch(1);
                    layout->addLayout(imageRow);
                }
                if (!text.isEmpty())
                {
                    ShadLabel* label = new ShadLabel(text, this);
                    label->setTextFormat(Qt::PlainText);
                    label->setWordWrap(true);
                    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
                    layout->addWidget(label);
                }
                setMaximumWidth(kBubbleMaxWidth);
                QObject::connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                const int index = _thumbnails.indexOf(watched);
                if (index >= 0 && event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton && _onImageClicked)
                {
                    _onImageClicked(index);
                    return true;
                }
                return QWidget::eventFilter(watched, event);
            }

            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(Qt::NoPen);
                painter.setBrush(shadTheme->getColor(ShadThemeType::Muted));
                const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
                painter.drawRoundedRect(QRectF(rect()), radius, radius);
            }

        private:
            ImageClickHandler _onImageClicked;
            QList<QObject*> _thumbnails;
    };

    ShadField* createField(const ExampleI18n::TextFunction& label, QWidget* control, QWidget* parent)
    {
        ShadField* field = new ShadField(QString(), control, parent);
        field->setOrientation(ShadFieldType::Horizontal);
        ExampleI18n::bind(field, [field, label]() { field->setLabel(label()); });
        return field;
    }
}

AiChatPage::AiChatPage(QWidget* parent)
    : ExampleBasePage([] { return tr("AI 助手"); }, [] { return tr("基于组件库的 AI 对话示例，支持流式输出。"); }, parent), _client(new AiChatClient(this))
{
    for (int index = 0; index < static_cast<int>(_providerStates.size()); ++index)
    {
        _providerStates[index].apiKey = qEnvironmentVariable(kApiKeyVariables[index]);
        _providerStates[index].model = AiChatClient::defaultModel(static_cast<AiChatClient::Provider>(index));
    }

    ShadCard* settingsCard = addSection([] { return tr("连接设置"); }, [] { return QString(); });
    _providerComboBox = new ShadComboBox(this);
    _providerComboBox->addItems({QString(), QStringLiteral("Claude"), QStringLiteral("DeepSeek")});
    _endpointComboBox = new ShadComboBox(this);
    _endpointComboBox->addItems({QString(), QString()});
    _urlInput = new ShadInput(this);
    _modelInput = new ShadInput(this);
    _keyInput = new ShadInput(this);
    _keyInput->setEchoMode(QLineEdit::Password);
    _keyInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    _effortComboBox = new ShadComboBox(this);
    for (QWidget* control : {static_cast<QWidget*>(_providerComboBox), static_cast<QWidget*>(_endpointComboBox), static_cast<QWidget*>(_urlInput), static_cast<QWidget*>(_modelInput), static_cast<QWidget*>(_effortComboBox), static_cast<QWidget*>(_keyInput)})
    {
        control->setFixedWidth(kControlWidth);
    }
    ExampleI18n::bind(this, [this]()
    {
        _providerComboBox->setItemText(kOpenAiIndex, tr("OpenAI 兼容格式"));
        _endpointComboBox->setItemText(kOfficialIndex, tr("官方"));
        _endpointComboBox->setItemText(1, tr("自定义"));
        for (int index = 0; index < _effortComboBox->count(); ++index)
        {
            _effortComboBox->setItemText(index, effortLabel(_effortComboBox->itemData(index).toString()));
        }
        _keyInput->setPlaceholderText(tr("在此粘贴 API Key"));
    });
    addStretchItem(settingsCard, createField([] { return tr("服务商"); }, _providerComboBox, this));
    addStretchItem(settingsCard, createField([] { return tr("接口地址"); }, _endpointComboBox, this));
    addStretchItem(settingsCard, createField([] { return tr("基础地址"); }, _urlInput, this));
    addStretchItem(settingsCard, createField([] { return tr("模型"); }, _modelInput, this));
    addStretchItem(settingsCard, createField([] { return tr("推理强度"); }, _effortComboBox, this));
    addStretchItem(settingsCard, createField([] { return QStringLiteral("API Key"); }, _keyInput, this));
    connect(_providerComboBox, &ShadComboBox::currentIndexChanged, this, [this](int index)
    {
        if (index < 0)
        {
            return;
        }
        _providerIndex = index;
        loadProviderState();
    });
    connect(_endpointComboBox, &ShadComboBox::currentIndexChanged, this, [this](int index)
    {
        if (index < 0)
        {
            return;
        }
        _providerStates[_providerIndex].isOfficial = index == kOfficialIndex;
        updateEndpointInput();
    });
    connect(_urlInput, &QLineEdit::textEdited, this, [this](const QString& text) { _providerStates[_providerIndex].customUrl = text; });
    connect(_modelInput, &QLineEdit::textEdited, this, [this](const QString& text) { _providerStates[_providerIndex].model = text; });
    connect(_keyInput, &QLineEdit::textEdited, this, [this](const QString& text) { _providerStates[_providerIndex].apiKey = text; });
    connect(_effortComboBox, &ShadComboBox::currentIndexChanged, this, [this](int index)
    {
        if (index < 0)
        {
            return;
        }
        _providerStates[_providerIndex].effort = _effortComboBox->itemData(index).toString();
    });

    ShadCard* chatCard = addSection([] { return tr("对话"); }, [] { return tr("Ctrl+Enter 发送（macOS 上为 Cmd+Enter），回复以 Markdown 流式显示"); });
    _messageArea = new ShadScrollArea(this);
    _messageArea->setFixedHeight(kMessageAreaHeight);
    _messageContainer = new QWidget();
    _messageColumn = new QVBoxLayout(_messageContainer);
    _messageColumn->setContentsMargins(0, 0, 0, 0);
    _messageColumn->setSpacing(kMessageSpacing);
    _emptyLabel = new ShadLabel(_messageContainer);
    _emptyLabel->setColorRole(ShadThemeType::MutedForeground);
    _emptyLabel->setAlignment(Qt::AlignCenter);
    ExampleI18n::bind(_emptyLabel, [this]() { _emptyLabel->setText(tr("发送一条消息开始对话")); });
    _messageColumn->addWidget(_emptyLabel);
    _messageColumn->addStretch(1);
    _messageArea->setWidget(_messageContainer);
    addStretchItem(chatCard, _messageArea);
    QScrollBar* messageScrollBar = _messageArea->verticalScrollBar();
    connect(messageScrollBar, &QScrollBar::valueChanged, this, [this, messageScrollBar](int value) { _isFollowing = value >= messageScrollBar->maximum() - kFollowThreshold; });
    connect(messageScrollBar, &QScrollBar::rangeChanged, this, [this, messageScrollBar](int minimum, int maximum)
    {
        Q_UNUSED(minimum);
        if (_isFollowing)
        {
            messageScrollBar->setValue(maximum);
        }
    });

    _imageViewer = new ShadImageViewer(this);
    _attachmentGroup = new ShadAttachmentGroup(this);
    _attachmentGroup->hide();
    PromptTextarea* promptInput = new PromptTextarea(this);
    promptInput->setImageHandler([this](const QImage& image, const QString& name) { addPendingImage(image, name); });
    _promptInput = promptInput;
    _promptInput->setAutoResize(true);
    _promptInput->setMaximumAutoHeight(kPromptMaxHeight);
    _promptInput->installEventFilter(this);
    _sendButton = new ShadButton(QString(), ShadButtonType::Default, this);
    _sendButton->setShadIcon(ShadIconType::SendHorizontal);
    _attachButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    _attachButton->setShadIcon(ShadIconType::Paperclip);
    _stopButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    _stopButton->setShadIcon(ShadIconType::Square);
    _clearButton = new ShadButton(QString(), ShadButtonType::Ghost, this);
    _clearButton->setShadIcon(ShadIconType::Trash2);
    ExampleI18n::bind(_promptInput, [this]()
    {
        _promptInput->setPlaceholderText(tr("输入消息..."));
        _sendButton->setText(tr("发送"));
        _attachButton->setText(tr("添加图片"));
        _stopButton->setText(tr("停止"));
        _clearButton->setText(tr("清空对话"));
    });
    addStretchItem(chatCard, _attachmentGroup);
    addStretchItem(chatCard, _promptInput);
    addRow(chatCard, {_sendButton, _attachButton, _stopButton, _clearButton});
    connect(_sendButton, &ShadButton::clicked, this, [this]() { sendMessage(); });
    connect(_attachButton, &ShadButton::clicked, this, [this]() { chooseImages(); });
    connect(_stopButton, &ShadButton::clicked, this, [this]() { stopReply(); });
    connect(_clearButton, &ShadButton::clicked, this, [this]() { clearConversation(); });

    connect(_client, &AiChatClient::textReceived, this, [this](const QString& text)
    {
        removePendingMarker();
        if (_replyView == nullptr)
        {
            _replyView = new ShadMarkdownView(_messageContainer);
            appendWidget(_replyView);
        }
        _replyText += text;
        _replyView->appendMarkdown(text);
    });
    connect(_client, &AiChatClient::refused, this, [this]()
    {
        _isRefused = true;
        removePendingMarker();
        appendNotice(ShadIconType::ShieldAlert, tr("请求被安全策略拒绝"), ShadMarkerType::Default);
    });
    connect(_client, &AiChatClient::finished, this, [this]()
    {
        removePendingMarker();
        if (_replyText.isEmpty() && !_isRefused)
        {
            appendNotice(ShadIconType::CircleAlert, tr("没有返回内容"), ShadMarkerType::Default);
        }
        finishReply();
        updateControls();
    });
    connect(_client, &AiChatClient::failed, this, [this](const QString& message)
    {
        removePendingMarker();
        finishReply();
        appendError(message);
        updateControls();
    });

    loadProviderState();
    updateControls();
}

AiChatPage::~AiChatPage()
{
}

bool AiChatPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _promptInput && event->type() == QEvent::KeyPress)
    {
        const QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        const bool isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        if (isEnter && keyEvent->modifiers().testFlag(Qt::ControlModifier))
        {
            sendMessage();
            return true;
        }
    }
    return ExampleBasePage::eventFilter(watched, event);
}

AiChatClient::Provider AiChatPage::currentProvider() const
{
    return static_cast<AiChatClient::Provider>(_providerIndex);
}

void AiChatPage::loadProviderState()
{
    const ProviderState& state = _providerStates[_providerIndex];
    {
        const QSignalBlocker blocker(_endpointComboBox);
        _endpointComboBox->setCurrentIndex(state.isOfficial ? kOfficialIndex : 1);
    }
    _modelInput->setText(state.model);
    _keyInput->setText(state.apiKey);
    fillEffortItems();
    updateEndpointInput();
}

void AiChatPage::fillEffortItems()
{
    const QSignalBlocker blocker(_effortComboBox);
    _effortComboBox->clear();
    _effortComboBox->addItem(effortLabel(QString()), QString());
    for (const QString& level : AiChatClient::effortLevels())
    {
        _effortComboBox->addItem(effortLabel(level), level);
    }
    const int index = _effortComboBox->findData(_providerStates[_providerIndex].effort);
    _effortComboBox->setCurrentIndex(qMax(kOffEffortIndex, index));
}

QString AiChatPage::effortLabel(const QString& level) const
{
    if (level == QStringLiteral("low"))
    {
        return tr("低");
    }
    if (level == QStringLiteral("medium"))
    {
        return tr("中");
    }
    if (level == QStringLiteral("high"))
    {
        return tr("高");
    }
    if (level == QStringLiteral("xhigh"))
    {
        return tr("超高");
    }
    if (level == QStringLiteral("max"))
    {
        return tr("最高");
    }
    return tr("关闭");
}

void AiChatPage::updateEndpointInput()
{
    const ProviderState& state = _providerStates[_providerIndex];
    const AiChatClient::Provider provider = currentProvider();
    _urlInput->setText(state.isOfficial ? AiChatClient::officialBaseUrl(provider) : state.customUrl);
    _urlInput->setPlaceholderText(provider == AiChatClient::Provider::OpenAiCompatible ? QStringLiteral("https://api.example.com/v1") : QStringLiteral("https://api.example.com"));
    updateControls();
}

void AiChatPage::updateControls()
{
    const bool isRunning = _client->isRunning();
    _sendButton->setEnabled(!isRunning);
    _stopButton->setEnabled(isRunning);
    _providerComboBox->setEnabled(!isRunning);
    _endpointComboBox->setEnabled(!isRunning);
    _urlInput->setEnabled(!isRunning && !_providerStates[_providerIndex].isOfficial);
    _modelInput->setEnabled(!isRunning);
    _keyInput->setEnabled(!isRunning);
    _effortComboBox->setEnabled(!isRunning);
}

void AiChatPage::sendMessage()
{
    if (_client->isRunning())
    {
        return;
    }
    const QString text = _promptInput->toPlainText().trimmed();
    if (text.isEmpty() && _pendingImages.isEmpty())
    {
        return;
    }
    const ProviderState& state = _providerStates[_providerIndex];
    AiChatClient::Config config;
    config.provider = currentProvider();
    config.isOfficial = state.isOfficial;
    config.baseUrl = state.isOfficial ? AiChatClient::officialBaseUrl(config.provider) : state.customUrl;
    config.model = _modelInput->text().trimmed();
    config.apiKey = _keyInput->text().trimmed();
    config.effort = state.effort;
    if (config.apiKey.isEmpty())
    {
        appendError(tr("请先填写 API Key"));
        return;
    }
    if (config.model.isEmpty())
    {
        appendError(tr("请先填写模型"));
        return;
    }
    AiChatClient::Message message{QStringLiteral("user"), text, {}};
    QList<QImage> images;
    for (const PendingImage& pending : std::as_const(_pendingImages))
    {
        message.images.append(pending.image);
        images.append(pending.source);
    }
    _promptInput->clear();
    clearPendingImages();
    appendUserMessage(text, images);
    _history.append(message);
    _replyView = nullptr;
    _replyText.clear();
    _isRefused = false;
    _pendingMarker = new ShadMarker(ShadIconType::Sparkles, tr("思考中..."), _messageContainer);
    _pendingMarker->setShimmer(true);
    appendWidget(_pendingMarker);
    _isFollowing = true;
    _client->send(config, _history);
    updateControls();
}

void AiChatPage::stopReply()
{
    if (!_client->isRunning())
    {
        return;
    }
    _client->abort();
    removePendingMarker();
    finishReply();
    appendNotice(ShadIconType::None, tr("已停止"), ShadMarkerType::Separator);
    updateControls();
}

void AiChatPage::clearConversation()
{
    _client->abort();
    _pendingMarker = nullptr;
    _replyView = nullptr;
    _replyText.clear();
    _history.clear();
    for (int index = _messageColumn->count() - 1; index >= 0; --index)
    {
        QWidget* widget = _messageColumn->itemAt(index)->widget();
        if (widget != nullptr && widget != _emptyLabel)
        {
            delete _messageColumn->takeAt(index);
            widget->deleteLater();
        }
    }
    _emptyLabel->show();
    _isFollowing = true;
    updateControls();
}

void AiChatPage::appendWidget(QWidget* widget)
{
    _emptyLabel->hide();
    _messageColumn->insertWidget(_messageColumn->count() - 1, widget);
}

void AiChatPage::showImages(const QList<QImage>& images, int index)
{
    QList<QPixmap> pixmaps;
    for (const QImage& image : images)
    {
        pixmaps.append(QPixmap::fromImage(image));
    }
    _imageViewer->setPixmaps(pixmaps);
    _imageViewer->open(index);
}

void AiChatPage::appendUserMessage(const QString& text, const QList<QImage>& images)
{
    QWidget* row = new QWidget(_messageContainer);
    QHBoxLayout* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addStretch(1);
    rowLayout->addWidget(new UserBubble(text, images, [this, images](int index) { showImages(images, index); }, row));
    appendWidget(row);
}

void AiChatPage::chooseImages()
{
    const QStringList paths = QFileDialog::getOpenFileNames(this, tr("选择图片"), QString(), tr("图片 (*.png *.jpg *.jpeg *.gif *.webp *.bmp)"));
    for (const QString& path : paths)
    {
        addPendingImage(QImage(path), QFileInfo(path).fileName());
    }
}

void AiChatPage::addPendingImage(const QImage& image, const QString& name)
{
    const QString displayName = name.isEmpty() ? tr("粘贴的图片") : name;
    if (image.isNull())
    {
        appendNotice(ShadIconType::CircleAlert, tr("无法读取图片：%1").arg(displayName), ShadMarkerType::Default);
        return;
    }
    if (_pendingImages.size() >= kMaxImages)
    {
        appendNotice(ShadIconType::CircleAlert, tr("每条消息最多 %1 张图片").arg(kMaxImages), ShadMarkerType::Default);
        return;
    }
    PendingImage pending;
    pending.image = AiChatClient::encodeImage(image);
    pending.source = image;
    pending.preview = qMax(image.width(), image.height()) > kPreviewEdge ? image.scaled(kPreviewEdge, kPreviewEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation) : image;
    ShadAttachment* attachment = new ShadAttachment(ShadIconType::Image, displayName, QStringLiteral("%1 × %2").arg(image.width()).arg(image.height()), _attachmentGroup);
    attachment->setImage(QPixmap::fromImage(pending.preview));
    attachment->setClickable(true);
    ShadButton* removeButton = attachment->addAction(ShadIconType::X, tr("移除"));
    connect(removeButton, &ShadButton::clicked, this, [this, attachment]() { removePendingImage(attachment); });
    connect(attachment, &ShadAttachment::clicked, this, [this, attachment]()
    {
        QList<QImage> images;
        int index = 0;
        for (const PendingImage& item : std::as_const(_pendingImages))
        {
            if (item.attachment == attachment)
            {
                index = images.size();
            }
            images.append(item.source);
        }
        showImages(images, index);
    });
    pending.attachment = attachment;
    _pendingImages.append(pending);
    _attachmentGroup->addAttachment(attachment);
    _attachmentGroup->show();
}

void AiChatPage::removePendingImage(ShadAttachment* attachment)
{
    for (int index = 0; index < _pendingImages.size(); ++index)
    {
        if (_pendingImages.at(index).attachment == attachment)
        {
            _pendingImages.removeAt(index);
            _attachmentGroup->removeAttachment(attachment);
            attachment->deleteLater();
            break;
        }
    }
    _attachmentGroup->setVisible(!_pendingImages.isEmpty());
}

void AiChatPage::clearPendingImages()
{
    _pendingImages.clear();
    _attachmentGroup->clear();
    _attachmentGroup->hide();
}

void AiChatPage::appendError(const QString& message)
{
    appendWidget(new ShadAlert(ShadIconType::CircleAlert, tr("请求失败"), message, ShadAlertType::Destructive, _messageContainer));
}

void AiChatPage::appendNotice(ShadIconType::IconName icon, const QString& text, ShadMarkerType::Variant variant)
{
    ShadMarker* marker = new ShadMarker(icon, text, _messageContainer);
    marker->setVariant(variant);
    appendWidget(marker);
}

void AiChatPage::removePendingMarker()
{
    if (_pendingMarker == nullptr)
    {
        return;
    }
    _messageColumn->removeWidget(_pendingMarker);
    _pendingMarker->deleteLater();
    _pendingMarker = nullptr;
}

void AiChatPage::finishReply()
{
    if (!_replyText.isEmpty())
    {
        _history.append(AiChatClient::Message{QStringLiteral("assistant"), _replyText});
    }
    _replyText.clear();
    _replyView = nullptr;
}
