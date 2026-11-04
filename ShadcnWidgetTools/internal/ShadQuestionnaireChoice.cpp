#include "ShadQuestionnaireChoice.h"

#include <QAbstractButton>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <climits>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kPaddingX = 16;
    constexpr int kPaddingY = 14;
    constexpr int kGap = 12;
    constexpr int kContentGap = 4;
    constexpr int kMinCardHeight = 44;
    constexpr int kIndicatorSize = 16;
    constexpr int kIndicatorOffset = 2;
    constexpr int kDotSize = 8;
    constexpr int kShortcutSize = 20;
    constexpr int kShortcutFontSize = 10;
    constexpr int kMinTextWidth = 40;
    constexpr qreal kDarkBackgroundAlpha = 0.2;
    constexpr qreal kHoverAlpha = 0.5;
    constexpr qreal kCheckedBorderAlpha = 0.4;
    constexpr qreal kDarkIndicatorAlpha = 0.3;

    int wrappedHeight(const QFont& font, const QString& text, int width)
    {
        if (text.isEmpty())
        {
            return 0;
        }
        return QFontMetrics(font).boundingRect(QRect(0, 0, qMax(1, width), INT_MAX), Qt::TextWordWrap, text).height();
    }
} // namespace

ShadQuestionnaireChoice::ShadQuestionnaireChoice(QWidget* parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    _hoverTransition = new ShadTransition(this);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadQuestionnaireChoice::~ShadQuestionnaireChoice()
{
}

void ShadQuestionnaireChoice::setLabel(const QString& label)
{
    _label = label;
    setAccessibleName(label);
    updateGeometry();
    update();
}

QString ShadQuestionnaireChoice::getLabel() const
{
    return _label;
}

void ShadQuestionnaireChoice::setDescription(const QString& description)
{
    _description = description;
    setAccessibleDescription(description);
    updateGeometry();
    update();
}

QString ShadQuestionnaireChoice::getDescription() const
{
    return _description;
}

void ShadQuestionnaireChoice::setRadio(bool radio)
{
    _isRadio = radio;
    update();
}

bool ShadQuestionnaireChoice::isRadio() const
{
    return _isRadio;
}

void ShadQuestionnaireChoice::setShortcutText(const QString& text)
{
    if (_shortcutText == text)
    {
        return;
    }
    _shortcutText = text;
    updateGeometry();
    update();
}

QString ShadQuestionnaireChoice::getShortcutText() const
{
    return _shortcutText;
}

void ShadQuestionnaireChoice::setInvalid(bool invalid)
{
    if (_isInvalid == invalid)
    {
        return;
    }
    _isInvalid = invalid;
    update();
}

bool ShadQuestionnaireChoice::isInvalid() const
{
    return _isInvalid;
}

QSize ShadQuestionnaireChoice::sizeHint() const
{
    const QFontMetrics labelMetrics(labelFont());
    const QFontMetrics descriptionMetrics(font());
    const int contentWidth = qMax(labelMetrics.horizontalAdvance(_label), descriptionMetrics.horizontalAdvance(_description));
    const int width = chromeWidth() + contentWidth;
    return QSize(width, heightForWidth(width));
}

QSize ShadQuestionnaireChoice::minimumSizeHint() const
{
    const int width = chromeWidth() + kMinTextWidth;
    return QSize(width, kMinCardHeight + 2 * ShadStyleHelper::kFocusRingWidth);
}

bool ShadQuestionnaireChoice::hasHeightForWidth() const
{
    return true;
}

int ShadQuestionnaireChoice::heightForWidth(int width) const
{
    const int content = qMax(textHeight(textWidth(width)), kIndicatorOffset + kIndicatorSize);
    return qMax(kMinCardHeight, content + 2 * kPaddingY) + 2 * ShadStyleHelper::kFocusRingWidth;
}

void ShadQuestionnaireChoice::nextCheckState()
{
    if (_isRadio && isChecked())
    {
        return;
    }
    QAbstractButton::nextCheckState();
}

void ShadQuestionnaireChoice::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            event->ignore();
            return;
        default:
            break;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadQuestionnaireChoice::enterEvent(QEnterEvent* event)
{
    _hoverTransition->animateTo(1.0);
    QAbstractButton::enterEvent(event);
}

void ShadQuestionnaireChoice::leaveEvent(QEvent* event)
{
    _hoverTransition->animateTo(0.0);
    QAbstractButton::leaveEvent(event);
}

void ShadQuestionnaireChoice::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadQuestionnaireChoice::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadQuestionnaireChoice::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadQuestionnaireChoice::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF bounds(rect());
    const int margin = kFocusRingWidth;
    const QRectF card = bounds.adjusted(margin, margin, -margin, -margin);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool checked = isChecked();
    const bool showFocusRing = hasFocus() && _isFocusVisible;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    QColor background = shadTheme->isDark() ? withAlpha(shadTheme->getColor(ShadThemeType::Input), kDarkBackgroundAlpha) : QColor(0, 0, 0, 0);
    background = mix(background, withAlpha(shadTheme->getColor(ShadThemeType::Muted), kHoverAlpha), _hoverTransition->value());
    QColor border = shadTheme->getColor(ShadThemeType::Input);
    if (checked)
    {
        background = shadTheme->getColor(ShadThemeType::Muted);
        border = withAlpha(shadTheme->getColor(ShadThemeType::Primary), kCheckedBorderAlpha);
    }
    if (_isInvalid)
    {
        border = shadTheme->getColor(ShadThemeType::Destructive);
    }
    if (showFocusRing)
    {
        border = shadTheme->getColor(ShadThemeType::Ring);
    }
    drawShadowXs(&painter, card, radius);
    fillRoundedRect(&painter, card, radius, background);
    strokeRoundedRect(&painter, card, radius, border);
    if (showFocusRing)
    {
        drawFocusRing(&painter, card, radius, focusRingColor(_isInvalid));
    }

    const qreal top = card.top() + kPaddingY;
    const QRectF indicator = visualRect(direction, bounds, QRectF(card.left() + kPaddingX, top + kIndicatorOffset, kIndicatorSize, kIndicatorSize));
    if (_isRadio)
    {
        QColor fill = shadTheme->isDark() ? withAlpha(shadTheme->getColor(ShadThemeType::Input), kDarkIndicatorAlpha) : QColor(0, 0, 0, 0);
        QColor stroke = shadTheme->getColor(ShadThemeType::Input);
        if (checked)
        {
            fill = shadTheme->getColor(ShadThemeType::Primary);
            stroke = fill;
        }
        painter.setPen(QPen(stroke, 1.0));
        painter.setBrush(fill);
        painter.drawEllipse(indicator.adjusted(0.5, 0.5, -0.5, -0.5));
        if (checked)
        {
            painter.setPen(Qt::NoPen);
            painter.setBrush(shadTheme->getColor(ShadThemeType::PrimaryForeground));
            const qreal inset = (kIndicatorSize - kDotSize) / 2.0;
            painter.drawEllipse(indicator.adjusted(inset, inset, -inset, -inset));
        }
    }
    else
    {
        drawCheckbox(&painter, indicator, checked ? Qt::Checked : Qt::Unchecked);
    }

    const int contentWidth = textWidth(width());
    const qreal textLeft = card.left() + kPaddingX + kIndicatorSize + kGap;
    const int textFlags = static_cast<int>(visualAlignment(direction, Qt::AlignLeft | Qt::AlignTop)) | Qt::TextWordWrap;
    const QFont boldFont = labelFont();
    const int labelHeight = wrappedHeight(boldFont, _label, contentWidth);
    painter.setFont(boldFont);
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    painter.drawText(visualRect(direction, bounds, QRectF(textLeft, top, contentWidth, labelHeight)), textFlags, _label);
    if (!_description.isEmpty())
    {
        const qreal descriptionTop = top + labelHeight + kContentGap;
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(visualRect(direction, bounds, QRectF(textLeft, descriptionTop, contentWidth, card.bottom() - descriptionTop)), textFlags, _description);
    }

    if (!_shortcutText.isEmpty())
    {
        const QRectF badge = visualRect(direction, bounds, QRectF(card.right() - kPaddingX - kShortcutSize, top + kIndicatorOffset, kShortcutSize, kShortcutSize));
        drawShadowXs(&painter, badge, radius);
        fillRoundedRect(&painter, badge, radius, shadTheme->getColor(ShadThemeType::Background));
        strokeRoundedRect(&painter, badge, radius, shadTheme->getColor(ShadThemeType::Input));
        painter.setFont(shortcutFont());
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(badge, Qt::AlignCenter, _shortcutText);
    }
}

int ShadQuestionnaireChoice::chromeWidth() const
{
    const int shortcut = _shortcutText.isEmpty() ? 0 : kGap + kShortcutSize;
    return 2 * ShadStyleHelper::kFocusRingWidth + 2 * kPaddingX + kIndicatorSize + kGap + shortcut;
}

int ShadQuestionnaireChoice::textWidth(int width) const
{
    return width - chromeWidth();
}

int ShadQuestionnaireChoice::textHeight(int width) const
{
    const int labelHeight = wrappedHeight(labelFont(), _label, width);
    const int descriptionHeight = wrappedHeight(font(), _description, width);
    return labelHeight + (descriptionHeight > 0 ? kContentGap + descriptionHeight : 0);
}

QFont ShadQuestionnaireChoice::labelFont() const
{
    QFont labelFont = font();
    labelFont.setWeight(QFont::Medium);
    return labelFont;
}

QFont ShadQuestionnaireChoice::shortcutFont() const
{
    QFont monoFont = font();
    monoFont.setFamilies(ShadStyleHelper::monospaceFontFamilies());
    monoFont.setPixelSize(kShortcutFontSize);
    monoFont.setWeight(QFont::Medium);
    return monoFont;
}
