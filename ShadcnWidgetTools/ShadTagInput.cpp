#include "ShadTagInput.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadStyleHelper.h"
#include "ShadTagInputPrivate.h"
#include "ShadTheme.h"

ShadTagInput::ShadTagInput(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTagInputPrivate())
{
    Q_D(ShadTagInput);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
    setCursor(Qt::IBeamCursor);
    d->_input = new ShadInput(this);
    ShadStyleHelper::setEmbedded(d->_input, true);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_input->installEventFilter(this);
    setFocusProxy(d->_input);
    connect(d->_input, &QLineEdit::textEdited, this, [d](const QString& text) { d->handleTextEdited(text); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->relayout();
}

ShadTagInput::~ShadTagInput()
{
}

void ShadTagInput::setTags(const QStringList& tags)
{
    Q_D(ShadTagInput);
    const QStringList normalizedTags = d->normalized(tags);
    if (d->_tags == normalizedTags)
    {
        return;
    }
    d->_tags = normalizedTags;
    d->_activeIndex = -1;
    d->_hoveredCloseIndex = -1;
    d->_pressedCloseIndex = -1;
    d->updatePlaceholder();
    d->relayout();
    Q_EMIT tagsChanged(d->_tags);
}

QStringList ShadTagInput::getTags() const
{
    Q_D(const ShadTagInput);
    return d->_tags;
}

bool ShadTagInput::addTag(const QString& tag)
{
    Q_D(ShadTagInput);
    const QString text = tag.trimmed();
    if (text.isEmpty())
    {
        return false;
    }
    if (!d->canAccept(text))
    {
        Q_EMIT tagRejected(text);
        return false;
    }
    d->_tags.append(text);
    d->_activeIndex = -1;
    d->updatePlaceholder();
    d->relayout();
    Q_EMIT tagAdded(text);
    Q_EMIT tagsChanged(d->_tags);
    return true;
}

bool ShadTagInput::removeTag(const QString& tag)
{
    Q_D(ShadTagInput);
    const int index = int(d->_tags.indexOf(tag));
    if (index < 0)
    {
        return false;
    }
    d->removeAt(index);
    return true;
}

void ShadTagInput::removeTagAt(int index)
{
    Q_D(ShadTagInput);
    d->removeAt(index);
}

QString ShadTagInput::tagAt(int index) const
{
    Q_D(const ShadTagInput);
    return d->_tags.value(index);
}

int ShadTagInput::count() const
{
    Q_D(const ShadTagInput);
    return int(d->_tags.size());
}

void ShadTagInput::setPlaceholderText(const QString& text)
{
    Q_D(ShadTagInput);
    d->_placeholderText = text;
    d->updatePlaceholder();
}

QString ShadTagInput::getPlaceholderText() const
{
    Q_D(const ShadTagInput);
    return d->_placeholderText;
}

void ShadTagInput::setSeparators(const QString& separators)
{
    Q_D(ShadTagInput);
    d->_separators = separators;
}

QString ShadTagInput::getSeparators() const
{
    Q_D(const ShadTagInput);
    return d->_separators;
}

void ShadTagInput::setMaxTags(int maxTags)
{
    Q_D(ShadTagInput);
    d->_maxTags = qMax(0, maxTags);
    d->applyConstraints();
}

int ShadTagInput::getMaxTags() const
{
    Q_D(const ShadTagInput);
    return d->_maxTags;
}

void ShadTagInput::setDuplicatesAllowed(bool allowed)
{
    Q_D(ShadTagInput);
    d->_isDuplicatesAllowed = allowed;
    d->applyConstraints();
}

bool ShadTagInput::isDuplicatesAllowed() const
{
    Q_D(const ShadTagInput);
    return d->_isDuplicatesAllowed;
}

void ShadTagInput::setReadOnly(bool readOnly)
{
    Q_D(ShadTagInput);
    if (d->_isReadOnly == readOnly)
    {
        return;
    }
    d->_isReadOnly = readOnly;
    d->_activeIndex = -1;
    d->_hoveredCloseIndex = -1;
    d->_pressedCloseIndex = -1;
    if (readOnly)
    {
        ShadStyleHelper::clearFocusWithin(d->_input);
    }
    d->_input->setVisible(!readOnly);
    setCursor(readOnly ? Qt::ArrowCursor : Qt::IBeamCursor);
    d->relayout();
}

bool ShadTagInput::isReadOnly() const
{
    Q_D(const ShadTagInput);
    return d->_isReadOnly;
}

void ShadTagInput::setInvalid(bool invalid)
{
    Q_D(ShadTagInput);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadTagInput::isInvalid() const
{
    Q_D(const ShadTagInput);
    return d->_isInvalid;
}

QLineEdit* ShadTagInput::lineEdit() const
{
    Q_D(const ShadTagInput);
    return d->_input;
}

QSize ShadTagInput::sizeHint() const
{
    Q_D(const ShadTagInput);
    return QSize(ShadTagInputPrivate::kDefaultWidth, d->_layoutHeight);
}

QSize ShadTagInput::minimumSizeHint() const
{
    Q_D(const ShadTagInput);
    return QSize(ShadTagInputPrivate::kMinInputWidth + 2 * (ShadStyleHelper::kFocusRingWidth + ShadTagInputPrivate::kPaddingX), d->_layoutHeight);
}

void ShadTagInput::clear()
{
    Q_D(ShadTagInput);
    d->_input->clear();
    setTags(QStringList());
}

bool ShadTagInput::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadTagInput);
    if (watched == d->_input)
    {
        if (event->type() == QEvent::FocusIn)
        {
            update();
        }
        else if (event->type() == QEvent::FocusOut)
        {
            d->_activeIndex = -1;
            update();
        }
        else if (event->type() == QEvent::KeyPress && d->handleKeyPress(static_cast<QKeyEvent*>(event)))
        {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadTagInput::changeEvent(QEvent* event)
{
    Q_D(ShadTagInput);
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::FontChange)
    {
        d->relayout();
    }
    QWidget::changeEvent(event);
}

void ShadTagInput::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadTagInput);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    const int index = d->tagIndexAt(pos);
    if (index >= 0 && d->isCloseVisible() && d->closeRect(index).contains(pos))
    {
        d->_pressedCloseIndex = index;
    }
    else
    {
        d->setActiveIndex(d->_isReadOnly ? -1 : index);
    }
    if (!d->_isReadOnly)
    {
        d->_input->setFocus(Qt::MouseFocusReason);
    }
    event->accept();
}

void ShadTagInput::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadTagInput);
    d->updateHover(event->position().toPoint());
    QWidget::mouseMoveEvent(event);
}

void ShadTagInput::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadTagInput);
    if (event->button() != Qt::LeftButton || d->_pressedCloseIndex < 0)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    const int index = d->_pressedCloseIndex;
    d->_pressedCloseIndex = -1;
    if (index < d->_tagRects.size() && d->closeRect(index).contains(pos))
    {
        d->removeAt(index);
    }
    d->updateHover(pos);
    event->accept();
}

void ShadTagInput::leaveEvent(QEvent* event)
{
    Q_D(ShadTagInput);
    d->updateHover(QPoint(-1, -1));
    QWidget::leaveEvent(event);
}

void ShadTagInput::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadTagInput);
    QWidget::resizeEvent(event);
    d->relayout();
}

void ShadTagInput::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    Q_D(ShadTagInput);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isFocused = d->_input->hasFocus();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (d->_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (isFocused || d->_isInvalid)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_isInvalid));
    }
    d->paintTags(&painter);
}
