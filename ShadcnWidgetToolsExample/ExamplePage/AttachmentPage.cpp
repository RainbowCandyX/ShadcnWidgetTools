#include "AttachmentPage.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRandomGenerator>
#include <QString>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <functional>
#include <memory>
#include <utility>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadAttachment.h"
#include "ShadAttachmentGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadFileDropZone.h"
#include "ShadToaster.h"

namespace
{
    // 分组示例中横向附件宽度 w-64
    constexpr int kGroupItemWidth = 256 + 2 * ExampleLayout::kFocusRingWidth;
    // 示例图片边长
    constexpr int kPhotoSize = 360;
    // 上传示例的单个文件大小上限 10 MB
    constexpr qint64 kMaxUploadSize = 10 * 1024 * 1024;

    // 随状态变化的文字：保存生成当前文字的函数，状态变化时替换，切换语言时按当前状态重新生成
    struct DynamicText
    {
        ExampleI18n::TextFunction text;
        std::function<void(const QString&)> apply;

        void set(ExampleI18n::TextFunction newText)
        {
            text = std::move(newText);
            apply(text());
        }
    };

    std::shared_ptr<DynamicText> bindDynamicText(QObject* owner, std::function<void(const QString&)> apply, ExampleI18n::TextFunction text)
    {
        const std::shared_ptr<DynamicText> state = std::make_shared<DynamicText>(DynamicText{std::move(text), std::move(apply)});
        ExampleI18n::bind(owner, [state]() { state->apply(state->text()); });
        return state;
    }

    // 附件描述
    std::shared_ptr<DynamicText> bindDescription(ShadAttachment* attachment, ExampleI18n::TextFunction text)
    {
        return bindDynamicText(attachment, [attachment](const QString& value) { attachment->setDescription(value); }, std::move(text));
    }

    // 附件操作按钮的提示与无障碍名称
    std::shared_ptr<DynamicText> bindActionToolTip(ShadButton* action, ExampleI18n::TextFunction text)
    {
        return bindDynamicText(action, [action](const QString& value)
        {
            action->setToolTip(value);
            action->setAccessibleName(value);
        }, std::move(text));
    }

    // 纵向排列附件的容器，spacing 为附件可视间距；容器占满卡片宽度
    QWidget* createColumn(QWidget* parent, int spacing)
    {
        QWidget* column = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(spacing - 2 * ExampleLayout::kFocusRingWidth);
        return column;
    }

    // 不依赖网络的示例图片：天空渐变、太阳与两层山丘
    QPixmap createPhoto(const QColor& skyTop, const QColor& skyBottom, const QColor& sun, const QColor& farHill, const QColor& nearHill)
    {
        QPixmap pixmap(kPhotoSize, kPhotoSize);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient sky(0, 0, 0, kPhotoSize);
        sky.setColorAt(0.0, skyTop);
        sky.setColorAt(1.0, skyBottom);
        painter.fillRect(pixmap.rect(), sky);
        painter.setPen(Qt::NoPen);
        painter.setBrush(sun);
        painter.drawEllipse(QPointF(kPhotoSize * 0.68, kPhotoSize * 0.36), kPhotoSize * 0.12, kPhotoSize * 0.12);
        QPainterPath far;
        far.moveTo(0, kPhotoSize * 0.72);
        far.cubicTo(kPhotoSize * 0.25, kPhotoSize * 0.52, kPhotoSize * 0.5, kPhotoSize * 0.78, kPhotoSize, kPhotoSize * 0.58);
        far.lineTo(kPhotoSize, kPhotoSize);
        far.lineTo(0, kPhotoSize);
        far.closeSubpath();
        painter.setBrush(farHill);
        painter.drawPath(far);
        QPainterPath near;
        near.moveTo(0, kPhotoSize * 0.86);
        near.cubicTo(kPhotoSize * 0.35, kPhotoSize * 0.7, kPhotoSize * 0.65, kPhotoSize * 0.95, kPhotoSize, kPhotoSize * 0.8);
        near.lineTo(kPhotoSize, kPhotoSize);
        near.lineTo(0, kPhotoSize);
        near.closeSubpath();
        painter.setBrush(nearHill);
        painter.drawPath(near);
        return pixmap;
    }

    struct PhotoItem
    {
        QString name;
        QString meta;
        QPixmap image;
    };

    QList<PhotoItem> photoItems()
    {
        return {
            {
                QStringLiteral("workspace.png"), QStringLiteral("PNG · 820 KB"),
                createPhoto(QColor(0x93, 0xc5, 0xfd), QColor(0xe0, 0xf2, 0xfe), QColor(0xfd, 0xe0, 0x47), QColor(0x60, 0xa5, 0xfa), QColor(0x25, 0x63, 0xeb))
            },
            {
                QStringLiteral("desk-reference.jpg"), QStringLiteral("JPG · 1.1 MB"),
                createPhoto(QColor(0xfd, 0xba, 0x74), QColor(0xfe, 0xf3, 0xc7), QColor(0xff, 0xff, 0xff), QColor(0xf9, 0x73, 0x16), QColor(0x9a, 0x34, 0x12))
            },
            {
                QStringLiteral("office-reference.jpg"), QStringLiteral("JPG · 940 KB"),
                createPhoto(QColor(0x86, 0xef, 0xac), QColor(0xf0, 0xfd, 0xf4), QColor(0xfe, 0xf0, 0x8a), QColor(0x22, 0xc5, 0x5e), QColor(0x15, 0x80, 0x3d))
            },
        };
    }
} // namespace

AttachmentPage::AttachmentPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("附件"); }, []
    {
        return tr("ShadAttachment / ShadAttachmentGroup：显示文件或图片附件的图标、名称、上传状态与操作。");
    }, parent)
{
    // 操作按钮的反馈统一用 toast 显示
    const auto notify = [toaster](const QString& title, const QString& description)
    {
        if (toaster != nullptr)
        {
            toaster->add(title, description);
        }
    };

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("图标、标题与描述，结束侧带操作按钮"); });
    QWidget* basicColumn = createColumn(this, 12);
    ShadAttachment* uploadingAttachment = new ShadAttachment(ShadIconType::LoaderCircle, QStringLiteral("sales-dashboard.pdf"), QString(), basicColumn);
    bindDescription(uploadingAttachment, [] { return tr("上传中 · 64%"); });
    uploadingAttachment->setState(ShadAttachmentType::Uploading);
    ShadButton* cancelUploadAction = uploadingAttachment->addAction(ShadIconType::X, QString());
    bindActionToolTip(cancelUploadAction, [] { return tr("取消上传"); });
    connect(cancelUploadAction, &ShadButton::clicked, this, [notify]()
    {
        notify(tr("已取消上传"), QStringLiteral("sales-dashboard.pdf"));
    });
    ShadAttachment* codeAttachment = new ShadAttachment(ShadIconType::FileCode, QStringLiteral("message-renderer.tsx"), QStringLiteral("TypeScript · 12 KB"), basicColumn);
    ShadButton* removeCodeAction = codeAttachment->addAction(ShadIconType::X, QString());
    bindActionToolTip(removeCodeAction, [] { return tr("移除 %1").arg(QStringLiteral("message-renderer.tsx")); });
    connect(removeCodeAction, &ShadButton::clicked, this, [notify]()
    {
        notify(tr("已移除附件"), QStringLiteral("message-renderer.tsx"));
    });
    basicColumn->layout()->addWidget(uploadingAttachment);
    basicColumn->layout()->addWidget(codeAttachment);
    addStretchItem(basicCard, basicColumn);

    ShadCard* stateCard = addSection([] { return tr("状态"); }, [] { return tr("等待、上传中、处理中、失败与完成五种状态"); });
    QWidget* stateColumn = createColumn(this, 8);
    struct StateDemo
    {
        ShadAttachmentType::State state;
        ShadIconType::IconName icon;
        QString name;
        ExampleI18n::TextFunction description;
    };
    const QList<StateDemo> stateDemos{
        {ShadAttachmentType::Idle, ShadIconType::Clock, QStringLiteral("selected-file.pdf"), [] { return tr("等待上传"); }},
        {
            ShadAttachmentType::Uploading, ShadIconType::LoaderCircle, QStringLiteral("design-system.zip"),
            [] { return tr("上传中 · 64%"); }
        },
        {
            ShadAttachmentType::Processing, ShadIconType::FileText, QStringLiteral("market-research.pdf"),
            [] { return tr("正在处理文档"); }
        },
        {
            ShadAttachmentType::Error, ShadIconType::FileWarning, QStringLiteral("financial-model.xlsx"),
            [] { return tr("上传失败，请重试。"); }
        },
        {
            ShadAttachmentType::Done, ShadIconType::Check, QStringLiteral("uploaded-report.pdf"),
            [] { return tr("已上传 · 1.8 MB"); }
        },
    };
    for (const StateDemo& demo : stateDemos)
    {
        ShadAttachment* attachment = new ShadAttachment(demo.icon, demo.name, QString(), stateColumn);
        const std::shared_ptr<DynamicText> description = bindDescription(attachment, demo.description);
        attachment->setState(demo.state);
        const QString name = demo.name;
        if (demo.state == ShadAttachmentType::Error)
        {
            // 重试：进入上传中，2 秒后完成
            QPointer<ShadAttachment> guard(attachment);
            ShadButton* retryAction = attachment->addAction(ShadIconType::RefreshCw, QString());
            bindActionToolTip(retryAction, [] { return tr("重新上传"); });
            connect(retryAction, &ShadButton::clicked, this, [guard, description]()
            {
                if (guard.isNull())
                {
                    return;
                }
                guard->setState(ShadAttachmentType::Uploading);
                guard->setShadIcon(ShadIconType::LoaderCircle);
                description->set([] { return tr("上传中..."); });
                QTimer::singleShot(2000, guard.data(), [guard, description]()
                {
                    guard->setState(ShadAttachmentType::Done);
                    guard->setShadIcon(ShadIconType::Check);
                    description->set([] { return tr("已上传 · 2.6 MB"); });
                });
            });
        }
        ShadButton* removeAction = attachment->addAction(ShadIconType::X, QString());
        bindActionToolTip(removeAction, [name] { return tr("移除 %1").arg(name); });
        connect(removeAction, &ShadButton::clicked, this, [notify, name]() { notify(tr("已移除附件"), name); });
        stateColumn->layout()->addWidget(attachment);
    }
    addStretchItem(stateCard, stateColumn);

    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("setAttachmentSize：默认、小号与超小三种尺寸"); });
    QWidget* sizeColumn = createColumn(this, 12);
    ShadAttachment* mediumAttachment = new ShadAttachment(ShadIconType::FileText, QString(), QStringLiteral("PDF · 2.4 MB"), sizeColumn);
    ShadAttachment* smallAttachment = new ShadAttachment(ShadIconType::FileText, QString(), QStringLiteral("PDF · 2.4 MB"), sizeColumn);
    smallAttachment->setAttachmentSize(ShadAttachmentType::Small);
    ShadAttachment* extraSmallAttachment = new ShadAttachment(ShadIconType::FileText, QString(), QString(), sizeColumn);
    extraSmallAttachment->setAttachmentSize(ShadAttachmentType::ExtraSmall);
    ExampleI18n::bind(sizeColumn, [mediumAttachment, smallAttachment, extraSmallAttachment]()
    {
        mediumAttachment->setTitle(tr("默认附件"));
        smallAttachment->setTitle(tr("小号附件"));
        extraSmallAttachment->setTitle(tr("超小附件"));
    });
    sizeColumn->layout()->addWidget(mediumAttachment);
    sizeColumn->layout()->addWidget(smallAttachment);
    sizeColumn->layout()->addWidget(extraSmallAttachment);
    addStretchItem(sizeCard, sizeColumn);

    ShadCard* imageCard = addSection([] { return tr("图片"); }, [] { return tr("显示裁剪为正方形的图片附件，可点击打开"); });
    const QList<PhotoItem> photos = photoItems();
    ShadAttachmentGroup* imageGroup = new ShadAttachmentGroup(this);
    for (const PhotoItem& photo : photos)
    {
        ShadAttachment* attachment = new ShadAttachment(imageGroup);
        attachment->setOrientation(Qt::Vertical);
        attachment->setImage(photo.image);
        attachment->setTitle(photo.name);
        attachment->setDescription(photo.meta);
        attachment->setClickable(true);
        const QString name = photo.name;
        ExampleI18n::bind(attachment, [attachment, name]() { attachment->setToolTip(tr("打开 %1").arg(name)); });
        connect(attachment, &ShadAttachment::clicked, this, [notify, name]() { notify(tr("打开图片"), name); });
        ShadButton* removeAction = attachment->addAction(ShadIconType::X, QString());
        bindActionToolTip(removeAction, [name] { return tr("移除 %1").arg(name); });
        connect(removeAction, &ShadButton::clicked, attachment, &ShadAttachment::deleteLater);
        imageGroup->addAttachment(attachment);
    }
    addStretchItem(imageCard, imageGroup);

    ShadCard* groupCard = addSection([] { return tr("分组"); }, [] { return tr("横向滚动的附件分组，可移除附件"); });
    ShadAttachmentGroup* fileGroup = new ShadAttachmentGroup(this);
    struct FileItem
    {
        QString name;
        QString meta;
        ShadIconType::IconName icon;
        QPixmap image;
    };
    const QList<FileItem> fileItems{
        {QStringLiteral("briefing-notes.pdf"), QStringLiteral("PDF · 1.4 MB"), ShadIconType::FileText, QPixmap()},
        {QStringLiteral("workspace.png"), QStringLiteral("PNG · 820 KB"), ShadIconType::None, photos.first().image},
        {QStringLiteral("customers.csv"), QStringLiteral("CSV · 18 KB"), ShadIconType::Table, QPixmap()},
        {QStringLiteral("renderer.tsx"), QStringLiteral("TSX · 12 KB"), ShadIconType::FileCode, QPixmap()},
    };
    for (const FileItem& item : fileItems)
    {
        ShadAttachment* attachment = new ShadAttachment(item.icon, item.name, item.meta, fileGroup);
        attachment->setImage(item.image);
        attachment->setFixedWidth(kGroupItemWidth);
        const QString name = item.name;
        ShadButton* removeAction = attachment->addAction(ShadIconType::X, QString());
        bindActionToolTip(removeAction, [name] { return tr("移除 %1").arg(name); });
        connect(removeAction, &ShadButton::clicked, attachment, &ShadAttachment::deleteLater);
        fileGroup->addAttachment(attachment);
    }
    addStretchItem(groupCard, fileGroup);

    ShadCard* triggerCard = addSection([] { return tr("可点击"); }, [] { return tr("setClickable(true)：卡片可点击，悬停时背景变化"); });
    QWidget* triggerColumn = createColumn(this, 12);
    ShadAttachment* previewAttachment = new ShadAttachment(ShadIconType::FileText, QStringLiteral("research-summary.pdf"), QString(), triggerColumn);
    previewAttachment->setClickable(true);
    ExampleI18n::bind(previewAttachment, [previewAttachment]()
    {
        previewAttachment->setDescription(tr("PDF · 3.2 MB · 点击预览"));
        previewAttachment->setToolTip(tr("预览 %1").arg(QStringLiteral("research-summary.pdf")));
    });
    connect(previewAttachment, &ShadAttachment::clicked, this, [notify]()
    {
        notify(tr("打开预览"), QStringLiteral("research-summary.pdf"));
    });
    triggerColumn->layout()->addWidget(previewAttachment);
    addStretchItem(triggerCard, triggerColumn);

    ShadCard* linkCard = addSection([] { return tr("链接"); }, [] { return tr("setUrl：点击卡片打开网址或本地文件"); });
    QWidget* linkColumn = createColumn(this, 12);
    ShadAttachment* docAttachment = new ShadAttachment(ShadIconType::ExternalLink, QStringLiteral("ShadcnWidgetTools - GitHub"), QStringLiteral("github.com"), linkColumn);
    docAttachment->setUrl(QUrl(QStringLiteral("https://github.com/RainbowCandyX/ShadcnWidgetTools")));
    linkColumn->layout()->addWidget(docAttachment);
    // 本地文件链接：在临时目录生成一个示例文本文件，setFile 读取文件信息；文件内容只在创建页面时按当时的语言写入
    ShadAttachment* fileAttachment = new ShadAttachment(linkColumn);
    const QString readmePath = QDir::temp().filePath(QStringLiteral("ShadcnWidgetTools-readme.txt"));
    QFile readmeFile(readmePath);
    if (readmeFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QTextStream stream(&readmeFile);
        stream.setEncoding(QStringConverter::Utf8);
        stream << tr("ShadcnWidgetTools 示例文件\n\n点击附件卡片时由系统默认程序打开。\n");
        readmeFile.close();
    }
    else
    {
        qWarning("无法创建示例文件：%s，%s", qUtf8Printable(QDir::toNativeSeparators(readmePath)), qUtf8Printable(readmeFile.errorString()));
    }
    if (fileAttachment->setFile(readmePath))
    {
        fileAttachment->setUrl(QUrl::fromLocalFile(readmePath));
    }
    else
    {
        fileAttachment->setShadIcon(ShadIconType::FileWarning);
        fileAttachment->setTitle(QStringLiteral("readme.txt"));
        bindDescription(fileAttachment, [] { return tr("无法创建示例文件"); });
        fileAttachment->setState(ShadAttachmentType::Error);
    }
    linkColumn->layout()->addWidget(fileAttachment);
    // 不自动打开，由程序自己处理链接
    ShadAttachment* inAppAttachment = new ShadAttachment(ShadIconType::FileText, QStringLiteral("quarterly-review.pdf"), QString(), linkColumn);
    bindDescription(inAppAttachment, [] { return tr("PDF · 4.1 MB · 程序内预览"); });
    inAppAttachment->setUrl(QUrl(QStringLiteral("https://example.com/files/quarterly-review.pdf")));
    inAppAttachment->setOpenExternalLinks(false);
    connect(inAppAttachment, &ShadAttachment::linkActivated, this, [notify](const QUrl& url)
    {
        notify(tr("在程序内预览"), url.toString());
    });
    linkColumn->layout()->addWidget(inAppAttachment);
    addStretchItem(linkCard, linkColumn);

    ShadCard* uploadCard = addSection([] { return tr("上传"); }, [] { return tr("拖入或选择文件，生成附件并模拟上传"); });
    QWidget* uploadColumn = createColumn(this, 12);
    ShadFileDropZone* dropZone = new ShadFileDropZone(uploadColumn);
    ExampleI18n::bind(dropZone, [dropZone]()
    {
        dropZone->setNameFilters({tr("图片与 PDF (*.png *.jpg *.jpeg *.gif *.webp *.pdf)")});
        dropZone->setDescription(tr("或点击选择文件，支持 PNG、JPG、GIF、WebP、PDF，单个不超过 10 MB"));
    });
    dropZone->setMaximumFileSize(kMaxUploadSize);
    ShadAttachmentGroup* uploadGroup = new ShadAttachmentGroup(uploadColumn);
    uploadGroup->hide();
    uploadColumn->layout()->addWidget(dropZone);
    uploadColumn->layout()->addWidget(uploadGroup);
    // 全部移除后隐藏分组；附件删除完成后才能统计到正确数量
    const auto updateGroupVisibility = [uploadGroup]()
    {
        QTimer::singleShot(0, uploadGroup, [uploadGroup]() { uploadGroup->setVisible(uploadGroup->count() > 0); });
    };
    connect(dropZone, &ShadFileDropZone::filesSelected, this, [uploadGroup, updateGroupVisibility](const QStringList& paths)
    {
        for (const QString& path : paths)
        {
            ShadAttachment* attachment = new ShadAttachment(uploadGroup);
            attachment->setFixedWidth(kGroupItemWidth);
            if (!attachment->setFile(path))
            {
                delete attachment;
                continue;
            }
            const QString meta = attachment->getDescription();
            attachment->setState(ShadAttachmentType::Uploading);
            const std::shared_ptr<DynamicText> description = bindDescription(attachment, [] { return tr("上传中 · 0%"); });
            ShadButton* removeAction = attachment->addAction(ShadIconType::X, QString());
            const std::shared_ptr<DynamicText> actionToolTip = bindActionToolTip(removeAction, [] { return tr("取消上传"); });
            connect(removeAction, &ShadButton::clicked, attachment, &ShadAttachment::deleteLater);
            connect(attachment, &QObject::destroyed, uploadGroup, updateGroupVisibility);
            // 模拟上传：每 100ms 前进一段，到 100% 后完成
            QTimer* timer = new QTimer(attachment);
            timer->setInterval(100);
            connect(timer, &QTimer::timeout, attachment, [attachment, timer, meta, description, actionToolTip, progress = 0]() mutable
            {
                progress = qMin(100, progress + QRandomGenerator::global()->bounded(4, 12));
                if (progress < 100)
                {
                    const int currentProgress = progress;
                    description->set([currentProgress]
                    {
                        return tr("上传中 · %1%").arg(currentProgress);
                    });
                    return;
                }
                timer->stop();
                attachment->setState(ShadAttachmentType::Done);
                description->set([meta] { return tr("已上传 · %1").arg(meta); });
                // 上传完成后按钮的含义变为移除
                const QString title = attachment->getTitle();
                actionToolTip->set([title] { return tr("移除 %1").arg(title); });
            });
            timer->start();
            // 新文件放在最前面
            uploadGroup->insertAttachment(0, attachment);
        }
        uploadGroup->setVisible(uploadGroup->count() > 0);
        if (ShadAttachment* first = uploadGroup->attachmentAt(0))
        {
            uploadGroup->scrollToAttachment(first);
        }
    });
    connect(dropZone, &ShadFileDropZone::filesRejected, this, [toaster](const QStringList& paths, const QString& reason)
    {
        if (toaster == nullptr)
        {
            return;
        }
        QStringList names;
        for (const QString& path : paths)
        {
            names.append(QFileInfo(path).fileName());
        }
        toaster->add(tr("无法添加文件：%1").arg(reason), names.join(tr("、", "文件名列表分隔符")), ShadToastType::Error);
    });
    addStretchItem(uploadCard, uploadColumn);
}

AttachmentPage::~AttachmentPage()
{
}
