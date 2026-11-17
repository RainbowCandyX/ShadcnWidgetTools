#include "ShadAutoCompletePopup.h"

#include <QEasingCurve>
#include <QFont>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QScreen>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadPopupEffect.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kShadowMargin = 12;
    constexpr int kSideOffset = 4;
    constexpr int kMinWidth = 128;
    constexpr int kScreenMargin = 8;
    constexpr int kBorderWidth = 1;
    constexpr int kOpenDurationMs = 150;
    constexpr int kSlideDistance = 8;
} // namespace

ShadAutoCompletePopup::ShadAutoCompletePopup(QWidget* anchor)
    : QWidget(anchor, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), _anchor(anchor)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]() { return cardRect(); });

    _scrollArea = new ShadScrollArea(this);
    _scrollArea->setFocusPolicy(Qt::NoFocus);
    _scrollArea->getOverlayScrollBar(Qt::Vertical)->setTrackInset(ShadComboBoxList::kPadding);
    _list = new ShadComboBoxList();
    _list->setCurrentRow(-1);
    _scrollArea->setWidget(_list);

    _openAnimation = new QVariantAnimation(this);
    _openAnimation->setDuration(kOpenDurationMs);
    _openAnimation->setStartValue(0.0);
    _openAnimation->setEndValue(1.0);
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.25, 0.1), QPointF(0.25, 1.0), QPointF(1.0, 1.0));
    _openAnimation->setEasingCurve(easingCurve);
    connect(_openAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _openProgress = value.toReal();
        setWindowOpacity(_openProgress);
        layoutContent();
        update();
    });
    connect(_list, &ShadComboBoxList::itemClicked, this, [this](int index)
    {
        if (_list->isSelectable(index))
        {
            Q_EMIT rowClicked(_list->getItems().at(index).row);
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAutoCompletePopup::~ShadAutoCompletePopup()
{
}

void ShadAutoCompletePopup::setItems(const QList<ShadComboBoxPopupItem>& items)
{
    _list->setItems(items);
    _list->highlightFirst();
    if (isVisible())
    {
        updatePopupGeometry();
    }
    scrollToHighlight();
}

bool ShadAutoCompletePopup::isEmpty() const
{
    return _list->getItems().isEmpty();
}

void ShadAutoCompletePopup::setMaxVisibleItems(int count)
{
    _maxVisibleItems = qMax(1, count);
    if (isVisible())
    {
        updatePopupGeometry();
    }
}

void ShadAutoCompletePopup::open()
{
    setLayoutDirection(_anchor->layoutDirection());
    if (isVisible())
    {
        updatePopupGeometry();
        return;
    }
    _openAnimation->stop();
    _openProgress = 0.0;
    updatePopupGeometry();
    setWindowOpacity(0.0);
    show();
    raise();
    scrollToHighlight();
    _openAnimation->start();
}

void ShadAutoCompletePopup::updatePopupGeometry()
{
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    const QRect localBody = _anchor->rect().adjusted(focusMargin, focusMargin, -focusMargin, -focusMargin);
    const QRect body(_anchor->mapToGlobal(localBody.topLeft()), localBody.size());
    const QScreen* screen = _anchor->screen();
    const QRect available = screen != nullptr ? screen->availableGeometry() : QRect();

    const int chromeHeight = 2 * kBorderWidth;
    const int maxListHeight = _maxVisibleItems * ShadComboBoxList::kItemHeight + 2 * ShadComboBoxList::kPadding;
    int listHeight = qMin(_list->sizeHint().height(), maxListHeight);
    int cardWidth = qMax(body.width(), kMinWidth);
    if (available.isValid())
    {
        cardWidth = qMin(cardWidth, available.width() - 2 * kScreenMargin);
        const int spaceBelow = available.bottom() - kScreenMargin - body.bottom() - kSideOffset;
        const int spaceAbove = body.top() - kSideOffset - available.top() - kScreenMargin;
        if (!isVisible())
        {
            _isAbove = chromeHeight + listHeight > spaceBelow && spaceAbove > spaceBelow;
        }
        const int space = _isAbove ? spaceAbove : spaceBelow;
        listHeight = qMax(qMin(listHeight, space - chromeHeight), ShadComboBoxList::kItemHeight);
    }
    const int cardHeight = chromeHeight + listHeight;
    int cardX = layoutDirection() == Qt::RightToLeft ? body.right() + 1 - cardWidth : body.left();
    if (available.isValid())
    {
        cardX = qBound(available.left() + kScreenMargin, cardX, available.right() + 1 - kScreenMargin - cardWidth);
    }
    const int cardY = _isAbove ? body.top() - kSideOffset - cardHeight : body.bottom() + 1 + kSideOffset;
    setGeometry(cardX - kShadowMargin, cardY - kShadowMargin, cardWidth + 2 * kShadowMargin, cardHeight + 2 * kShadowMargin);
    layoutContent();
    update();
}

void ShadAutoCompletePopup::moveHighlight(int step)
{
    if (step == 0)
    {
        return;
    }
    const int direction = step > 0 ? 1 : -1;
    for (int count = 0; count < qAbs(step); ++count)
    {
        _list->moveHighlight(direction);
    }
    scrollToHighlight();
}

int ShadAutoCompletePopup::highlightedRow() const
{
    const int index = _list->getHighlightIndex();
    return _list->isSelectable(index) ? _list->getItems().at(index).row : -1;
}

QString ShadAutoCompletePopup::highlightedText() const
{
    const int index = _list->getHighlightIndex();
    return _list->isSelectable(index) ? _list->getItems().at(index).text : QString();
}

void ShadAutoCompletePopup::hideEvent(QHideEvent* event)
{
    _openAnimation->stop();
    QWidget::hideEvent(event);
    Q_EMIT popupHidden();
}

void ShadAutoCompletePopup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF card = cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    drawShadowMd(&painter, card, radius);
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
}

void ShadAutoCompletePopup::layoutContent()
{
    _scrollArea->setGeometry(cardRect().adjusted(kBorderWidth, kBorderWidth, -kBorderWidth, -kBorderWidth));
}

void ShadAutoCompletePopup::scrollToHighlight()
{
    const int index = _list->getHighlightIndex();
    if (index < 0)
    {
        return;
    }
    const QRect rowRect = _list->itemRect(index);
    _scrollArea->ensureVisible(rowRect.center().x(), rowRect.center().y(), 0, rowRect.height() / 2 + ShadComboBoxList::kPadding);
}

QRect ShadAutoCompletePopup::cardRect() const
{
    const int slide = qRound((1.0 - _openProgress) * kSlideDistance);
    QRect card = rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    card.translate(0, _isAbove ? slide : -slide);
    return card;
}
