#include "DialogPage.h"

#include <QCoreApplication>
#include <QDialog>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>
#include <utility>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadScrollArea.h"
#include "ShadTextarea.h"

namespace
{
    // dialog-demo 的 sm:max-w-sm
    constexpr int kFormDialogWidth = 384;
    // 可滚动内容 max-h-[50vh]：打开时的高度，缩小对话框时最低保留的高度
    constexpr int kScrollHeight = 320;
    constexpr int kMinimumScrollHeight = 120;
    // 固定尺寸示例的窗口尺寸
    constexpr int kFixedDialogWidth = 400;
    constexpr int kFixedDialogHeight = 260;
    // 段落数量与段落间距 mb-4
    constexpr int kParagraphCount = 10;
    constexpr int kParagraphGap = 16;
    // FieldGroup gap-7 与 Field gap-3
    constexpr int kFieldGroupGap = 28;
    constexpr int kFieldGap = 12;

    // 对话框的标题与说明随语言刷新，打开期间切换语言同样生效
    void bindDialogTexts(ShadDialog* dialog, ExampleI18n::TextFunction title, ExampleI18n::TextFunction description)
    {
        ExampleI18n::bind(dialog, [dialog, title = std::move(title), description = std::move(description)]()
        {
            dialog->setTitle(title());
            dialog->setDescription(description());
        });
    }

    // 文字随语言刷新的按钮
    ShadButton* createButton(ExampleI18n::TextFunction text, ShadButtonType::Variant variant, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), variant, parent);
        ExampleI18n::bind(button, [button, text = std::move(text)]() { button->setText(text()); });
        return button;
    }

    // 表单字段标签：左右各留焦点环边距，与输入框可视边缘对齐
    ShadLabel* createFieldLabel(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
        ExampleI18n::bind(label, [label, text = std::move(text)]() { label->setText(text()); });
        return label;
    }

    // Field：标签在上、输入框在下，间距扣除输入框上方的焦点环边距；输入框的值只在创建时设置
    QVBoxLayout* createField(ExampleI18n::TextFunction label, const QString& value, QWidget* parent)
    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kFieldGap - ExampleLayout::kFocusRingWidth);
        ShadLabel* fieldLabel = createFieldLabel(std::move(label), parent);
        ShadInput* input = new ShadInput(parent);
        input->setText(value);
        fieldLabel->setBuddy(input);
        layout->addWidget(fieldLabel);
        layout->addWidget(input);
        return layout;
    }

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 打开时高度为 kScrollHeight 的滚动区域：对话框按 sizeHint 计算打开时的高度，之后拖动放大对话框时随之变高，不留空白
    class ScrollContentArea : public ShadScrollArea
    {
        public:
            explicit ScrollContentArea(QWidget* parent)
                : ShadScrollArea(parent)
            {
                setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
                setMinimumHeight(kMinimumScrollHeight);
            }

            QSize sizeHint() const override
            {
                return QSize(ShadScrollArea::sizeHint().width(), kScrollHeight);
            }
    };

    // 滚动区域，内容为多段自动换行的文字
    ShadScrollArea* createScrollContent(QWidget* parent)
    {
        ShadScrollArea* area = new ScrollContentArea(parent);
        QWidget* content = new QWidget(area);
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kParagraphGap);
        for (int index = 0; index < kParagraphCount; ++index)
        {
            ShadLabel* paragraph = new ShadLabel(content);
            ExampleI18n::bind(paragraph, [paragraph, index]() { paragraph->setText(QCoreApplication::translate("DialogPage", "第 %1 段：这是一段用于演示滚动的示例文字。对话框内容超过可用高度时，内容区单独滚动，标题区保持在顶部可见。把 ShadScrollArea 放进 contentLayout 并限制高度即可得到这种效果，页脚位于内容区之外，同样不会随内容滚动。").arg(index + 1)); });
            paragraph->setWordWrap(true);
            layout->addWidget(paragraph);
        }
        area->setWidget(content);
        return area;
    }

    // 继承 ShadDialog：在构造函数里填充内容，外观、动画与关闭方式和其他对话框一致
    class FeedbackDialog : public ShadDialog
    {
        public:
            explicit FeedbackDialog(QWidget* parent = nullptr)
                : ShadDialog(parent)
            {
                ExampleLayout::addStayOnTopButton(this);
                contentLayout()->setSpacing(16 - 2 * ExampleLayout::kFocusRingWidth);
                _textarea = new ShadTextarea(this);
                // 不随内容自动增高，放大对话框时多行输入框跟着变高，占据多出的空间
                _textarea->setAutoResize(false);
                _contactCheckbox = new ShadCheckbox(this);
                contentLayout()->addWidget(_textarea);
                contentLayout()->addWidget(_contactCheckbox);
                ShadButton* cancelButton = new ShadButton(QString(), ShadButtonType::Outline, this);
                _sendButton = new ShadButton(this);
                _sendButton->setEnabled(false);
                footerLayout()->addWidget(cancelButton);
                footerLayout()->addWidget(_sendButton);
                // 只刷新文字，用户已输入的反馈内容不变
                ExampleI18n::bind(this, [this, cancelButton]()
                {
                    setTitle(QCoreApplication::translate("DialogPage", "发送反馈"));
                    setDescription(QCoreApplication::translate("DialogPage", "告诉我们你遇到的问题或建议，我们会尽快回复。"));
                    _textarea->setPlaceholderText(QCoreApplication::translate("DialogPage", "请输入你的反馈"));
                    _contactCheckbox->setText(QCoreApplication::translate("DialogPage", "允许通过邮件联系我"));
                    cancelButton->setText(QCoreApplication::translate("DialogPage", "取消"));
                    _sendButton->setText(QCoreApplication::translate("DialogPage", "发送"));
                });
                connect(cancelButton, &ShadButton::clicked, this, &ShadDialog::reject);
                connect(_sendButton, &ShadButton::clicked, this, &ShadDialog::accept);
                connect(_textarea, &ShadTextarea::textChanged, this, [this]() { _sendButton->setEnabled(!_textarea->toPlainText().trimmed().isEmpty()); });
            }

            QString getFeedback() const
            {
                return _textarea->toPlainText().trimmed();
            }

            bool isContactAllowed() const
            {
                return _contactCheckbox->isChecked();
            }

        private:
            ShadTextarea* _textarea{nullptr};
            ShadCheckbox* _contactCheckbox{nullptr};
            ShadButton* _sendButton{nullptr};
    };

    // 最近一次发送的反馈；size 小于 0 表示尚未发送
    struct FeedbackState
    {
        int size{-1};
        bool isContactAllowed{false};
    };

    // dialog-demo：编辑资料表单
    ShadDialog* createProfileDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "编辑资料"); }, [] { return QCoreApplication::translate("DialogPage", "在这里修改你的个人资料，完成后点击保存。"); });
        dialog->resize(kFormDialogWidth, dialog->height());
        ExampleLayout::addStayOnTopButton(dialog);
        // 字段间距 gap-7，扣除焦点环边距
        dialog->contentLayout()->setSpacing(kFieldGroupGap - ExampleLayout::kFocusRingWidth);
        dialog->contentLayout()->addLayout(createField([] { return QCoreApplication::translate("DialogPage", "名称"); }, QStringLiteral("Pedro Duarte"), dialog));
        dialog->contentLayout()->addLayout(createField([] { return QCoreApplication::translate("DialogPage", "用户名"); }, QStringLiteral("@peduarte"), dialog));
        ShadButton* cancelButton = createButton([] { return QCoreApplication::translate("DialogPage", "取消"); }, ShadButtonType::Outline, dialog);
        ShadButton* saveButton = createButton([] { return QCoreApplication::translate("DialogPage", "保存修改"); }, ShadButtonType::Default, dialog);
        saveButton->setDefault(true);
        dialog->footerLayout()->addWidget(cancelButton);
        dialog->footerLayout()->addWidget(saveButton);
        QObject::connect(cancelButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        QObject::connect(saveButton, &ShadButton::clicked, dialog, &ShadDialog::accept);
        return dialog;
    }

    // dialog-close-button：页脚关闭按钮靠起始侧排列
    ShadDialog* createShareDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "分享链接"); }, [] { return QCoreApplication::translate("DialogPage", "拿到这个链接的人都可以查看。"); });
        ExampleLayout::addStayOnTopButton(dialog);
        ShadInput* linkInput = new ShadInput(dialog);
        linkInput->setText(QStringLiteral("https://github.com/RainbowCandyX/ShadcnWidgetTools"));
        linkInput->setReadOnly(true);
        dialog->contentLayout()->addWidget(linkInput);
        ShadButton* closeButton = createButton([] { return QCoreApplication::translate("DialogPage", "关闭"); }, ShadButtonType::Default, dialog);
        dialog->footerLayout()->setAlignment(Qt::AlignLeft);
        dialog->footerLayout()->addWidget(closeButton);
        QObject::connect(closeButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        return dialog;
    }

    // 显示全部窗口按钮
    ShadDialog* createAllButtonsDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "可最大化的对话框"); }, [] { return QCoreApplication::translate("DialogPage", "标题栏显示最小化、最大化与关闭按钮，双击标题栏切换最大化，拖动边缘可以缩放。"); });
        ExampleI18n::bind(dialog, [dialog]() { dialog->setWindowTitle(QCoreApplication::translate("DialogPage", "窗口标题")); });
        dialog->setButtonFlags(ShadWindowType::AllButtons);
        ExampleLayout::addStayOnTopButton(dialog);
        return dialog;
    }

    // 不显示窗口按钮
    ShadDialog* createNoButtonsDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "无关闭按钮"); }, [] { return QCoreApplication::translate("DialogPage", "这个对话框的标题栏没有关闭按钮，按 Esc 关闭。"); });
        dialog->setButtonFlags(ShadWindowType::NoButton);
        return dialog;
    }

    // 固定尺寸
    ShadDialog* createFixedDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "固定尺寸"); }, [] { return QCoreApplication::translate("DialogPage", "这个对话框的尺寸固定为 400 × 260，边缘不能拖动缩放。"); });
        dialog->setFixedSize(kFixedDialogWidth, kFixedDialogHeight);
        ExampleLayout::addStayOnTopButton(dialog);
        ShadButton* closeButton = createButton([] { return QCoreApplication::translate("DialogPage", "关闭"); }, ShadButtonType::Outline, dialog);
        dialog->footerLayout()->addWidget(closeButton);
        QObject::connect(closeButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        return dialog;
    }

    // dialog-sticky-footer：内容滚动时页脚保持可见
    ShadDialog* createStickyDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "粘性页脚"); }, [] { return QCoreApplication::translate("DialogPage", "这个对话框的页脚在内容滚动时保持可见。"); });
        ExampleLayout::addStayOnTopButton(dialog);
        dialog->contentLayout()->addWidget(createScrollContent(dialog));
        ShadButton* closeButton = createButton([] { return QCoreApplication::translate("DialogPage", "关闭"); }, ShadButtonType::Outline, dialog);
        dialog->footerLayout()->addWidget(closeButton);
        QObject::connect(closeButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        return dialog;
    }

    // dialog-scrollable-content：标题区固定，内容滚动
    ShadDialog* createScrollDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "可滚动内容"); }, [] { return QCoreApplication::translate("DialogPage", "这是一个内容可以滚动的对话框。"); });
        ExampleLayout::addStayOnTopButton(dialog);
        dialog->contentLayout()->addWidget(createScrollContent(dialog));
        return dialog;
    }

    // 删除确认框，exec 返回后由调用方删除
    ShadDialog* createConfirmDialog(QWidget* parent)
    {
        ShadDialog* dialog = new ShadDialog(parent);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "删除这个项目？"); }, [] { return QCoreApplication::translate("DialogPage", "删除后无法恢复，项目中的所有文件都会被移除。"); });
        ExampleLayout::addStayOnTopButton(dialog);
        ShadButton* cancelButton = createButton([] { return QCoreApplication::translate("DialogPage", "取消"); }, ShadButtonType::Outline, dialog);
        ShadButton* deleteButton = createButton([] { return QCoreApplication::translate("DialogPage", "删除"); }, ShadButtonType::Destructive, dialog);
        dialog->footerLayout()->addWidget(cancelButton);
        dialog->footerLayout()->addWidget(deleteButton);
        QObject::connect(cancelButton, &ShadButton::clicked, dialog, &ShadDialog::reject);
        QObject::connect(deleteButton, &ShadButton::clicked, dialog, &ShadDialog::accept);
        return dialog;
    }

    // 没有父窗口：不绘制遮罩
    ShadDialog* createStandaloneDialog()
    {
        ShadDialog* dialog = new ShadDialog(nullptr);
        bindDialogTexts(dialog, [] { return QCoreApplication::translate("DialogPage", "独立对话框"); }, [] { return QCoreApplication::translate("DialogPage", "这个对话框没有父窗口，因此没有遮罩，按 Esc 或点击按钮关闭。"); });
        ExampleLayout::addStayOnTopButton(dialog);
        ShadButton* centerButton = createButton([] { return QCoreApplication::translate("DialogPage", "移到中央"); }, ShadButtonType::Outline, dialog);
        ShadButton* okButton = createButton([] { return QCoreApplication::translate("DialogPage", "知道了"); }, ShadButtonType::Default, dialog);
        dialog->footerLayout()->addWidget(centerButton);
        dialog->footerLayout()->addWidget(okButton);
        QObject::connect(centerButton, &ShadButton::clicked, dialog, &ShadDialog::moveToCenter);
        QObject::connect(okButton, &ShadButton::clicked, dialog, &ShadDialog::accept);
        return dialog;
    }
} // namespace

DialogPage::DialogPage(QWidget* parent)
    : ExampleBasePage([] { return tr("对话框"); }, [] { return tr("ShadDialog：与 ShadWindow 风格一致的独立对话框窗口，模态打开时显示遮罩。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("设置标题与说明，放入表单和按钮后打开"); });
    ShadButton* profileButton = createButton([] { return tr("打开对话框"); }, ShadButtonType::Outline, this);
    connect(profileButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createProfileDialog(this)); });
    addItem(basicCard, profileButton);

    ShadCard* closeCard = addSection([] { return tr("自定义关闭按钮"); }, [] { return tr("页脚放关闭按钮，并靠起始侧排列"); });
    ShadButton* shareButton = createButton([] { return tr("分享"); }, ShadButtonType::Outline, this);
    connect(shareButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createShareDialog(this)); });
    addItem(closeCard, shareButton);

    // 标题栏按钮
    ShadCard* buttonFlagsCard = addSection([] { return tr("标题栏按钮"); }, [] { return tr("setButtonFlags 设置标题栏的窗口按钮，默认只有关闭按钮"); });
    ShadButton* allButtonsButton = createButton([] { return tr("显示全部按钮"); }, ShadButtonType::Outline, this);
    connect(allButtonsButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createAllButtonsDialog(this)); });
    ShadButton* noButtonsButton = createButton([] { return tr("隐藏全部按钮"); }, ShadButtonType::Outline, this);
    connect(noButtonsButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createNoButtonsDialog(this)); });
    addRow(buttonFlagsCard, {allButtonsButton, noButtonsButton});

    ShadCard* fixedCard = addSection([] { return tr("固定尺寸"); }, [] { return tr("固定尺寸后窗口不能缩放"); });
    ShadButton* fixedButton = createButton([] { return tr("固定尺寸"); }, ShadButtonType::Outline, this);
    connect(fixedButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createFixedDialog(this)); });
    addItem(fixedCard, fixedButton);

    ShadCard* stickyCard = addSection([] { return tr("粘性页脚"); }, [] { return tr("内容区可滚动，页脚始终可见"); });
    ShadButton* stickyButton = createButton([] { return tr("粘性页脚"); }, ShadButtonType::Outline, this);
    connect(stickyButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createStickyDialog(this)); });
    addItem(stickyCard, stickyButton);

    ShadCard* scrollCard = addSection([] { return tr("可滚动内容"); }, [] { return tr("内容较长时只滚动内容区，标题区保持在顶部"); });
    ShadButton* scrollButton = createButton([] { return tr("可滚动内容"); }, ShadButtonType::Outline, this);
    connect(scrollButton, &ShadButton::clicked, this, [this]() { ExampleLayout::openTransientDialog(createScrollDialog(this)); });
    addItem(scrollCard, scrollButton);

    // 继承 ShadDialog 定制自己的对话框
    ShadCard* subclassCard = addSection([] { return tr("继承 ShadDialog"); }, [] { return tr("在子类中填充内容，窗口风格保持统一"); });
    ShadButton* feedbackButton = createButton([] { return tr("发送反馈"); }, ShadButtonType::Outline, this);
    ShadLabel* feedbackResult = createMutedText(this);
    // 按最近一次发送的反馈生成结果文字，切换语言时重新生成
    const std::shared_ptr<FeedbackState> feedbackState = std::make_shared<FeedbackState>();
    const auto updateFeedbackResult = [feedbackResult, feedbackState]()
    {
        if (feedbackState->size < 0)
        {
            feedbackResult->setText(tr("尚未发送反馈"));
            return;
        }
        const QString text = feedbackState->isContactAllowed ? tr("已发送 %1 个字的反馈，允许邮件联系") : tr("已发送 %1 个字的反馈");
        feedbackResult->setText(text.arg(feedbackState->size));
    };
    ExampleI18n::bind(feedbackResult, updateFeedbackResult);
    connect(feedbackButton, &ShadButton::clicked, this, [this, feedbackResult, feedbackState, updateFeedbackResult]()
    {
        FeedbackDialog* dialog = new FeedbackDialog(this);
        connect(dialog, &FeedbackDialog::accepted, feedbackResult, [dialog, feedbackState, updateFeedbackResult]()
        {
            feedbackState->size = static_cast<int>(dialog->getFeedback().size());
            feedbackState->isContactAllowed = dialog->isContactAllowed();
            updateFeedbackResult();
        });
        ExampleLayout::openTransientDialog(dialog);
    });
    addRow(subclassCard, {feedbackButton, feedbackResult});

    // exec() 阻塞等待结果，关闭动画结束后才返回
    ShadCard* execCard = addSection([] { return tr("exec 返回值"); }, [] { return tr("exec() 以模态打开并阻塞，返回 Accepted 或 Rejected"); });
    ShadButton* deleteButton = createButton([] { return tr("删除项目"); }, ShadButtonType::Destructive, this);
    ShadLabel* execResult = createMutedText(this);
    // 最近一次 exec() 的返回值，小于 0 表示尚未操作
    const std::shared_ptr<int> lastExecResult = std::make_shared<int>(-1);
    const auto updateExecResult = [execResult, lastExecResult]()
    {
        if (*lastExecResult < 0)
        {
            execResult->setText(tr("尚未操作"));
            return;
        }
        execResult->setText(*lastExecResult == QDialog::Accepted ? tr("exec() 返回 Accepted：已删除") : tr("exec() 返回 Rejected：已取消"));
    };
    ExampleI18n::bind(execResult, updateExecResult);
    connect(deleteButton, &ShadButton::clicked, this, [this, lastExecResult, updateExecResult]()
    {
        QPointer<ShadDialog> dialog = createConfirmDialog(this);
        const int result = dialog->exec();
        // 对话框打开期间页面被销毁时，对话框已随页面删除
        if (dialog.isNull())
        {
            return;
        }
        delete dialog.data();
        *lastExecResult = result;
        updateExecResult();
    });
    addRow(execCard, {deleteButton, execResult});

    // 没有父窗口的对话框不挡住页面，再次点击只提到最前
    ShadCard* standaloneCard = addSection([] { return tr("无父窗口"); }, [] { return tr("无父窗口时没有遮罩，窗口在屏幕居中"); });
    ShadButton* standaloneButton = createButton([] { return tr("打开独立对话框"); }, ShadButtonType::Outline, this);
    connect(standaloneButton, &ShadButton::clicked, this, [this, standaloneDialog = QPointer<ShadDialog>()]() mutable
    {
        if (!standaloneDialog.isNull())
        {
            standaloneDialog->raise();
            standaloneDialog->activateWindow();
            return;
        }
        standaloneDialog = createStandaloneDialog();
        // 没有父对象，页面销毁时一起释放
        ShadDialog* dialog = standaloneDialog.data();
        connect(this, &QObject::destroyed, dialog, [dialog]() { delete dialog; });
        ExampleLayout::openTransientDialog(dialog);
    });
    addItem(standaloneCard, standaloneButton);
}

DialogPage::~DialogPage()
{
}
