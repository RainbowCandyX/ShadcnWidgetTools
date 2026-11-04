#include "SpinnerPage.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <initializer_list>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ExampleMutedItem.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadLabel.h"
#include "ShadSpinner.h"
#include "ShadTextarea.h"
#include "ShadTheme.h"

namespace
{
    // 输入组 max-w-md
    constexpr int kInputGroupWidth = 448;
    // EmptyMedia variant="icon"：size-10 rounded-lg bg-muted
    constexpr int kEmptyMediaSize = 40;
    // 尺寸示例 gap-6
    constexpr int kSizeGap = 24;

    // 空状态的图标框：bg-muted rounded-lg，中间放一个 Spinner
    class EmptyMedia : public QWidget
    {
        public:
            explicit EmptyMedia(QWidget* parent = nullptr)
                : QWidget(parent)
            {
                setFixedSize(kEmptyMediaSize, kEmptyMediaSize);
                QHBoxLayout* layout = new QHBoxLayout(this);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->addWidget(new ShadSpinner(this), 0, Qt::AlignCenter);
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(Qt::NoPen);
                painter.setBrush(shadTheme->getColor(ShadThemeType::Muted));
                painter.drawRoundedRect(QRectF(rect()), radius, radius);
            }
    };

    ShadLabel* createLabel(ExampleI18n::TextFunction text, int pixelSize, QFont::Weight weight, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        QFont font = label->font();
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        label->setFont(font);
        return label;
    }

    // 禁用并显示加载状态的小号按钮
    ShadButton* createLoadingButton(ExampleI18n::TextFunction text, ShadButtonType::Variant variant, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), variant, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        button->setButtonSize(ShadButtonType::Small);
        button->setLoading(true);
        button->setEnabled(false);
        return button;
    }

    ShadBadge* createLoadingBadge(ExampleI18n::TextFunction text, ShadBadgeType::Variant variant, QWidget* parent)
    {
        ShadBadge* badge = new ShadBadge(QString(), variant, parent);
        ExampleI18n::bind(badge, [badge, text]() { badge->setText(text()); });
        badge->setShadIcon(ShadIconType::LoaderCircle);
        return badge;
    }
} // namespace

SpinnerPage::SpinnerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("加载指示"); }, [] { return tr("ShadSpinner：表示正在加载的旋转图标，按钮、徽章与输入组中同样可用。"); }, parent)
{
    using Position = ShadInputGroupType::AddonPosition;

    // spinner-demo：条目中的加载状态
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("匀速旋转，隐藏时暂停，显示后继续"); });
    ExampleMutedItem* paymentItem = new ExampleMutedItem(this);
    ShadLabel* paymentTitle = createLabel([] { return tr("正在处理付款..."); }, 14, QFont::Medium, paymentItem);
    ShadLabel* paymentAmount = new ShadLabel(QStringLiteral("¥100.00"), paymentItem);
    paymentItem->itemLayout()->addWidget(new ShadSpinner(paymentItem));
    paymentItem->itemLayout()->addWidget(paymentTitle, 1);
    paymentItem->itemLayout()->addWidget(paymentAmount);
    addStretchItem(basicCard, paymentItem);

    // spinner-size：size-3 / size-4 / size-6 / size-8
    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("setSpinnerSize：设置不同的图标尺寸"); });
    QWidget* sizeRow = new QWidget(this);
    QHBoxLayout* sizeLayout = new QHBoxLayout(sizeRow);
    sizeLayout->setContentsMargins(0, 0, 0, 0);
    sizeLayout->setSpacing(kSizeGap);
    for (const int size : {12, 16, 24, 32})
    {
        ShadSpinner* spinner = new ShadSpinner(sizeRow);
        spinner->setSpinnerSize(size);
        sizeLayout->addWidget(spinner, 0, Qt::AlignVCenter);
    }
    sizeLayout->addStretch(1);
    addItem(sizeCard, sizeRow);

    // spinner-custom：换成其他图标，并演示颜色与启停
    ShadCard* customCard = addSection([] { return tr("自定义"); }, [] { return tr("更换图标与颜色，或停止旋转"); });
    ShadSpinner* loaderSpinner = new ShadSpinner(this);
    loaderSpinner->setShadIcon(ShadIconType::Loader);
    ShadSpinner* mutedSpinner = new ShadSpinner(this);
    mutedSpinner->setColorRole(ShadThemeType::MutedForeground);
    ShadSpinner* destructiveSpinner = new ShadSpinner(this);
    destructiveSpinner->setColorRole(ShadThemeType::Destructive);
    ShadSpinner* customColorSpinner = new ShadSpinner(this);
    customColorSpinner->setColor(QColor(59, 130, 246));
    ShadSpinner* toggleSpinner = new ShadSpinner(this);
    ShadButton* toggleButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    toggleButton->setButtonSize(ShadButtonType::Small);
    const auto updateToggleButton = [toggleSpinner, toggleButton]() { toggleButton->setText(toggleSpinner->isRunning() ? tr("暂停") : tr("继续")); };
    connect(toggleButton, &ShadButton::clicked, toggleSpinner, [toggleSpinner, updateToggleButton]()
    {
        toggleSpinner->setRunning(!toggleSpinner->isRunning());
        updateToggleButton();
    });
    ExampleI18n::bind(toggleButton, updateToggleButton);
    addRow(customCard, {loaderSpinner, mutedSpinner, destructiveSpinner, customColorSpinner, toggleSpinner, toggleButton});

    // spinner-button：禁用按钮中的加载图标
    ShadCard* buttonCard = addSection([] { return tr("按钮"); }, [] { return tr("setLoading：在按钮图标位置显示加载动画"); });
    addRow(buttonCard, {createLoadingButton([] { return tr("加载中..."); }, ShadButtonType::Default, this), createLoadingButton([] { return tr("请稍候"); }, ShadButtonType::Outline, this), createLoadingButton([] { return tr("处理中"); }, ShadButtonType::Secondary, this)});

    // spinner-badge：徽章中的加载图标
    ShadCard* badgeCard = addSection([] { return tr("徽章"); }, [] { return tr("徽章中的加载图标在可见时自动旋转"); });
    addRow(badgeCard, {createLoadingBadge([] { return tr("同步中"); }, ShadBadgeType::Default, this), createLoadingBadge([] { return tr("更新中"); }, ShadBadgeType::Secondary, this), createLoadingBadge([] { return tr("处理中"); }, ShadBadgeType::Outline, this)});

    // spinner-input-group：输入组附加区中的加载指示
    ShadCard* inputGroupCard = addSection([] { return tr("输入组"); }, [] { return tr("把 Spinner 放进输入组的附加区"); });
    QWidget* inputGroupColumn = new QWidget(this);
    QVBoxLayout* inputGroupLayout = new QVBoxLayout(inputGroupColumn);
    inputGroupLayout->setContentsMargins(0, 0, 0, 0);
    inputGroupLayout->setSpacing(16 - 2 * ExampleLayout::kFocusRingWidth);
    ShadInputGroup* messageGroup = new ShadInputGroup(inputGroupColumn);
    messageGroup->setFixedWidth(kInputGroupWidth);
    ExampleI18n::bind(messageGroup, [messageGroup]() { messageGroup->getInput()->setPlaceholderText(tr("发送消息...")); });
    messageGroup->getInput()->setEnabled(false);
    ShadSpinner* messageSpinner = new ShadSpinner(messageGroup);
    messageSpinner->setColorRole(ShadThemeType::MutedForeground);
    messageGroup->addWidget(Position::InlineEnd, messageSpinner);
    ShadInputGroup* validateGroup = new ShadInputGroup(ShadInputGroupType::PlainText, inputGroupColumn);
    validateGroup->setFixedWidth(kInputGroupWidth);
    ExampleI18n::bind(validateGroup, [validateGroup]() { validateGroup->getTextarea()->setPlaceholderText(tr("发送消息...")); });
    validateGroup->getTextarea()->setEnabled(false);
    ShadSpinner* validateSpinner = new ShadSpinner(validateGroup);
    validateSpinner->setColorRole(ShadThemeType::MutedForeground);
    validateGroup->addWidget(Position::BlockEnd, validateSpinner);
    ShadLabel* validateText = validateGroup->addText(Position::BlockEnd, QString());
    ExampleI18n::bind(validateText, [validateText]() { validateText->setText(tr("正在验证...")); });
    validateGroup->addStretch(Position::BlockEnd);
    ShadButton* sendButton = validateGroup->addButton(Position::BlockEnd, QString(), ShadIconType::ArrowUp, ShadButtonType::IconExtraSmall);
    sendButton->setVariant(ShadButtonType::Default);
    ExampleI18n::bind(sendButton, [sendButton]() { sendButton->setToolTip(tr("发送")); });
    inputGroupLayout->addWidget(messageGroup);
    inputGroupLayout->addWidget(validateGroup);
    addItem(inputGroupCard, inputGroupColumn);

    // spinner-empty：空状态中的加载指示
    ShadCard* emptyCard = addSection([] { return tr("空状态"); }, [] { return tr("用现有组件组合出加载中的空状态"); });
    // 空状态区域占满卡片宽度，内容水平居中
    QWidget* emptyArea = new QWidget(this);
    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyArea);
    emptyLayout->setContentsMargins(0, 24, 0, 24);
    emptyLayout->setSpacing(8);
    ShadLabel* emptyTitle = createLabel([] { return tr("正在处理你的请求"); }, 18, QFont::Medium, emptyArea);
    emptyTitle->setAlignment(Qt::AlignHCenter);
    ShadLabel* emptyDescription = createLabel([] { return tr("请稍候，我们正在处理你的请求，请不要刷新页面。"); }, 14, QFont::Normal, emptyArea);
    emptyDescription->setColorRole(ShadThemeType::MutedForeground);
    emptyDescription->setAlignment(Qt::AlignHCenter);
    emptyDescription->setWordWrap(true);
    ShadButton* cancelButton = new ShadButton(QString(), ShadButtonType::Outline, emptyArea);
    ExampleI18n::bind(cancelButton, [cancelButton]() { cancelButton->setText(tr("取消")); });
    cancelButton->setButtonSize(ShadButtonType::Small);
    emptyLayout->addWidget(new EmptyMedia(emptyArea), 0, Qt::AlignHCenter);
    emptyLayout->addSpacing(8);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyDescription);
    emptyLayout->addSpacing(16 - ExampleLayout::kFocusRingWidth);
    emptyLayout->addWidget(cancelButton, 0, Qt::AlignHCenter);
    addStretchItem(emptyCard, emptyArea);
}

SpinnerPage::~SpinnerPage()
{
}
