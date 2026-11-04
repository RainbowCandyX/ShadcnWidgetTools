#include "ShadEmpty.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadEmptyPrivate.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kTitleFontSize = 18;
    constexpr qreal kTitleTracking = -0.025;
    constexpr int kTextFontSize = 14;
    constexpr int kContentGap = 16;
    constexpr int kIconSize = 24;
    // 最小宽度下至少留给文字的宽度
    constexpr int kMinTextWidth = 64;
} // namespace

ShadEmpty::ShadEmpty(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadEmptyPrivate())
{
    Q_D(ShadEmpty);
    d->q_ptr = this;
    // 横向占满可用空间，高度随宽度变化
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    d->_titleLabel = new QLabel(this);
    d->_titleLabel->setTextFormat(Qt::PlainText);
    d->_titleLabel->setWordWrap(true);
    d->_titleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    ShadStyleHelper::applyFont(d->_titleLabel, kTitleFontSize, QFont::Medium);
    QFont titleFont = d->_titleLabel->font();
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, kTitleFontSize * kTitleTracking);
    d->_titleLabel->setFont(titleFont);
    // 链接不自动打开，交给使用者处理
    d->_descriptionLabel = new QLabel(this);
    d->_descriptionLabel->setTextFormat(Qt::AutoText);
    d->_descriptionLabel->setWordWrap(true);
    d->_descriptionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    d->_descriptionLabel->setOpenExternalLinks(false);
    d->_descriptionLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    d->_descriptionLabel->installEventFilter(this);
    ShadStyleHelper::applyFont(d->_descriptionLabel, kTextFontSize, QFont::Normal);
    connect(d->_descriptionLabel, &QLabel::linkActivated, this, &ShadEmpty::linkActivated);
    connect(d->_descriptionLabel, &QLabel::linkHovered, this, [d](const QString& link) { d->setLinkHovered(!link.isEmpty()); });
    d->_contentWidget = new QWidget(this);
    ShadStyleHelper::applyFont(d->_contentWidget, kTextFontSize, QFont::Normal);
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(kContentGap);
    d->updateColors();
    d->updateContent();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->updateColors(); });
}

ShadEmpty::ShadEmpty(ShadIconType::IconName icon, const QString& title, const QString& description, QWidget* parent)
    : ShadEmpty(parent)
{
    Q_D(ShadEmpty);
    d->_icon = icon;
    d->_titleLabel->setText(title);
    d->_descriptionLabel->setText(description);
    d->updateContent();
}

ShadEmpty::~ShadEmpty()
{
    Q_D(ShadEmpty);
    // 私有数据先于子组件释放，需提前断开依赖它的连接和事件过滤器
    QObject::disconnect(d->_descriptionLabel, nullptr, this, nullptr);
    d->_descriptionLabel->removeEventFilter(this);
}

void ShadEmpty::setMediaVariant(ShadEmptyType::MediaVariant variant)
{
    Q_D(ShadEmpty);
    if (d->_mediaVariant == variant)
    {
        return;
    }
    d->_mediaVariant = variant;
    d->updateContent();
}

ShadEmptyType::MediaVariant ShadEmpty::getMediaVariant() const
{
    Q_D(const ShadEmpty);
    return d->_mediaVariant;
}

void ShadEmpty::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadEmpty);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateContent();
}

ShadIconType::IconName ShadEmpty::getShadIcon() const
{
    Q_D(const ShadEmpty);
    return d->_icon;
}

void ShadEmpty::setMediaWidget(QWidget* widget)
{
    Q_D(ShadEmpty);
    if (d->_mediaWidget == widget)
    {
        return;
    }
    if (!d->_mediaWidget.isNull())
    {
        ShadStyleHelper::clearFocusWithin(d->_mediaWidget);
        d->_mediaWidget->hide();
        d->_mediaWidget->deleteLater();
    }
    d->_mediaWidget = widget;
    if (widget != nullptr)
    {
        // 媒体组件不参与布局，由本组件手动定位
        widget->setParent(this);
        widget->show();
    }
    d->updateContent();
}

QWidget* ShadEmpty::getMediaWidget() const
{
    Q_D(const ShadEmpty);
    return d->_mediaWidget.data();
}

void ShadEmpty::setTitle(const QString& title)
{
    Q_D(ShadEmpty);
    d->_titleLabel->setText(title);
    d->updateContent();
}

QString ShadEmpty::getTitle() const
{
    Q_D(const ShadEmpty);
    return d->_titleLabel->text();
}

void ShadEmpty::setDescription(const QString& description)
{
    Q_D(ShadEmpty);
    d->_descriptionLabel->setText(description);
    d->setLinkHovered(false);
    d->updateContent();
}

QString ShadEmpty::getDescription() const
{
    Q_D(const ShadEmpty);
    return d->_descriptionLabel->text();
}

void ShadEmpty::setBorderVisible(bool visible)
{
    Q_D(ShadEmpty);
    if (d->_isBorderVisible == visible)
    {
        return;
    }
    d->_isBorderVisible = visible;
    d->updateContent();
}

bool ShadEmpty::isBorderVisible() const
{
    Q_D(const ShadEmpty);
    return d->_isBorderVisible;
}

QVBoxLayout* ShadEmpty::contentLayout() const
{
    Q_D(const ShadEmpty);
    return d->_contentLayout;
}

bool ShadEmpty::hasHeightForWidth() const
{
    return true;
}

int ShadEmpty::heightForWidth(int width) const
{
    Q_D(const ShadEmpty);
    // 与实际排版宽度保持一致
    const int emptyWidth = qMin(width, maximumWidth());
    return qMax(d->computeLayout(emptyWidth).totalHeight, minimumHeight());
}

QSize ShadEmpty::sizeHint() const
{
    Q_D(const ShadEmpty);
    // 理想宽度取各部分的自然宽度
    int naturalWidth = 0;
    const QSize media = d->mediaSize();
    if (!media.isEmpty())
    {
        naturalWidth = media.width();
    }
    if (d->hasTitle())
    {
        naturalWidth = qMax(naturalWidth, ShadStyleHelper::textWidth(d->_titleLabel->font(), d->_titleLabel->text()));
    }
    if (d->hasDescription())
    {
        naturalWidth = qMax(naturalWidth, d->_descriptionLabel->sizeHint().width());
    }
    if (d->hasContent())
    {
        naturalWidth = qMax(naturalWidth, d->_contentWidget->sizeHint().width());
    }
    const int width = qMin(2 * d->padding() + qMin(naturalWidth, ShadEmptyPrivate::kMaxContentWidth), maximumWidth());
    return QSize(width, heightForWidth(width));
}

QSize ShadEmpty::minimumSizeHint() const
{
    Q_D(const ShadEmpty);
    // 高度由 heightForWidth 决定，这里只约束最小宽度
    int minimumWidth = kMinTextWidth;
    const QSize media = d->mediaSize();
    if (!media.isEmpty())
    {
        minimumWidth = qMax(minimumWidth, media.width());
    }
    if (d->hasContent())
    {
        minimumWidth = qMax(minimumWidth, d->_contentWidget->minimumSizeHint().width());
    }
    const int pad = d->padding();
    return QSize(2 * pad + minimumWidth, qMax(2 * pad, minimumHeight()));
}

bool ShadEmpty::event(QEvent* event)
{
    Q_D(ShadEmpty);
    if (event->type() == QEvent::LayoutRequest)
    {
        // 媒体组件或操作区的尺寸变化
        d->layoutChildren();
        updateGeometry();
    }
    else if (event->type() == QEvent::Show)
    {
        // 隐藏期间内容可能变化，显示时重新排版
        d->layoutChildren();
    }
    return QWidget::event(event);
}

bool ShadEmpty::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadEmpty);
    // 直接移出描述时不一定发出 linkHovered，需手动恢复链接颜色
    if (watched == d->_descriptionLabel && event->type() == QEvent::Leave)
    {
        d->setLinkHovered(false);
    }
    return QWidget::eventFilter(watched, event);
}

void ShadEmpty::changeEvent(QEvent* event)
{
    Q_D(ShadEmpty);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutChildren();
    }
    QWidget::changeEvent(event);
}

void ShadEmpty::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadEmpty);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadEmpty::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadEmpty);
    using namespace ShadStyleHelper;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (d->_isBorderVisible)
    {
        strokeDashedRoundedRect(&painter, QRectF(rect()), radius, shadTheme->getColor(ShadThemeType::Border));
    }
    if (!d->_mediaWidget.isNull() || d->_icon == ShadIconType::None || d->_mediaRect.isEmpty())
    {
        return;
    }
    const QRectF mediaRect(d->_mediaRect);
    if (d->_mediaVariant == ShadEmptyType::Icon)
    {
        fillRoundedRect(&painter, mediaRect, radius, shadTheme->getColor(ShadThemeType::Muted));
    }
    ShadIcon::paint(&painter, mediaRect, d->_icon, shadTheme->getColor(ShadThemeType::Foreground), kIconSize);
}
