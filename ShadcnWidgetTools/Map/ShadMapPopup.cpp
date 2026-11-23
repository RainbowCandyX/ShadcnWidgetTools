#include "ShadMapPopup.h"

#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSize>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kPaddingX = 12;
    constexpr int kPaddingY = 10;
    constexpr int kSpacing = 2;
    constexpr int kArrowHeight = 6;
    constexpr int kArrowHalfWidth = 6;
    constexpr int kCloseInset = 4;
    constexpr int kMaxWidth = 280;
    constexpr int kTitleFontSize = 14;
    constexpr int kTextFontSize = 13;

    ShadLabel* createLabel(QWidget* parent, int pixelSize, QFont::Weight weight, ShadThemeType::ThemeColor colorRole)
    {
        ShadLabel* label = new ShadLabel(parent);
        QFont font = label->font();
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        label->setFont(font);
        label->setColorRole(colorRole);
        label->setWordWrap(true);
        label->setTextFormat(Qt::PlainText);
        label->hide();
        return label;
    }
} // namespace

ShadMapPopup::ShadMapPopup(QWidget* parent)
    : QWidget(parent)
{
    setMaximumWidth(kMaxWidth);
    setAttribute(Qt::WA_NoMousePropagation, true);
    _layout = new QVBoxLayout(this);
    _layout->setSpacing(kSpacing);
    _titleLabel = createLabel(this, kTitleFontSize, QFont::Medium, ShadThemeType::PopoverForeground);
    _descriptionLabel = createLabel(this, kTextFontSize, QFont::Normal, ShadThemeType::MutedForeground);
    _label = createLabel(this, kTextFontSize, QFont::Normal, ShadThemeType::PopoverForeground);
    _layout->addWidget(_titleLabel);
    _layout->addWidget(_descriptionLabel);
    _layout->addWidget(_label);
    _closeButton = new ShadButton(this);
    _closeButton->setVariant(ShadButtonType::Ghost);
    _closeButton->setButtonSize(ShadButtonType::IconExtraSmall);
    _closeButton->setShadIcon(ShadIconType::X);
    _closeButton->hide();
    connect(_closeButton, &ShadButton::clicked, this, [this]()
    {
        hide();
        Q_EMIT closed();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    retranslate();
    updateMargins();
}

ShadMapPopup::~ShadMapPopup()
{
}

void ShadMapPopup::setCoordinate(const ShadMapCoordinate& coordinate)
{
    if (_coordinate == coordinate)
    {
        return;
    }
    _coordinate = coordinate;
    Q_EMIT changed();
}

ShadMapCoordinate ShadMapPopup::getCoordinate() const
{
    return _coordinate;
}

void ShadMapPopup::setTitle(const QString& title)
{
    updateLabel(_titleLabel, title);
}

QString ShadMapPopup::getTitle() const
{
    return _titleLabel->text();
}

void ShadMapPopup::setDescription(const QString& description)
{
    updateLabel(_descriptionLabel, description);
}

QString ShadMapPopup::getDescription() const
{
    return _descriptionLabel->text();
}

void ShadMapPopup::setText(const QString& text)
{
    updateLabel(_label, text);
}

QString ShadMapPopup::getText() const
{
    return _label->text();
}

void ShadMapPopup::setContentWidget(QWidget* widget)
{
    if (_content == widget)
    {
        return;
    }
    if (!_content.isNull())
    {
        _layout->removeWidget(_content);
        _content->hide();
        _content->deleteLater();
    }
    _content = widget;
    if (widget != nullptr)
    {
        _layout->addWidget(widget);
        widget->show();
    }
    Q_EMIT changed();
}

QWidget* ShadMapPopup::getContentWidget() const
{
    return _content.data();
}

void ShadMapPopup::setCloseButtonVisible(bool visible)
{
    if (isCloseButtonVisible() == visible)
    {
        return;
    }
    _closeButton->setVisible(visible);
    updateMargins();
    Q_EMIT changed();
}

bool ShadMapPopup::isCloseButtonVisible() const
{
    return !_closeButton->isHidden();
}

void ShadMapPopup::setPlacement(ShadPlacementType::Side placement)
{
    if (_placement == placement)
    {
        return;
    }
    _placement = placement;
    updateMargins();
    updateCloseButton();
    update();
    Q_EMIT changed();
}

ShadPlacementType::Side ShadMapPopup::getPlacement() const
{
    return _placement;
}

void ShadMapPopup::setArrowVisible(bool visible)
{
    if (_isArrowVisible == visible)
    {
        return;
    }
    _isArrowVisible = visible;
    updateMargins();
    updateCloseButton();
    update();
    Q_EMIT changed();
}

bool ShadMapPopup::isArrowVisible() const
{
    return _isArrowVisible;
}

void ShadMapPopup::setAnchorOffset(int offset)
{
    if (_anchorOffset == offset)
    {
        return;
    }
    _anchorOffset = offset;
    Q_EMIT changed();
}

int ShadMapPopup::getAnchorOffset() const
{
    return _anchorOffset;
}

void ShadMapPopup::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    else if (event->type() == QEvent::LayoutDirectionChange)
    {
        updateMargins();
        updateCloseButton();
    }
    QWidget::changeEvent(event);
}

void ShadMapPopup::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateCloseButton();
}

void ShadMapPopup::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    Q_EMIT changed();
}

void ShadMapPopup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    const QRectF card = cardRect();
    QPainterPath path;
    path.addRoundedRect(card, radius, radius);
    if (_isArrowVisible)
    {
        const QPointF center = card.center();
        QPainterPath arrow;
        switch (_placement)
        {
            case ShadPlacementType::Bottom:
                arrow.moveTo(center.x() - kArrowHalfWidth, card.top());
                arrow.lineTo(center.x(), card.top() - kArrowHeight);
                arrow.lineTo(center.x() + kArrowHalfWidth, card.top());
                break;
            case ShadPlacementType::Left:
                arrow.moveTo(card.right(), center.y() - kArrowHalfWidth);
                arrow.lineTo(card.right() + kArrowHeight, center.y());
                arrow.lineTo(card.right(), center.y() + kArrowHalfWidth);
                break;
            case ShadPlacementType::Right:
                arrow.moveTo(card.left(), center.y() - kArrowHalfWidth);
                arrow.lineTo(card.left() - kArrowHeight, center.y());
                arrow.lineTo(card.left(), center.y() + kArrowHalfWidth);
                break;
            default:
                arrow.moveTo(center.x() - kArrowHalfWidth, card.bottom());
                arrow.lineTo(center.x(), card.bottom() + kArrowHeight);
                arrow.lineTo(center.x() + kArrowHalfWidth, card.bottom());
                break;
        }
        arrow.closeSubpath();
        path = path.united(arrow);
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
    painter.setBrush(shadTheme->getColor(ShadThemeType::Popover));
    painter.drawPath(path);
}

QRectF ShadMapPopup::cardRect() const
{
    const qreal arrow = _isArrowVisible ? kArrowHeight : 0;
    const QRectF bounds = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    switch (_placement)
    {
        case ShadPlacementType::Bottom:
            return bounds.adjusted(0.0, arrow, 0.0, 0.0);
        case ShadPlacementType::Left:
            return bounds.adjusted(0.0, 0.0, -arrow, 0.0);
        case ShadPlacementType::Right:
            return bounds.adjusted(arrow, 0.0, 0.0, 0.0);
        default:
            return bounds.adjusted(0.0, 0.0, 0.0, -arrow);
    }
}

void ShadMapPopup::updateLabel(ShadLabel* label, const QString& text)
{
    if (label->text() == text)
    {
        return;
    }
    label->setText(text);
    label->setVisible(!text.isEmpty());
    Q_EMIT changed();
}

void ShadMapPopup::updateMargins()
{
    const int arrow = _isArrowVisible ? kArrowHeight : 0;
    const int closeExtent = _closeButton->isHidden() ? 0 : _closeButton->sizeHint().width();
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const int left = kPaddingX + (isRightToLeft ? closeExtent : 0) + (_placement == ShadPlacementType::Right ? arrow : 0);
    const int right = kPaddingX + (isRightToLeft ? 0 : closeExtent) + (_placement == ShadPlacementType::Left ? arrow : 0);
    const int top = kPaddingY + (_placement == ShadPlacementType::Bottom ? arrow : 0);
    const int bottom = kPaddingY + (_placement == ShadPlacementType::Top ? arrow : 0);
    _layout->setContentsMargins(left, top, right, bottom);
}

void ShadMapPopup::updateCloseButton()
{
    const QRectF card = cardRect();
    const QSize buttonSize = _closeButton->sizeHint();
    const int x = layoutDirection() == Qt::RightToLeft ? qRound(card.left()) + kCloseInset : qRound(card.right()) - buttonSize.width() - kCloseInset;
    _closeButton->setGeometry(x, qRound(card.top()) + kCloseInset, buttonSize.width(), buttonSize.height());
}

void ShadMapPopup::retranslate()
{
    _closeButton->setAccessibleName(tr("关闭"));
    _closeButton->setToolTip(tr("关闭"));
}
