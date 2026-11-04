#include "ShadCard.h"

#include <QBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadCardPrivate.h"
#include "ShadLabel.h"
#include "ShadMaxWidthBoxLayout.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // shadow-sm 预留边距，与可聚焦组件的焦点环边距一致，便于对齐
    constexpr int kShadowMargin = ShadStyleHelper::kFocusRingWidth;
    constexpr int kBorderWidth = 1;
    // 内边距与区块间距，对应 py-6 px-6 gap-6
    constexpr int kPadding = 24;
    constexpr int kSectionGap = 24;
    // 标题与描述间距，对应 CardHeader gap-1.5
    constexpr int kHeaderGap = 6;
} // namespace

ShadCard::ShadCard(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCardPrivate())
{
    Q_D(ShadCard);
    d->q_ptr = this;
    const int outerMargin = kShadowMargin + kBorderWidth + kPadding;
    // 卡片设置固定或最大宽度后，自动换行的标题与描述按卡片的实际宽度计算高度
    QBoxLayout* mainLayout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    mainLayout->setContentsMargins(outerMargin, outerMargin, outerMargin, outerMargin);
    mainLayout->setSpacing(kSectionGap);

    // Header：标题 + 描述在左列，操作在右列并跨两行
    d->_headerWidget = new QWidget(this);
    d->_headerLayout = new QGridLayout(d->_headerWidget);
    d->_headerLayout->setContentsMargins(0, 0, 0, 0);
    d->_headerLayout->setHorizontalSpacing(kHeaderGap);
    d->_headerLayout->setVerticalSpacing(kHeaderGap);
    d->_titleLabel = new ShadLabel(d->_headerWidget);
    d->_titleLabel->setColorRole(ShadThemeType::CardForeground);
    ShadStyleHelper::applyFont(d->_titleLabel, 16, QFont::DemiBold);
    d->_titleLabel->setWordWrap(true);
    d->_descriptionLabel = new ShadLabel(d->_headerWidget);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    d->_descriptionLabel->setWordWrap(true);
    d->_headerLayout->addWidget(d->_titleLabel, 0, 0);
    d->_headerLayout->addWidget(d->_descriptionLabel, 1, 0);
    d->_headerLayout->setColumnStretch(0, 1);
    mainLayout->addWidget(d->_headerWidget);

    d->_contentWidget = new QWidget(this);
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(d->_contentWidget);

    d->_footerWidget = new QWidget(this);
    d->_footerLayout = new QHBoxLayout(d->_footerWidget);
    d->_footerLayout->setContentsMargins(0, 0, 0, 0);
    d->_footerWidget->setVisible(false);
    mainLayout->addWidget(d->_footerWidget);

    d->updateHeaderVisibility();
    d->applyThemeColors();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->applyThemeColors(); });
}

ShadCard::ShadCard(const QString& title, const QString& description, QWidget* parent)
    : ShadCard(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadCard::~ShadCard()
{
}

void ShadCard::setTitle(const QString& title)
{
    Q_D(ShadCard);
    d->_titleLabel->setText(title);
    d->updateHeaderVisibility();
}

QString ShadCard::getTitle() const
{
    Q_D(const ShadCard);
    return d->_titleLabel->text();
}

void ShadCard::setDescription(const QString& description)
{
    Q_D(ShadCard);
    d->_descriptionLabel->setText(description);
    d->updateHeaderVisibility();
}

QString ShadCard::getDescription() const
{
    Q_D(const ShadCard);
    return d->_descriptionLabel->text();
}

void ShadCard::setHeaderAction(QWidget* widget)
{
    Q_D(ShadCard);
    if (d->_headerAction == widget)
    {
        return;
    }
    if (d->_headerAction != nullptr)
    {
        d->_headerLayout->removeWidget(d->_headerAction);
        d->_headerAction->deleteLater();
    }
    d->_headerAction = widget;
    if (widget != nullptr)
    {
        d->_headerLayout->addWidget(widget, 0, 1, 2, 1, Qt::AlignRight | Qt::AlignTop);
    }
    d->updateHeaderVisibility();
}

QWidget* ShadCard::getHeaderAction() const
{
    Q_D(const ShadCard);
    return d->_headerAction;
}

QVBoxLayout* ShadCard::contentLayout() const
{
    Q_D(const ShadCard);
    return d->_contentLayout;
}

QHBoxLayout* ShadCard::footerLayout()
{
    Q_D(ShadCard);
    d->_footerWidget->setVisible(true);
    return d->_footerLayout;
}

void ShadCard::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    // 对应 bg-card rounded-xl border shadow-sm
    const QRectF bodyRect = insetRect(rect(), kShadowMargin);
    const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawShadowSm(&painter, bodyRect, radius);
    fillRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Card));
    strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Border));
}
