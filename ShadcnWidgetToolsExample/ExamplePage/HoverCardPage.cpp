#include "HoverCardPage.h"

#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadAvatar.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadHoverCard.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadLabel.h"

namespace
{
    // w-80
    constexpr int kProfileCardWidth = 320;
    constexpr int kHintCardWidth = 200;

    // 文字随语言刷新的标签
    ShadLabel* createLabel(ExampleI18n::TextFunction text, int pixelSize, QFont::Weight weight, ShadThemeType::ThemeColor colorRole, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        QFont labelFont = label->font();
        labelFont.setPixelSize(pixelSize);
        labelFont.setWeight(weight);
        label->setFont(labelFont);
        label->setColorRole(colorRole);
        label->setWordWrap(true);
        return label;
    }

    QWidget* createProfile(QWidget* parent)
    {
        QWidget* profile = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(profile);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(16);
        ShadAvatar* avatar = new ShadAvatar(profile);
        avatar->setFallbackText(QStringLiteral("VC"));
        layout->addWidget(avatar, 0, Qt::AlignTop);

        QVBoxLayout* textLayout = new QVBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(4);
        textLayout->addWidget(createLabel([] { return QStringLiteral("@nextjs"); }, 14, QFont::DemiBold, ShadThemeType::PopoverForeground, profile));
        textLayout->addWidget(createLabel([] { return QCoreApplication::translate("HoverCardPage", "React 框架，由 @vercel 创建并维护。"); }, 14, QFont::Normal, ShadThemeType::PopoverForeground, profile));
        QHBoxLayout* joinedLayout = new QHBoxLayout();
        joinedLayout->setContentsMargins(0, 4, 0, 0);
        joinedLayout->setSpacing(8);
        ShadLabel* calendarIcon = new ShadLabel(ShadIcon::glyph(ShadIconType::CalendarDays), profile);
        calendarIcon->setFont(ShadIcon::font(16));
        calendarIcon->setColorRole(ShadThemeType::MutedForeground);
        joinedLayout->addWidget(calendarIcon);
        ShadLabel* joinedLabel = createLabel([] { return QCoreApplication::translate("HoverCardPage", "2021 年 12 月加入"); }, 12, QFont::Normal, ShadThemeType::MutedForeground, profile);
        joinedLabel->setWordWrap(false);
        joinedLayout->addWidget(joinedLabel, 1);
        textLayout->addLayout(joinedLayout);
        layout->addLayout(textLayout, 1);
        return profile;
    }
} // namespace

HoverCardPage::HoverCardPage(QWidget* parent)
    : ExampleBasePage([] { return tr("悬停卡片"); }, [] { return tr("ShadHoverCard：悬停时显示的卡片。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("悬停后显示"); });
    ShadButton* profileButton = new ShadButton(QStringLiteral("@nextjs"), ShadButtonType::Link, this);
    ShadHoverCard* profileCard = new ShadHoverCard(this);
    profileCard->setContentWidth(kProfileCardWidth);
    profileCard->contentLayout()->addWidget(createProfile(profileCard));
    profileCard->setTrigger(profileButton);
    addRow(basicCard, {profileButton});

    ShadCard* sideCard = addSection([] { return tr("方向"); }, [] { return tr("四个方向"); });
    struct SideOption
    {
        ExampleI18n::TextFunction text;
        ShadPlacementType::Side side;
    };
    const QList<SideOption> options{
        {[] { return tr("上方"); }, ShadPlacementType::Top},
        {[] { return tr("右侧"); }, ShadPlacementType::Right},
        {[] { return tr("下方"); }, ShadPlacementType::Bottom},
        {[] { return tr("左侧"); }, ShadPlacementType::Left},
    };
    QList<QWidget*> sideButtons;
    for (const SideOption& option : options)
    {
        const ExampleI18n::TextFunction text = option.text;
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        ShadHoverCard* card = new ShadHoverCard(this);
        card->setContentWidth(kHintCardWidth);
        card->setSide(option.side);
        card->setOpenDelay(200);
        card->contentLayout()->addWidget(createLabel([text] { return tr("显示在%1").arg(text()); }, 14, QFont::Normal, ShadThemeType::PopoverForeground, card));
        card->setTrigger(button);
        sideButtons.append(button);
    }
    addRow(sideCard, sideButtons);
}

HoverCardPage::~HoverCardPage()
{
}
