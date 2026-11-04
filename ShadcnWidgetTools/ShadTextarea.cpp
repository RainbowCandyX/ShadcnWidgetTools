#include "ShadTextarea.h"

#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QTextDocument>
#include <QWidget>
#include <QtGlobal>

#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTextareaPrivate.h"
#include "ShadTheme.h"

ShadTextarea::ShadTextarea(QWidget* parent)
    : QPlainTextEdit(parent), d_ptr(new ShadTextareaPrivate())
{
    Q_D(ShadTextarea);
    d->q_ptr = this;
    // 使用 Plain Box 线框仅为了获得固定的 frameWidth，外框实际由 paintFrame 自绘
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(ShadTextareaPrivate::kFrameInset);
    setViewportMargins(ShadTextareaPrivate::kPaddingX, ShadTextareaPrivate::kPaddingY, ShadTextareaPrivate::kPaddingX, ShadTextareaPrivate::kPaddingY);
    viewport()->setAutoFillBackground(false);
    // 替换为细滚动条，轨道两端留白避开 rounded-md 圆角
    ShadScrollBar* verticalBar = new ShadScrollBar(Qt::Vertical, this);
    verticalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadTextareaPrivate::kFrameInset);
    setVerticalScrollBar(verticalBar);
    ShadScrollBar* horizontalBar = new ShadScrollBar(Qt::Horizontal, this);
    horizontalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadTextareaPrivate::kFrameInset);
    setHorizontalScrollBar(horizontalBar);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->applyThemeColors();
    d->updateScrollBarPolicy();
    connect(document(), &QTextDocument::contentsChanged, this, [d]() { d->updateAutoHeight(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->applyThemeColors(); });
}

ShadTextarea::~ShadTextarea()
{
}

void ShadTextarea::setInvalid(bool invalid)
{
    Q_D(ShadTextarea);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadTextarea::isInvalid() const
{
    Q_D(const ShadTextarea);
    return d->_isInvalid;
}

void ShadTextarea::setAutoResize(bool autoResize)
{
    Q_D(ShadTextarea);
    if (d->_isAutoResize == autoResize)
    {
        return;
    }
    d->_isAutoResize = autoResize;
    setSizePolicy(QSizePolicy::Expanding, autoResize ? QSizePolicy::Fixed : QSizePolicy::Preferred);
    d->updateScrollBarPolicy();
    d->updateAutoHeight();
    updateGeometry();
}

bool ShadTextarea::isAutoResize() const
{
    Q_D(const ShadTextarea);
    return d->_isAutoResize;
}

void ShadTextarea::setMaximumAutoHeight(int height)
{
    Q_D(ShadTextarea);
    d->_maximumAutoHeight = qMax(0, height);
    d->updateScrollBarPolicy();
    d->updateAutoHeight();
}

int ShadTextarea::getMaximumAutoHeight() const
{
    Q_D(const ShadTextarea);
    return d->_maximumAutoHeight;
}

QSize ShadTextarea::sizeHint() const
{
    Q_D(const ShadTextarea);
    const int height = d->_isAutoResize ? d->_autoHeight : ShadTextareaPrivate::kMinVisualHeight + 2 * d->focusMargin();
    return QSize(QPlainTextEdit::sizeHint().width(), height);
}

QSize ShadTextarea::minimumSizeHint() const
{
    Q_D(const ShadTextarea);
    return QSize(QPlainTextEdit::minimumSizeHint().width(), ShadTextareaPrivate::kMinVisualHeight + 2 * d->focusMargin());
}

bool ShadTextarea::event(QEvent* event)
{
    Q_D(ShadTextarea);
    if (event->type() == QEvent::Paint)
    {
        d->paintFrame();
        return true;
    }
    if (event->type() == QEvent::DynamicPropertyChange && static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() == ShadStyleHelper::kEmbeddedProperty)
    {
        d->applyEmbedded();
    }
    if (event->type() == QEvent::FontChange)
    {
        // 先让基类把新字体应用到文档，再按新的行高计算
        const bool isHandled = QPlainTextEdit::event(event);
        d->updateAutoHeight();
        return isHandled;
    }
    return QPlainTextEdit::event(event);
}

void ShadTextarea::focusInEvent(QFocusEvent* event)
{
    QPlainTextEdit::focusInEvent(event);
    update();
}

void ShadTextarea::focusOutEvent(QFocusEvent* event)
{
    QPlainTextEdit::focusOutEvent(event);
    update();
}

void ShadTextarea::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadTextarea);
    QPlainTextEdit::resizeEvent(event);
    // 宽度变化会改变自动换行，需要重新计算高度
    if (event->size().width() != event->oldSize().width())
    {
        d->updateAutoHeight();
    }
}
