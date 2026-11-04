#include "ShadPopover.h"

#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QHideEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadLabel.h"
#include "ShadPlacement.h"
#include "ShadPopoverPrivate.h"
#include "ShadPopupEffect.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 对应 animate-in 的 150ms
    constexpr int kOpenDurationMs = 150;
    // slide-in-from-*-2
    constexpr qreal kSlideDistance = 8.0;
    // PopoverHeader 的 gap-1，标题区与内容之间 gap-4
    constexpr int kHeaderGap = 4;
    constexpr int kContentGap = 16;
} // namespace

ShadPopoverPrivate::ShadPopoverPrivate()
{
}

ShadPopoverPrivate::~ShadPopoverPrivate()
{
}

QSize ShadPopoverPrivate::cardSize() const
{
    const int width = _contentWidth;
    const int height = _card->hasHeightForWidth() ? _card->heightForWidth(width) : _card->sizeHint().height();
    return QSize(width, qMax(height, _card->minimumSizeHint().height()));
}

QRect ShadPopoverPrivate::cardRect() const
{
    Q_Q(const ShadPopover);
    QRect card = q->rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    // 从锚点方向滑入：在下方时从上方滑入（slide-in-from-top-2），其余方向同理
    const int slide = qRound((1.0 - _openProgress) * kSlideDistance);
    switch (_actualSide)
    {
        case ShadPlacementType::Top:
            card.translate(0, slide);
            break;
        case ShadPlacementType::Bottom:
            card.translate(0, -slide);
            break;
        case ShadPlacementType::Left:
            card.translate(slide, 0);
            break;
        case ShadPlacementType::Right:
            card.translate(-slide, 0);
            break;
    }
    return card;
}

void ShadPopoverPrivate::reposition()
{
    Q_Q(ShadPopover);
    if (_anchor.isNull())
    {
        return;
    }
    // 本库可聚焦组件四周预留了焦点环边距，按可视边缘对齐
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    const QRect localRect = _anchor->rect().adjusted(focusMargin, focusMargin, -focusMargin, -focusMargin);
    const QRect anchorRect(_anchor->mapToGlobal(localRect.topLeft()), localRect.size());
    const ShadPlacement::Result placement = ShadPlacement::place(anchorRect, cardSize(), _side, _align, kSideOffset, _anchor->layoutDirection());
    _actualSide = placement.side;
    q->setGeometry(placement.rect.adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin));
    layoutCard();
}

void ShadPopoverPrivate::layoutCard()
{
    _card->setGeometry(cardRect());
}

void ShadPopoverPrivate::updateHeader()
{
    const bool hasTitle = !_titleLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    _titleLabel->setVisible(hasTitle);
    _descriptionLabel->setVisible(hasDescription);
    _header->setVisible(hasTitle || hasDescription);
}

void ShadPopoverPrivate::focusFirstChild()
{
    Q_Q(ShadPopover);
    QWidget* candidate = _card->nextInFocusChain();
    while (candidate != nullptr && candidate != _card)
    {
        const bool isTabFocus = (candidate->focusPolicy() & Qt::TabFocus) == Qt::TabFocus;
        if (q->isAncestorOf(candidate) && candidate->isVisibleTo(q) && candidate->isEnabled() && isTabFocus)
        {
            candidate->setFocus(Qt::PopupFocusReason);
            return;
        }
        candidate = candidate->nextInFocusChain();
    }
    // 没有可聚焦的子组件时浮层自身获得焦点，用于响应 Esc
    q->setFocus(Qt::PopupFocusReason);
}

ShadPopover::ShadPopover(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), d_ptr(new ShadPopoverPrivate())
{
    Q_D(ShadPopover);
    d->q_ptr = this;
    // 圆角、描边与阴影自绘，窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]()
    {
        Q_D(ShadPopover);
        return d->cardRect();
    });

    d->_card = new QWidget(this);
    d->_cardLayout = new QVBoxLayout(d->_card);
    const int inset = ShadPopoverPrivate::kBorderWidth + d->_padding;
    d->_cardLayout->setContentsMargins(inset, inset, inset, inset);
    d->_cardLayout->setSpacing(kContentGap);
    d->_header = new QWidget(d->_card);
    QVBoxLayout* headerLayout = new QVBoxLayout(d->_header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(kHeaderGap);
    d->_titleLabel = new ShadLabel(d->_header);
    d->_titleLabel->setColorRole(ShadThemeType::PopoverForeground);
    d->_titleLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_titleLabel, 14, QFont::Medium);
    d->_descriptionLabel = new ShadLabel(d->_header);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    d->_descriptionLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    headerLayout->addWidget(d->_titleLabel);
    headerLayout->addWidget(d->_descriptionLabel);
    d->_cardLayout->addWidget(d->_header);
    d->_contentLayout = new QVBoxLayout();
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(kContentGap);
    d->_cardLayout->addLayout(d->_contentLayout);
    d->updateHeader();
    // 内容尺寸变化（例如多行输入框自动增高）时重新计算浮层高度
    d->_card->installEventFilter(this);

    d->_openAnimation = new QVariantAnimation(this);
    d->_openAnimation->setDuration(kOpenDurationMs);
    d->_openAnimation->setStartValue(0.0);
    d->_openAnimation->setEndValue(1.0);
    d->_openAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_openAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_openProgress = value.toReal();
        setWindowOpacity(d->_openProgress);
        d->layoutCard();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadPopover::~ShadPopover()
{
}

void ShadPopover::setTitle(const QString& title)
{
    Q_D(ShadPopover);
    d->_titleLabel->setText(title);
    d->updateHeader();
}

QString ShadPopover::getTitle() const
{
    Q_D(const ShadPopover);
    return d->_titleLabel->text();
}

void ShadPopover::setDescription(const QString& description)
{
    Q_D(ShadPopover);
    d->_descriptionLabel->setText(description);
    d->updateHeader();
}

QString ShadPopover::getDescription() const
{
    Q_D(const ShadPopover);
    return d->_descriptionLabel->text();
}

void ShadPopover::setContentWidth(int width)
{
    Q_D(ShadPopover);
    d->_contentWidth = qMax(2 * (ShadPopoverPrivate::kBorderWidth + d->_padding), width);
    if (isVisible())
    {
        d->reposition();
    }
}

int ShadPopover::getContentWidth() const
{
    Q_D(const ShadPopover);
    return d->_contentWidth;
}

void ShadPopover::setContentPadding(int padding)
{
    Q_D(ShadPopover);
    const int contentPadding = qMax(0, padding);
    if (d->_padding == contentPadding)
    {
        return;
    }
    d->_padding = contentPadding;
    const int inset = ShadPopoverPrivate::kBorderWidth + contentPadding;
    d->_cardLayout->setContentsMargins(inset, inset, inset, inset);
    d->_contentWidth = qMax(2 * inset, d->_contentWidth);
    if (isVisible())
    {
        d->reposition();
    }
}

int ShadPopover::getContentPadding() const
{
    Q_D(const ShadPopover);
    return d->_padding;
}

QVBoxLayout* ShadPopover::contentLayout() const
{
    Q_D(const ShadPopover);
    return d->_contentLayout;
}

void ShadPopover::popup(QWidget* anchor, ShadPlacementType::Side side, ShadPlacementType::Align align)
{
    Q_D(ShadPopover);
    if (anchor == nullptr)
    {
        return;
    }
    d->_anchor = anchor;
    d->_side = side;
    d->_align = align;
    // 弹出窗口不会继承锚点的布局方向，这里显式同步
    setLayoutDirection(anchor->layoutDirection());
    d->_openAnimation->stop();
    d->_openProgress = 0.0;
    setWindowOpacity(0.0);
    d->reposition();
    show();
    raise();
    d->focusFirstChild();
    d->_openAnimation->start();
}

void ShadPopover::toggle(QWidget* anchor, ShadPlacementType::Side side, ShadPlacementType::Align align)
{
    if (isVisible())
    {
        hide();
        return;
    }
    popup(anchor, side, align);
}

QSize ShadPopover::sizeHint() const
{
    Q_D(const ShadPopover);
    return d->cardSize() + QSize(2 * ShadPopoverPrivate::kShadowMargin, 2 * ShadPopoverPrivate::kShadowMargin);
}

bool ShadPopover::event(QEvent* event)
{
    Q_D(ShadPopover);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    else if (event->type() == QEvent::FontChange && isVisible())
    {
        d->reposition();
    }
    return QWidget::event(event);
}

bool ShadPopover::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadPopover);
    if (watched == d->_card && event->type() == QEvent::LayoutRequest && isVisible())
    {
        // 等卡片布局处理完这次请求后再按新高度定位
        QMetaObject::invokeMethod(this, [d]() { d->reposition(); }, Qt::QueuedConnection);
    }
    return QWidget::eventFilter(watched, event);
}

void ShadPopover::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    Q_EMIT aboutToShow();
}

void ShadPopover::hideEvent(QHideEvent* event)
{
    Q_D(ShadPopover);
    d->_openAnimation->stop();
    Q_EMIT aboutToHide();
    QWidget::hideEvent(event);
}

void ShadPopover::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadPopover);
    if (!d->cardRect().contains(event->position().toPoint()))
    {
        // 点击 anchor 关闭浮层时不再把这次按下转发给 anchor，否则会立刻重新打开
        if (!d->_anchor.isNull())
        {
            const QPoint anchorPos = d->_anchor->mapFromGlobal(event->globalPosition().toPoint());
            if (d->_anchor->rect().contains(anchorPos))
            {
                setAttribute(Qt::WA_NoMouseReplay);
            }
        }
        hide();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadPopover::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadPopover);
    using namespace ShadStyleHelper;
    const QRectF card = d->cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // bg-popover border shadow-md
    drawShadowMd(&painter, card, radius);
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
}
