#include "ShadNavigationMenuLink.h"

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadItemContent.h"
#include "ShadNavigationMenuLinkPrivate.h"
#include "ShadNavigationMenuViewport.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadNavigationMenuLinkPrivate::ShadNavigationMenuLinkPrivate()
{
}

ShadNavigationMenuLinkPrivate::~ShadNavigationMenuLinkPrivate()
{
}

ShadNavigationMenuLink::ShadNavigationMenuLink(QWidget* parent)
    : ShadNavigationMenuLink(QString(), QString(), parent)
{
}

ShadNavigationMenuLink::ShadNavigationMenuLink(const QString& title, const QString& description, QWidget* parent)
    : QAbstractButton(parent), d_ptr(new ShadNavigationMenuLinkPrivate())
{
    Q_D(ShadNavigationMenuLink);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    QVBoxLayout* layout = new QVBoxLayout(this);
    const int padding = ShadNavigationMenuLinkPrivate::kPadding;
    layout->setContentsMargins(padding, padding, padding, padding);
    layout->setSpacing(0);
    d->_content = new ShadItemContent(this);
    d->_content->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(d->_content);
    setTitle(title);
    setDescription(description);
    connect(this, &QAbstractButton::clicked, this, [this]()
    {
        if (ShadNavigationMenuViewport* viewport = qobject_cast<ShadNavigationMenuViewport*>(window()))
        {
            viewport->hide();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadNavigationMenuLink::~ShadNavigationMenuLink()
{
}

void ShadNavigationMenuLink::setTitle(const QString& title)
{
    Q_D(ShadNavigationMenuLink);
    setText(title);
    d->_content->setTitle(title);
}

QString ShadNavigationMenuLink::getTitle() const
{
    return text();
}

void ShadNavigationMenuLink::setDescription(const QString& description)
{
    Q_D(ShadNavigationMenuLink);
    d->_content->setDescription(description);
}

QString ShadNavigationMenuLink::getDescription() const
{
    Q_D(const ShadNavigationMenuLink);
    return d->_content->getDescription();
}

QSize ShadNavigationMenuLink::sizeHint() const
{
    const QSize hint = QAbstractButton::sizeHint();
    const int width = qMin(hint.width(), ShadNavigationMenuLinkPrivate::kPreferredWidth + 2 * ShadNavigationMenuLinkPrivate::kPadding);
    return QSize(width, heightForWidth(width));
}

void ShadNavigationMenuLink::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        click();
        event->accept();
        return;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadNavigationMenuLink::enterEvent(QEnterEvent* event)
{
    Q_D(ShadNavigationMenuLink);
    d->_isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ShadNavigationMenuLink::leaveEvent(QEvent* event)
{
    Q_D(ShadNavigationMenuLink);
    d->_isHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ShadNavigationMenuLink::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadNavigationMenuLink);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadNavigationMenuLink::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadNavigationMenuLink);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadNavigationMenuLink::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadNavigationMenuLink);
    using namespace ShadStyleHelper;
    const QRectF bounds(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Small);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // hover / focus 为 bg-accent
    if (isEnabled() && (d->_isHovered || hasFocus()))
    {
        fillRoundedRect(&painter, bounds, radius, shadTheme->getColor(ShadThemeType::Accent));
    }
    if (hasFocus() && d->_isFocusVisible)
    {
        const QRectF box = bounds.adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
        strokeRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, box, radius, focusRingColor(false));
    }
}
