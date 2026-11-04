#include "ShadDialogPrivate.h"

#include <QCursor>
#include <QGuiApplication>
#include <QLayout>
#include <QRect>
#include <QScreen>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadDialogOverlay.h"
#include "ShadLabel.h"
#include "ShadTitleBar.h"

ShadDialogPrivate::ShadDialogPrivate()
{
}

ShadDialogPrivate::~ShadDialogPrivate()
{
}

QWidget* ShadDialogPrivate::hostWindow() const
{
    Q_Q(const ShadDialog);
    QWidget* parent = q->parentWidget();
    return parent != nullptr ? parent->window() : nullptr;
}

QScreen* ShadDialogPrivate::targetScreen() const
{
    QWidget* host = hostWindow();
    if (host != nullptr && host->isVisible() && host->screen() != nullptr)
    {
        return host->screen();
    }
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    return screen != nullptr ? screen : QGuiApplication::primaryScreen();
}

void ShadDialogPrivate::updateWindowHints()
{
    Q_Q(ShadDialog);
    const ShadWindowType::ButtonFlags buttonFlags = _titleBar->getButtonFlags();
    Qt::WindowFlags windowFlags = q->windowFlags();
    windowFlags.setFlag(Qt::WindowMinimizeButtonHint, buttonFlags.testFlag(ShadWindowType::MinimizeButton));
    windowFlags.setFlag(Qt::WindowMaximizeButtonHint, buttonFlags.testFlag(ShadWindowType::MaximizeButton));
    if (windowFlags != q->windowFlags())
    {
        q->setWindowFlags(windowFlags);
    }
}

void ShadDialogPrivate::updateHeader()
{
    const bool hasTitle = !_titleLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    _titleLabel->setVisible(hasTitle);
    _descriptionLabel->setVisible(hasDescription);
    _header->setVisible(hasTitle || hasDescription);
}

void ShadDialogPrivate::autoResize()
{
    Q_Q(ShadDialog);
    if (_hasAutoSize && q->size() != _autoSize)
    {
        return;
    }
    const bool isFixedWidth = q->minimumWidth() == q->maximumWidth();
    const bool isFixedHeight = q->minimumHeight() == q->maximumHeight();
    QScreen* screen = targetScreen();
    const QRect available = screen != nullptr ? screen->availableGeometry() : QRect();
    int width = kDefaultWidth;
    if (_hasAutoSize || isFixedWidth || q->testAttribute(Qt::WA_Resized))
    {
        width = q->width();
    }
    if (!isFixedWidth && available.isValid())
    {
        width = qMin(width, available.width() - 2 * kScreenMargin);
    }
    int height = q->height();
    if (!isFixedHeight)
    {
        // 根布局的 totalHeightForWidth 已包含窗口的 contentsMargins（边框）
        const QLayout* layout = q->layout();
        height = layout->hasHeightForWidth() ? layout->totalHeightForWidth(width) : layout->totalSizeHint().height();
        height = qMax(height, layout->totalMinimumSize().height());
        if (available.isValid())
        {
            height = qMin(height, available.height() - 2 * kScreenMargin);
        }
    }
    q->resize(width, height);
    _autoSize = q->size();
    _hasAutoSize = true;
}

void ShadDialogPrivate::focusFirstChild()
{
    Q_Q(ShadDialog);
    QWidget* candidate = q->nextInFocusChain();
    while (candidate != nullptr && candidate != q)
    {
        const bool isTabFocus = (candidate->focusPolicy() & Qt::TabFocus) == Qt::TabFocus;
        if (_body->isAncestorOf(candidate) && candidate->isVisibleTo(q) && candidate->isEnabled() && isTabFocus)
        {
            candidate->setFocus(Qt::PopupFocusReason);
            return;
        }
        candidate = candidate->nextInFocusChain();
    }
    // 没有可聚焦的子组件时对话框自身获得焦点，用于响应 Esc
    q->setFocus(Qt::PopupFocusReason);
}

void ShadDialogPrivate::showOverlay()
{
    Q_Q(ShadDialog);
    QWidget* host = hostWindow();
    if (!_isOverlayVisible || (q->windowModality() == Qt::NonModal && !_isNonModalOverlay) || host == nullptr || !host->isVisible())
    {
        return;
    }
    if (!_overlay.isNull() && _overlay->parentWidget() != host)
    {
        releaseOverlay();
    }
    if (_overlay.isNull())
    {
        _overlay = new ShadDialogOverlay(host);
    }
    _overlay->setProgress(_progress);
    _overlay->show();
    _overlay->bringToFront();
}

void ShadDialogPrivate::releaseOverlay()
{
    if (!_overlay.isNull())
    {
        delete _overlay.data();
    }
}

void ShadDialogPrivate::setProgress(qreal progress)
{
    Q_Q(ShadDialog);
    _progress = progress;
    q->setWindowOpacity(progress);
    if (!_overlay.isNull())
    {
        _overlay->setProgress(progress);
    }
}
