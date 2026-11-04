#include "ShadInputOtp.h"

#include <QClipboard>
#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QTimer>
#include <QtCore/qnamespace.h>

#include "ShadIcon.h"
#include "ShadInputOtpPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadInputOtp::ShadInputOtp(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadInputOtpPrivate())
{
    Q_D(ShadInputOtp);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    // 接收输入法提交的文字，例如全角数字
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setCursor(Qt::IBeamCursor);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_caretClock.start();
    d->_caretTimer = new QTimer(this);
    connect(d->_caretTimer, &QTimer::timeout, this, [this]() { update(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadInputOtp::~ShadInputOtp()
{
}

void ShadInputOtp::setGroups(const QList<int>& groups)
{
    Q_D(ShadInputOtp);
    QList<int> validGroups;
    for (int size : groups)
    {
        if (size > 0)
        {
            validGroups.append(size);
        }
    }
    if (validGroups.isEmpty())
    {
        validGroups.append(1);
    }
    d->_groups = validGroups;
    const QString value = d->_value.left(d->maxLength());
    d->commit(value, qMin(d->_cursor, int(value.size())), false);
    updateGeometry();
}

QList<int> ShadInputOtp::getGroups() const
{
    Q_D(const ShadInputOtp);
    return d->_groups;
}

int ShadInputOtp::getMaxLength() const
{
    Q_D(const ShadInputOtp);
    return d->maxLength();
}

void ShadInputOtp::setValue(const QString& value)
{
    Q_D(ShadInputOtp);
    const QString truncated = d->_isUpperCase ? value.left(d->maxLength()).toUpper() : value.left(d->maxLength());
    if (!d->isAcceptable(truncated))
    {
        return;
    }
    d->commit(truncated, int(truncated.size()), false);
}

QString ShadInputOtp::getValue() const
{
    Q_D(const ShadInputOtp);
    return d->_value;
}

void ShadInputOtp::clear()
{
    Q_D(ShadInputOtp);
    d->commit(QString(), 0, false);
}

void ShadInputOtp::setPattern(const QRegularExpression& pattern)
{
    Q_D(ShadInputOtp);
    d->_pattern = pattern;
}

QRegularExpression ShadInputOtp::getPattern() const
{
    Q_D(const ShadInputOtp);
    return d->_pattern;
}

QRegularExpression ShadInputOtp::onlyDigits()
{
    return QRegularExpression(QStringLiteral("^\\d+$"));
}

QRegularExpression ShadInputOtp::onlyChars()
{
    return QRegularExpression(QStringLiteral("^[a-zA-Z]+$"));
}

QRegularExpression ShadInputOtp::onlyDigitsAndChars()
{
    return QRegularExpression(QStringLiteral("^[a-zA-Z0-9]+$"));
}

void ShadInputOtp::setUpperCase(bool upperCase)
{
    Q_D(ShadInputOtp);
    if (d->_isUpperCase == upperCase)
    {
        return;
    }
    d->_isUpperCase = upperCase;
    // 开启时把已有的值一并转为大写；转换后不符合规则（例如自定义规则只接受小写）时保留原值
    if (upperCase)
    {
        const QString value = d->_value.toUpper();
        if (d->isAcceptable(value))
        {
            d->commit(value, d->_cursor, false);
        }
    }
}

bool ShadInputOtp::isUpperCase() const
{
    Q_D(const ShadInputOtp);
    return d->_isUpperCase;
}

void ShadInputOtp::setInvalid(bool invalid)
{
    Q_D(ShadInputOtp);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadInputOtp::isInvalid() const
{
    Q_D(const ShadInputOtp);
    return d->_isInvalid;
}

void ShadInputOtp::setSlotSize(const QSize& size)
{
    Q_D(ShadInputOtp);
    d->_slotSize = size.expandedTo(QSize(1, 1));
    updateGeometry();
    update();
}

QSize ShadInputOtp::getSlotSize() const
{
    Q_D(const ShadInputOtp);
    return d->_slotSize;
}

void ShadInputOtp::setSlotTextPixelSize(int pixelSize)
{
    Q_D(ShadInputOtp);
    d->_textPixelSize = qMax(1, pixelSize);
    update();
}

int ShadInputOtp::getSlotTextPixelSize() const
{
    Q_D(const ShadInputOtp);
    return d->_textPixelSize;
}

QSize ShadInputOtp::sizeHint() const
{
    Q_D(const ShadInputOtp);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int separatorWidth = 2 * ShadInputOtpPrivate::kGap + ShadInputOtpPrivate::kSeparatorSize;
    const int width = d->maxLength() * d->_slotSize.width() + (int(d->_groups.size()) - 1) * separatorWidth;
    return QSize(width + margin, d->_slotSize.height() + margin);
}

QSize ShadInputOtp::minimumSizeHint() const
{
    return sizeHint();
}

QVariant ShadInputOtp::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Q_D(const ShadInputOtp);
    // 输入法候选框显示在当前格附近
    if (query == Qt::ImCursorRectangle)
    {
        const int active = d->activeIndex();
        const QList<QRect> slotRects = d->geometry().slotRects;
        return active >= 0 && active < slotRects.size() ? QVariant(slotRects.at(active)) : QVariant(rect());
    }
    return QWidget::inputMethodQuery(query);
}

void ShadInputOtp::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadInputOtp);
    using namespace ShadStyleHelper;
    const ShadInputOtpPrivate::Geometry geometry = d->geometry();
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor borderColor = shadTheme->getColor(d->_isInvalid ? ShadThemeType::Destructive : ShadThemeType::Input);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // 对应 has-disabled:opacity-50
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 每组：shadow-xs，暗色 bg-input/30
    for (const QRect& groupRect : geometry.groups)
    {
        drawShadowXs(&painter, QRectF(groupRect), radius);
        if (shadTheme->isDark())
        {
            fillRoundedRect(&painter, QRectF(groupRect), radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
        }
    }
    // 同组相邻格子之间的 border-r，位于格子的逻辑结束侧
    painter.setPen(QPen(borderColor, 1.0));
    for (int index = 0; index < geometry.slotRects.size(); ++index)
    {
        if (d->isGroupEnd(index))
        {
            continue;
        }
        const QRect& slotRect = geometry.slotRects.at(index);
        const qreal lineX = isRightToLeft ? slotRect.left() + 0.5 : slotRect.left() + slotRect.width() - 0.5;
        painter.drawLine(QPointF(lineX, slotRect.top()), QPointF(lineX, slotRect.top() + slotRect.height()));
    }
    for (const QRect& groupRect : geometry.groups)
    {
        strokeRoundedRect(&painter, QRectF(groupRect), radius, borderColor);
    }
    for (const QRect& separatorRect : geometry.separators)
    {
        ShadIcon::paint(&painter, QRectF(separatorRect), ShadIconType::Minus, foreground, ShadInputOtpPrivate::kSeparatorSize);
    }

    // 字符
    QFont slotFont = font();
    slotFont.setPixelSize(d->_textPixelSize);
    painter.setFont(slotFont);
    painter.setPen(foreground);
    for (int index = 0; index < d->_value.size() && index < geometry.slotRects.size(); ++index)
    {
        painter.drawText(QRectF(geometry.slotRects.at(index)), Qt::AlignCenter, QString(d->_value.at(index)));
    }

    // has-aria-invalid:ring-3 ring-destructive/20：整组外围的错误圆环
    if (d->_isInvalid)
    {
        for (const QRect& groupRect : geometry.groups)
        {
            drawFocusRing(&painter, QRectF(groupRect), radius, focusRingColor(true));
        }
    }

    // 当前格：data-[active=true]:border-ring ring-3 ring-ring/50 z-10，盖住相邻格子共用的边框
    const int active = d->activeIndex();
    if (active < 0 || active >= geometry.slotRects.size())
    {
        return;
    }
    const bool isStart = d->isGroupStart(active);
    const bool isEnd = d->isGroupEnd(active);
    QRectF activeRect(geometry.slotRects.at(active));
    if (!isStart)
    {
        // 向逻辑起始侧扩展 1px，覆盖前一格的 border-r
        activeRect.adjust(isRightToLeft ? 0.0 : -1.0, 0.0, isRightToLeft ? 1.0 : 0.0, 0.0);
    }
    // 只有组的首尾格为圆角，按物理方向换算
    const bool roundLeft = isRightToLeft ? isEnd : isStart;
    const bool roundRight = isRightToLeft ? isStart : isEnd;
    const QColor activeBorder = shadTheme->getColor(d->_isInvalid ? ShadThemeType::Destructive : ShadThemeType::Ring);
    strokeRoundedRect(&painter, activeRect, radius, activeBorder, 1.0, roundLeft, roundRight);
    drawFocusRing(&painter, activeRect, radius, focusRingColor(d->_isInvalid), kFocusRingWidth, roundLeft, roundRight);

    // 当前格为空时的假光标：h-4 w-px bg-foreground animate-caret-blink
    if (active >= d->_value.size() && isEnabled())
    {
        const QPointF center = QRectF(geometry.slotRects.at(active)).center();
        const qreal caretHeight = ShadInputOtpPrivate::kCaretHeight;
        painter.setOpacity(painter.opacity() * d->caretOpacity());
        painter.fillRect(QRectF(qRound(center.x()) - 0.5, center.y() - caretHeight / 2.0, 1.0, caretHeight), foreground);
    }
}

void ShadInputOtp::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadInputOtp);
    if (event->matches(QKeySequence::Paste))
    {
        const QClipboard* clipboard = QGuiApplication::clipboard();
        if (clipboard != nullptr)
        {
            d->insertText(clipboard->text());
        }
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Copy))
    {
        QClipboard* clipboard = QGuiApplication::clipboard();
        if (clipboard != nullptr && !d->_value.isEmpty())
        {
            clipboard->setText(d->_value);
        }
        event->accept();
        return;
    }
    // 方向键在已输入的格子之间移动；填满后光标位于末尾，当前格按最后一格计算
    const int length = d->maxLength();
    const int current = qMin(d->_cursor, qMax(0, length - 1));
    const int forward = layoutDirection() == Qt::RightToLeft ? -1 : 1;
    switch (event->key())
    {
        case Qt::Key_Backspace:
            d->removeBefore();
            break;
        case Qt::Key_Delete:
            d->removeAt();
            break;
        case Qt::Key_Left:
            d->setCursor(current - forward);
            break;
        case Qt::Key_Right:
            d->setCursor(current + forward);
            break;
        case Qt::Key_Home:
            d->setCursor(0);
            break;
        case Qt::Key_End:
            d->setCursor(int(d->_value.size()));
            break;
        default:
            {
                const QString text = event->text();
                const bool hasCommandModifier = event->modifiers().testAnyFlags(Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
                if (text.isEmpty() || hasCommandModifier || !text.at(0).isPrint() || text.at(0).isSpace())
                {
                    QWidget::keyPressEvent(event);
                    return;
                }
                d->insertText(text);
                break;
            }
    }
    event->accept();
}

void ShadInputOtp::inputMethodEvent(QInputMethodEvent* event)
{
    Q_D(ShadInputOtp);
    if (!event->commitString().isEmpty())
    {
        d->insertText(event->commitString());
    }
    event->accept();
}

void ShadInputOtp::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadInputOtp);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    // 点击已输入的格子时移到该格，点击空格子或格子之外时移到末尾
    const int index = d->slotAt(event->position().toPoint());
    d->setCursor(index >= 0 ? qMin(index, int(d->_value.size())) : int(d->_value.size()));
    event->accept();
}

void ShadInputOtp::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadInputOtp);
    // 获得焦点时当前格在末尾；鼠标点击随后在 mousePressEvent 中改为点击的格子
    d->setCursor(int(d->_value.size()));
    QWidget::focusInEvent(event);
}

void ShadInputOtp::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadInputOtp);
    QWidget::focusOutEvent(event);
    d->updateCaretTimer();
    update();
}

void ShadInputOtp::changeEvent(QEvent* event)
{
    Q_D(ShadInputOtp);
    if (event->type() == QEvent::EnabledChange)
    {
        // 对应 disabled:cursor-not-allowed
        setCursor(isEnabled() ? Qt::IBeamCursor : Qt::ForbiddenCursor);
        d->updateCaretTimer();
        update();
    }
    else if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}
