#include "ScreenshotPage.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadImageViewer.h"
#include "ShadKeySequenceEdit.h"
#include "ShadLabel.h"
#include "ShadScreenshot.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kPreviewWidth = 360;
    constexpr int kPreviewHeight = 220;
    const QKeySequence kDefaultShortcut(Qt::CTRL | Qt::ALT | Qt::Key_S);

    enum class Status
    {
        Ready = 0,
        Unavailable,
        Captured,
        Saved,
        SaveFailed,
        Cancelled,
        Pinned,
    };

    struct PageState
    {
        Status status{Status::Ready};
        QSize size;
        QString filePath;
    };

    enum class ShortcutStatus
    {
        None = 0,
        Unsupported,
        Registered,
        Failed,
    };

    struct ShortcutState
    {
        ShortcutStatus status{ShortcutStatus::None};
        QKeySequence sequence;
    };

    void updateShortcutText(ShadLabel* label, const ShortcutState& state)
    {
        const QString text = state.sequence.toString(QKeySequence::NativeText);
        switch (state.status)
        {
            case ShortcutStatus::None:
                label->setText(QCoreApplication::translate("ScreenshotPage", "未注册快捷键"));
                break;
            case ShortcutStatus::Unsupported:
                label->setText(QCoreApplication::translate("ScreenshotPage", "当前平台不支持全局快捷键"));
                break;
            case ShortcutStatus::Registered:
                label->setText(QCoreApplication::translate("ScreenshotPage", "已注册 %1").arg(text));
                break;
            case ShortcutStatus::Failed:
                label->setText(QCoreApplication::translate("ScreenshotPage", "注册失败，%1 被占用或不支持").arg(text));
                break;
        }
    }

    void updateStatusText(ShadLabel* label, const PageState& state)
    {
        switch (state.status)
        {
            case Status::Ready:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 无"));
                break;
            case Status::Unavailable:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 无法开始截图，当前平台不支持或没有屏幕录制权限"));
                break;
            case Status::Captured:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 已截取 %1 × %2").arg(state.size.width()).arg(state.size.height()));
                break;
            case Status::Saved:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 已保存到 %1").arg(state.filePath));
                break;
            case Status::SaveFailed:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 保存失败, %1").arg(state.filePath));
                break;
            case Status::Cancelled:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 已取消"));
                break;
            case Status::Pinned:
                label->setText(QCoreApplication::translate("ScreenshotPage", "结果: 已钉在屏幕上，拖动移动，滚轮缩放，双击或 Esc 关闭"));
                break;
        }
    }
}

ScreenshotPage::ScreenshotPage(QWidget* parent)
    : ExampleBasePage([] { return tr("截图"); }, [] { return tr("ShadScreenshot：框选屏幕区域或单击窗口截图，可添加标注，复制、保存或钉在屏幕上。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("拖动框选或单击窗口，Enter 完成，Esc 取消"); });
    ShadScreenshot* screenshot = new ShadScreenshot(this);

    ShadSwitch* ownWindowsSwitch = new ShadSwitch(this);
    ExampleI18n::bind(ownWindowsSwitch, [ownWindowsSwitch]() { ownWindowsSwitch->setText(tr("截图包含本程序窗口")); });
    connect(ownWindowsSwitch, &QAbstractButton::toggled, screenshot, &ShadScreenshot::setOwnWindowsVisible);
    ShadSwitch* cornerPinSwitch = new ShadSwitch(this);
    ExampleI18n::bind(cornerPinSwitch, [cornerPinSwitch]() { cornerPinSwitch->setText(tr("选区右上角显示钉住按钮")); });
    connect(cornerPinSwitch, &QAbstractButton::toggled, screenshot, &ShadScreenshot::setCornerPinButtonVisible);
    addRow(basicCard, {ownWindowsSwitch, cornerPinSwitch});

    ShadButton* startButton = new ShadButton(QString(), ShadButtonType::Default, this);
    startButton->setShadIcon(ShadIconType::Crop);
    startButton->setEnabled(ShadScreenshot::isSupported());
    ExampleI18n::bind(startButton, [startButton]() { startButton->setText(tr("开始截图")); });
    ShadButton* viewButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    viewButton->setShadIcon(ShadIconType::Maximize);
    viewButton->setEnabled(false);
    ExampleI18n::bind(viewButton, [viewButton]() { viewButton->setText(tr("查看大图")); });
    ShadButton* clearButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    clearButton->setShadIcon(ShadIconType::Trash2);
    clearButton->setEnabled(false);
    ExampleI18n::bind(clearButton, [clearButton]() { clearButton->setText(tr("清除图片")); });
    ShadButton* closePinsButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    closePinsButton->setShadIcon(ShadIconType::PinOff);
    closePinsButton->setEnabled(false);
    ExampleI18n::bind(closePinsButton, [closePinsButton]() { closePinsButton->setText(tr("关闭全部贴图")); });
    addRow(basicCard, {startButton, viewButton, clearButton, closePinsButton});

    ShadLabel* statusLabel = new ShadLabel(this);
    statusLabel->setColorRole(ShadThemeType::MutedForeground);
    statusLabel->setWordWrap(true);
    addStretchItem(basicCard, statusLabel);

    QLabel* previewLabel = new QLabel(this);
    previewLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    previewLabel->hide();
    addItem(basicCard, previewLabel);

    ShadImageViewer* viewer = new ShadImageViewer(this);
    const std::shared_ptr<PageState> state = std::make_shared<PageState>();
    ExampleI18n::bind(statusLabel, [statusLabel, state]() { updateStatusText(statusLabel, *state); });

    connect(startButton, &QAbstractButton::clicked, this, [screenshot, statusLabel, state]()
    {
        if (!screenshot->start())
        {
            state->status = Status::Unavailable;
            updateStatusText(statusLabel, *state);
        }
    });
    connect(viewButton, &QAbstractButton::clicked, viewer, [viewer]() { viewer->open(0); });
    connect(clearButton, &QAbstractButton::clicked, this, [statusLabel, previewLabel, viewButton, clearButton, viewer, state]()
    {
        previewLabel->clear();
        previewLabel->hide();
        viewer->setPixmaps({});
        viewButton->setEnabled(false);
        clearButton->setEnabled(false);
        *state = PageState();
        updateStatusText(statusLabel, *state);
    });
    connect(screenshot, &ShadScreenshot::captured, this, [this, statusLabel, previewLabel, viewButton, clearButton, viewer, state](const QImage& image)
    {
        const QPixmap pixmap = QPixmap::fromImage(image);
        const qreal ratio = devicePixelRatioF();
        QPixmap preview = pixmap.scaled(QSize(kPreviewWidth, kPreviewHeight) * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        preview.setDevicePixelRatio(ratio);
        previewLabel->setPixmap(preview);
        previewLabel->show();
        viewer->setPixmaps({pixmap});
        viewButton->setEnabled(true);
        clearButton->setEnabled(true);
        state->status = Status::Captured;
        state->size = image.size();
        updateStatusText(statusLabel, *state);
    });
    connect(closePinsButton, &QAbstractButton::clicked, screenshot, [screenshot, closePinsButton]()
    {
        screenshot->closeAllPins();
        closePinsButton->setEnabled(false);
    });
    connect(screenshot, &ShadScreenshot::pinned, this, [statusLabel, closePinsButton, state]()
    {
        closePinsButton->setEnabled(true);
        state->status = Status::Pinned;
        updateStatusText(statusLabel, *state);
    });
    connect(screenshot, &ShadScreenshot::saved, this, [statusLabel, state](const QString& filePath)
    {
        state->status = Status::Saved;
        state->filePath = filePath;
        updateStatusText(statusLabel, *state);
    });
    connect(screenshot, &ShadScreenshot::saveFailed, this, [statusLabel, state](const QString& filePath)
    {
        state->status = Status::SaveFailed;
        state->filePath = filePath;
        updateStatusText(statusLabel, *state);
    });
    connect(screenshot, &ShadScreenshot::canceled, this, [statusLabel, state]()
    {
        state->status = Status::Cancelled;
        updateStatusText(statusLabel, *state);
    });

    ShadCard* shortcutCard = addSection([] { return tr("全局快捷键"); }, [] { return tr("示例程序默认注册 %1，在其他程序中也能开始截图").arg(kDefaultShortcut.toString(QKeySequence::NativeText)); });
    ShadKeySequenceEdit* shortcutEdit = new ShadKeySequenceEdit(this);
    shortcutEdit->setFixedWidth(ExampleLayout::kFieldWidth);
    shortcutEdit->setEnabled(ShadScreenshot::isShortcutSupported());
    ExampleI18n::bind(shortcutEdit, [shortcutEdit]() { shortcutEdit->setPlaceholderText(tr("点击后按下快捷键")); });
    addRow(shortcutCard, {shortcutEdit});
    ShadLabel* shortcutLabel = new ShadLabel(this);
    shortcutLabel->setColorRole(ShadThemeType::MutedForeground);
    shortcutLabel->setWordWrap(true);
    addStretchItem(shortcutCard, shortcutLabel);

    const std::shared_ptr<ShortcutState> shortcutState = std::make_shared<ShortcutState>();
    shortcutState->status = ShadScreenshot::isShortcutSupported() ? ShortcutStatus::None : ShortcutStatus::Unsupported;
    ExampleI18n::bind(shortcutLabel, [shortcutLabel, shortcutState]() { updateShortcutText(shortcutLabel, *shortcutState); });
    const auto applyShortcut = [screenshot, shortcutEdit, shortcutLabel, shortcutState]()
    {
        if (shortcutEdit->isRecording())
        {
            return;
        }
        const QKeySequence sequence = shortcutEdit->getKeySequence();
        const bool isRegistered = screenshot->setShortcut(sequence);
        shortcutEdit->setInvalid(!isRegistered);
        shortcutState->sequence = sequence;
        shortcutState->status = sequence.isEmpty() ? ShortcutStatus::None : (isRegistered ? ShortcutStatus::Registered : ShortcutStatus::Failed);
        updateShortcutText(shortcutLabel, *shortcutState);
    };
    connect(shortcutEdit, &ShadKeySequenceEdit::keySequenceChanged, this, applyShortcut);
    connect(shortcutEdit, &ShadKeySequenceEdit::editingFinished, this, applyShortcut);
    if (ShadScreenshot::isShortcutSupported())
    {
        shortcutEdit->setKeySequence(kDefaultShortcut);
        applyShortcut();
    }
}

ScreenshotPage::~ScreenshotPage()
{
}
