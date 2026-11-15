#include "ShadDiffView.h"

#include <QColor>
#include <QEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDiffContent.h"
#include "ShadDiffViewPrivate.h"
#include "ShadScrollArea.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kDefaultWidth = 480;
    constexpr int kMinimumWidth = 160;
} // namespace

ShadDiffView::ShadDiffView(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDiffViewPrivate())
{
    Q_D(ShadDiffView);
    d->q_ptr = this;
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    d->_scrollArea = new ShadScrollArea(this);
    d->_scrollArea->setWidgetResizable(false);
    d->_scrollArea->setLayoutDirection(Qt::LeftToRight);
    d->_scrollArea->setFocusPolicy(Qt::NoFocus);
    d->_content = new ShadDiffContent();
    d->_scrollArea->setWidget(d->_content);

    connect(d->_content, &ShadDiffContent::contentSizeChanged, this, [d]() { d->updateLayout(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->updateLayout(); });
    d->updateLayout();
}

ShadDiffView::ShadDiffView(const QString& oldText, const QString& newText, QWidget* parent)
    : ShadDiffView(parent)
{
    setTexts(oldText, newText);
}

ShadDiffView::~ShadDiffView()
{
    Q_D(ShadDiffView);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    QObject::disconnect(d->_content, nullptr, this, nullptr);
}

void ShadDiffView::setTexts(const QString& oldText, const QString& newText)
{
    Q_D(ShadDiffView);
    d->_content->setTexts(oldText, newText);
}

void ShadDiffView::setOldText(const QString& text)
{
    Q_D(ShadDiffView);
    d->_content->setTexts(text, d->_content->getNewText());
}

QString ShadDiffView::getOldText() const
{
    Q_D(const ShadDiffView);
    return d->_content->getOldText();
}

void ShadDiffView::setNewText(const QString& text)
{
    Q_D(ShadDiffView);
    d->_content->setTexts(d->_content->getOldText(), text);
}

QString ShadDiffView::getNewText() const
{
    Q_D(const ShadDiffView);
    return d->_content->getNewText();
}

void ShadDiffView::setMode(ShadDiffViewType::Mode mode)
{
    Q_D(ShadDiffView);
    d->_content->setMode(mode);
}

ShadDiffViewType::Mode ShadDiffView::getMode() const
{
    Q_D(const ShadDiffView);
    return d->_content->getMode();
}

void ShadDiffView::setLanguage(const QString& language)
{
    Q_D(ShadDiffView);
    d->_content->setLanguage(language);
}

QString ShadDiffView::getLanguage() const
{
    Q_D(const ShadDiffView);
    return d->_content->getLanguage();
}

void ShadDiffView::setLineNumbersVisible(bool visible)
{
    Q_D(ShadDiffView);
    d->_content->setLineNumbersVisible(visible);
}

bool ShadDiffView::isLineNumbersVisible() const
{
    Q_D(const ShadDiffView);
    return d->_content->isLineNumbersVisible();
}

void ShadDiffView::setContextLines(int lines)
{
    Q_D(ShadDiffView);
    d->_content->setContextLines(lines);
}

int ShadDiffView::getContextLines() const
{
    Q_D(const ShadDiffView);
    return d->_content->getContextLines();
}

void ShadDiffView::setWordWrap(bool wrap)
{
    Q_D(ShadDiffView);
    d->_content->setWordWrap(wrap);
}

bool ShadDiffView::isWordWrap() const
{
    Q_D(const ShadDiffView);
    return d->_content->isWordWrap();
}

void ShadDiffView::setSelectable(bool selectable)
{
    Q_D(ShadDiffView);
    d->_content->setSelectable(selectable);
}

bool ShadDiffView::isSelectable() const
{
    Q_D(const ShadDiffView);
    return d->_content->isSelectable();
}

QString ShadDiffView::selectedText() const
{
    Q_D(const ShadDiffView);
    return d->_content->selectedText();
}

void ShadDiffView::setMaximumContentHeight(int height)
{
    Q_D(ShadDiffView);
    const int value = qMax(0, height);
    if (d->_maximumContentHeight == value)
    {
        return;
    }
    d->_maximumContentHeight = value;
    d->updateLayout();
}

int ShadDiffView::getMaximumContentHeight() const
{
    Q_D(const ShadDiffView);
    return d->_maximumContentHeight;
}

int ShadDiffView::getAddedLineCount() const
{
    Q_D(const ShadDiffView);
    return d->_content->addedLineCount();
}

int ShadDiffView::getRemovedLineCount() const
{
    Q_D(const ShadDiffView);
    return d->_content->removedLineCount();
}

QSize ShadDiffView::sizeHint() const
{
    Q_D(const ShadDiffView);
    return QSize(kDefaultWidth, d->_preferredHeight);
}

QSize ShadDiffView::minimumSizeHint() const
{
    Q_D(const ShadDiffView);
    return QSize(kMinimumWidth, d->_preferredHeight);
}

void ShadDiffView::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadDiffView::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadDiffView);
    QWidget::resizeEvent(event);
    d->updateLayout();
}

void ShadDiffView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF bodyRect(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Input));
}
