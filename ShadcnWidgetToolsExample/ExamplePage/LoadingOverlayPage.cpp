#include "LoadingOverlayPage.h"

#include <QAbstractButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLoadingOverlay.h"
#include "ShadSkeleton.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kCardWidth = 360;
    constexpr int kLongLoadMs = 2000;
    constexpr int kShortLoadMs = 100;
    constexpr int kDelayMs = 300;
    constexpr int kSkeletonCount = 4;
    constexpr int kSkeletonHeight = 16;

    ShadCard* createTargetCard(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, QWidget* parent)
    {
        ShadCard* card = new ShadCard(parent);
        card->setFixedWidth(kCardWidth);
        ExampleI18n::bind(card, [card, title, description]()
        {
            card->setTitle(title());
            card->setDescription(description());
        });
        return card;
    }

    ShadLoadingOverlay* createOverlay(ShadCard* card)
    {
        ShadLoadingOverlay* overlay = new ShadLoadingOverlay(card);
        overlay->setRadius(shadTheme->getRadius(ShadThemeType::ExtraLarge));
        return overlay;
    }

    void loadFor(ShadLoadingOverlay* overlay, int durationMs)
    {
        overlay->start();
        QTimer::singleShot(durationMs, overlay, &ShadLoadingOverlay::stop);
    }
} // namespace

LoadingOverlayPage::LoadingOverlayPage(QWidget* parent)
    : ExampleBasePage([] { return tr("加载遮罩"); }, [] { return tr("ShadLoadingOverlay：盖在任意组件上的加载遮罩，居中显示转圈与文字，加载期间被盖住的组件不响应操作。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("ShadLoadingOverlay(target) 绑定目标组件，start() 与 stop() 控制显示；点击保存后表单被遮住 2 秒"); });
    ShadCard* formCard = createTargetCard([] { return tr("个人资料"); }, [] { return tr("修改后点击保存"); }, this);
    ShadInput* nameInput = new ShadInput(formCard);
    ExampleI18n::bind(nameInput, [nameInput]() { nameInput->setPlaceholderText(tr("姓名")); });
    ShadInput* emailInput = new ShadInput(formCard);
    ExampleI18n::bind(emailInput, [emailInput]() { emailInput->setPlaceholderText(tr("邮箱")); });
    ShadButton* saveButton = new ShadButton(formCard);
    ExampleI18n::bind(saveButton, [saveButton]() { saveButton->setText(tr("保存")); });
    formCard->contentLayout()->addWidget(nameInput);
    formCard->contentLayout()->addWidget(emailInput);
    formCard->contentLayout()->addWidget(saveButton);
    ShadLoadingOverlay* formOverlay = createOverlay(formCard);
    connect(saveButton, &QAbstractButton::clicked, formOverlay, [formOverlay]() { loadFor(formOverlay, kLongLoadMs); });
    addItem(basicCard, formCard);

    ShadCard* textCard = addSection([] { return tr("文字与延迟"); }, [] { return tr("setText 设置转圈下方的文字；setDelay(300) 在加载超过 300ms 时才显示，避免短暂的加载造成闪烁"); });
    ShadCard* listCard = createTargetCard([] { return tr("数据列表"); }, [] { return tr("加载很快结束时不会出现遮罩"); }, this);
    for (int index = 0; index < kSkeletonCount; ++index)
    {
        ShadSkeleton* skeleton = new ShadSkeleton(listCard);
        skeleton->setFixedHeight(kSkeletonHeight);
        listCard->contentLayout()->addWidget(skeleton);
    }
    ShadLoadingOverlay* listOverlay = createOverlay(listCard);
    listOverlay->setDelay(kDelayMs);
    ExampleI18n::bind(listOverlay, [listOverlay]() { listOverlay->setText(tr("正在加载数据...")); });
    ShadButton* shortButton = new ShadButton(this);
    shortButton->setVariant(ShadButtonType::Outline);
    ExampleI18n::bind(shortButton, [shortButton]() { shortButton->setText(tr("加载 100 毫秒")); });
    connect(shortButton, &QAbstractButton::clicked, listOverlay, [listOverlay]() { loadFor(listOverlay, kShortLoadMs); });
    ShadButton* longButton = new ShadButton(this);
    longButton->setVariant(ShadButtonType::Outline);
    ExampleI18n::bind(longButton, [longButton]() { longButton->setText(tr("加载 2 秒")); });
    connect(longButton, &QAbstractButton::clicked, listOverlay, [listOverlay]() { loadFor(listOverlay, kLongLoadMs); });
    addRow(textCard, {shortButton, longButton});
    addItem(textCard, listCard);
}

LoadingOverlayPage::~LoadingOverlayPage()
{
}
