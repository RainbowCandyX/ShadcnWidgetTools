#include "BadgePage.h"

#include <QString>

#include "ExampleI18n.h"
#include "ShadBadge.h"
#include "ShadCard.h"
#include "ShadIconType.h"

BadgePage::BadgePage(QWidget* parent)
    : ExampleBasePage([] { return tr("徽章"); }, [] { return tr("ShadBadge：用于展示状态或计数的徽章。"); }, parent)
{
    ShadCard* variantCard = addSection([] { return tr("变体"); }, [] { return tr("默认、次要、危险与描边四种变体"); });
    ShadBadge* defaultBadge = new ShadBadge(QString(), ShadBadgeType::Default, this);
    ShadBadge* secondaryBadge = new ShadBadge(QString(), ShadBadgeType::Secondary, this);
    ShadBadge* destructiveBadge = new ShadBadge(QString(), ShadBadgeType::Destructive, this);
    ShadBadge* outlineBadge = new ShadBadge(QString(), ShadBadgeType::Outline, this);
    ExampleI18n::bind(this, [defaultBadge, secondaryBadge, destructiveBadge, outlineBadge]()
    {
        defaultBadge->setText(tr("徽章"));
        secondaryBadge->setText(tr("次要"));
        destructiveBadge->setText(tr("危险"));
        outlineBadge->setText(tr("描边"));
    });
    addRow(variantCard, {defaultBadge, secondaryBadge, destructiveBadge, outlineBadge});

    ShadCard* iconCard = addSection([] { return tr("带图标"); }, [] { return tr("图标与文字组合的徽章"); });
    ShadBadge* verifiedBadge = new ShadBadge(QString(), ShadBadgeType::Secondary, this);
    verifiedBadge->setShadIcon(ShadIconType::BadgeCheck);
    ShadBadge* tagBadge = new ShadBadge(QString(), ShadBadgeType::Outline, this);
    tagBadge->setShadIcon(ShadIconType::Tag);
    ShadBadge* alertBadge = new ShadBadge(QString(), ShadBadgeType::Destructive, this);
    alertBadge->setShadIcon(ShadIconType::Info);
    ExampleI18n::bind(this, [verifiedBadge, tagBadge, alertBadge]()
    {
        verifiedBadge->setText(tr("已认证"));
        tagBadge->setText(tr("标签"));
        alertBadge->setText(tr("警告"));
    });
    addRow(iconCard, {verifiedBadge, tagBadge, alertBadge});

    ShadCard* countCard = addSection([] { return tr("计数"); }, [] { return tr("数字徽章"); });
    addRow(countCard, {new ShadBadge(QStringLiteral("8"), ShadBadgeType::Default, this), new ShadBadge(QStringLiteral("99"), ShadBadgeType::Destructive, this), new ShadBadge(QStringLiteral("20+"), ShadBadgeType::Outline, this)});
}

BadgePage::~BadgePage()
{
}
