#include "ShadImageViewer.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QContextMenuEvent>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QWheelEvent>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include "ShadContextMenu.h"
#include "ShadFrameAnimation.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadImageViewerButton.h"
#include "ShadImageViewerPrivate.h"
#include "ShadImageViewerToolbar.h"
#include "ShadSpinner.h"
#include "ShadStyleHelper.h"

ShadImageViewer::ShadImageViewer(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadImageViewerPrivate())
{
    Q_D(ShadImageViewer);
    d->q_ptr = this;
    d->_anchor = parent;
    d->_cache.setMaxCost(ShadImageViewerPrivate::kCacheCostKb);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_NoSystemBackground);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    setAccessibleName(tr("图片预览"));
    hide();
    if (parent != nullptr)
    {
        // 打开后会挂到顶层窗口上，需随 parent 一起销毁
        connect(parent, &QObject::destroyed, this, [this]() { deleteLater(); });
    }

    d->_closeButton = new ShadImageViewerButton(ShadIconType::X, ShadImageViewerButton::Round, this);
    d->_prevButton = new ShadImageViewerButton(ShadIconType::ChevronLeft, ShadImageViewerButton::Round, this);
    d->_nextButton = new ShadImageViewerButton(ShadIconType::ChevronRight, ShadImageViewerButton::Round, this);
    d->_toolbar = new ShadImageViewerToolbar(this);
    d->_zoomOutButton = d->_toolbar->addButton(ShadIconType::ZoomOut);
    d->_zoomLabel = d->_toolbar->addLabel(QStringLiteral("1000%"));
    d->_zoomInButton = d->_toolbar->addButton(ShadIconType::ZoomIn);
    d->_fitButton = d->_toolbar->addButton(ShadIconType::Expand);
    d->_rotateLeftButton = d->_toolbar->addButton(ShadIconType::RotateCcw);
    d->_rotateRightButton = d->_toolbar->addButton(ShadIconType::RotateCw);
    d->_flipHorizontalButton = d->_toolbar->addButton(ShadIconType::FlipHorizontal2);
    d->_flipVerticalButton = d->_toolbar->addButton(ShadIconType::FlipVertical2);
    d->_saveButton = d->_toolbar->addButton(ShadIconType::Download);
    d->_spinner = new ShadSpinner(this);
    d->_spinner->setSpinnerSize(ShadImageViewerPrivate::kSpinnerSize);
    d->_spinner->setColor(QColor(255, 255, 255));
    d->_spinner->hide();
    d->_contextMenu = new ShadContextMenu(this);
    d->_copyAction = d->_contextMenu->addItem(tr("复制图片"), ShadIconType::Copy);
    d->_saveAction = d->_contextMenu->addItem(tr("保存图片…"), ShadIconType::Download);
    connect(d->_copyAction, &QAction::triggered, this, [d]() { d->copyImage(); });
    connect(d->_saveAction, &QAction::triggered, this, [d]() { d->saveImage(); });
    connect(d->_saveButton, &ShadImageViewerButton::clicked, this, [d]() { d->saveImage(); });

    // 按钮与工具栏随打开动画淡入淡出
    const QList<QWidget*> chromeWidgets{d->_closeButton, d->_prevButton, d->_nextButton, d->_toolbar};
    for (QWidget* widget : chromeWidgets)
    {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
        effect->setOpacity(0.0);
        widget->setGraphicsEffect(effect);
        d->_chromeEffects.append(effect);
    }

    connect(d->_closeButton, &ShadImageViewerButton::clicked, this, [this]() { close(); });
    connect(d->_prevButton, &ShadImageViewerButton::clicked, this, [d]() { d->navigate(-1); });
    connect(d->_nextButton, &ShadImageViewerButton::clicked, this, [d]() { d->navigate(1); });
    connect(d->_zoomOutButton, &ShadImageViewerButton::clicked, this, [d]() { d->zoomBy(1.0 / ShadImageViewerPrivate::kZoomStep, d->contentRect().center()); });
    connect(d->_zoomInButton, &ShadImageViewerButton::clicked, this, [d]() { d->zoomBy(ShadImageViewerPrivate::kZoomStep, d->contentRect().center()); });
    connect(d->_fitButton, &ShadImageViewerButton::clicked, this, [d]() { d->toggleFit(d->contentRect().center()); });
    connect(d->_rotateLeftButton, &ShadImageViewerButton::clicked, this, [d]() { d->rotateBy(-90); });
    connect(d->_rotateRightButton, &ShadImageViewerButton::clicked, this, [d]() { d->rotateBy(90); });
    connect(d->_flipHorizontalButton, &ShadImageViewerButton::clicked, this, [this, d]()
    {
        d->_isFlippedHorizontally = !d->_isFlippedHorizontally;
        update();
    });
    connect(d->_flipVerticalButton, &ShadImageViewerButton::clicked, this, [this, d]()
    {
        d->_isFlippedVertically = !d->_isFlippedVertically;
        update();
    });

    d->_appearAnimation = new ShadFrameAnimation(this);
    connect(d->_appearAnimation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->setAppear(value); });
    connect(d->_appearAnimation, &ShadFrameAnimation::finished, this, [d]()
    {
        if (d->_isClosing)
        {
            d->finishClose();
        }
    });
    d->_offsetAnimation = new ShadFrameAnimation(this);
    d->_offsetAnimation->setDuration(ShadImageViewerPrivate::kSnapDurationMs);
    d->_offsetAnimation->setEasingCurve(QEasingCurve::OutCubic);
    d->_offsetAnimation->setStartValue(0.0);
    d->_offsetAnimation->setEndValue(1.0);
    connect(d->_offsetAnimation, &ShadFrameAnimation::valueChanged, this, [this, d](qreal value)
    {
        d->_offset = d->_offsetFrom + (d->_offsetTo - d->_offsetFrom) * value;
        update();
    });

    d->updateChrome();
}

ShadImageViewer::~ShadImageViewer()
{
    Q_D(ShadImageViewer);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    QObject::disconnect(d->_appearAnimation, nullptr, this, nullptr);
    QObject::disconnect(d->_offsetAnimation, nullptr, this, nullptr);
    d->detachHost();
}

void ShadImageViewer::setImagePaths(const QStringList& paths)
{
    Q_D(ShadImageViewer);
    d->_paths = paths;
    d->_pixmaps.clear();
    d->resetSource();
    d->_currentIndex = -1;
    d->showIndex(0, true);
}

QStringList ShadImageViewer::getImagePaths() const
{
    Q_D(const ShadImageViewer);
    return d->_paths;
}

void ShadImageViewer::setPixmaps(const QList<QPixmap>& pixmaps)
{
    Q_D(ShadImageViewer);
    d->_pixmaps = pixmaps;
    d->_paths.clear();
    d->resetSource();
    d->_currentIndex = -1;
    d->showIndex(0, true);
}

QList<QPixmap> ShadImageViewer::getPixmaps() const
{
    Q_D(const ShadImageViewer);
    return d->_pixmaps;
}

int ShadImageViewer::getImageCount() const
{
    Q_D(const ShadImageViewer);
    return d->imageCount();
}

void ShadImageViewer::setCurrentIndex(int index)
{
    Q_D(ShadImageViewer);
    if (d->imageCount() == 0)
    {
        return;
    }
    d->showIndex(index, false);
}

int ShadImageViewer::getCurrentIndex() const
{
    Q_D(const ShadImageViewer);
    return d->_currentIndex;
}

void ShadImageViewer::setToolbarVisible(bool visible)
{
    Q_D(ShadImageViewer);
    if (d->_isToolbarVisible == visible)
    {
        return;
    }
    d->_isToolbarVisible = visible;
    d->updateChrome();
    update();
}

bool ShadImageViewer::isToolbarVisible() const
{
    Q_D(const ShadImageViewer);
    return d->_isToolbarVisible;
}

void ShadImageViewer::open(int index)
{
    Q_D(ShadImageViewer);
    if (d->imageCount() == 0)
    {
        return;
    }
    if (d->_isOpen && !d->_isClosing)
    {
        setCurrentIndex(index);
        return;
    }
    QWidget* host = d->resolveHost();
    if (host == nullptr || host == this)
    {
        return;
    }
    const bool isFresh = !d->_isOpen;
    d->attachHost(host);
    d->_isOpen = true;
    d->_isClosing = false;
    if (isFresh)
    {
        QWidget* focus = QApplication::focusWidget();
        d->_focusBeforeOpen = focus != nullptr && focus != this && !isAncestorOf(focus) ? focus : nullptr;
        // 重新打开时重试之前失败的图片
        d->_failed.clear();
        d->setAppear(0.0);
    }
    setGeometry(host->rect());
    show();
    d->restack();
    setFocus(Qt::PopupFocusReason);
    d->showIndex(index, true);
    d->_appearAnimation->stop();
    d->_appearAnimation->setDuration(ShadImageViewerPrivate::kOpenDurationMs);
    d->_appearAnimation->setEasingCurve(QEasingCurve::OutCubic);
    d->_appearAnimation->setStartValue(d->_appear);
    d->_appearAnimation->setEndValue(1.0);
    d->_appearAnimation->start();
}

bool ShadImageViewer::isOpen() const
{
    Q_D(const ShadImageViewer);
    return d->_isOpen && !d->_isClosing;
}

void ShadImageViewer::preview(QWidget* parent, const QStringList& paths, int index)
{
    ShadImageViewer* viewer = new ShadImageViewer(parent);
    viewer->d_func()->_isDeleteOnClose = true;
    viewer->setImagePaths(paths);
    viewer->open(index);
    if (!viewer->isOpen())
    {
        viewer->deleteLater();
    }
}

bool ShadImageViewer::event(QEvent* event)
{
    Q_D(ShadImageViewer);
    switch (event->type())
    {
        case QEvent::LanguageChange:
            {
                setAccessibleName(tr("图片预览"));
                d->updateChrome();
                break;
            }
        case QEvent::LayoutDirectionChange:
            {
                d->updateChrome();
                break;
            }
        case QEvent::NativeGesture:
            {
                const QNativeGestureEvent* gesture = static_cast<QNativeGestureEvent*>(event);
                if (!d->_isOpen || d->_isClosing || d->_state != ShadImageViewerPrivate::ImageState::Ready)
                {
                    break;
                }
                if (gesture->gestureType() == Qt::ZoomNativeGesture && gesture->value() > -1.0)
                {
                    d->zoomBy(1.0 + gesture->value(), gesture->position());
                }
                else if (gesture->gestureType() == Qt::SmartZoomNativeGesture && d->isImageAt(gesture->position()))
                {
                    d->toggleFit(gesture->position());
                }
                event->accept();
                return true;
            }
        default:
            break;
    }
    return QWidget::event(event);
}

void ShadImageViewer::closeEvent(QCloseEvent* event)
{
    Q_D(ShadImageViewer);
    event->ignore();
    d->requestClose();
}

bool ShadImageViewer::focusNextPrevChild(bool next)
{
    Q_D(ShadImageViewer);
    // 打开期间焦点留在预览上，Tab 不移到被遮住的组件
    if (d->_isOpen)
    {
        return true;
    }
    return QWidget::focusNextPrevChild(next);
}

void ShadImageViewer::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadImageViewer);
    if (!d->_isOpen || d->_isClosing)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const QPointF center = d->contentRect().center();
    switch (event->key())
    {
        case Qt::Key_Escape:
            d->requestClose();
            break;
        case Qt::Key_Left:
        case Qt::Key_Right:
            d->navigate(d->keyStep(event->key()));
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            d->zoomBy(ShadImageViewerPrivate::kZoomStep, center);
            break;
        case Qt::Key_Minus:
        case Qt::Key_Underscore:
            d->zoomBy(1.0 / ShadImageViewerPrivate::kZoomStep, center);
            break;
        case Qt::Key_0:
            {
                d->resetTransform();
                d->updateChrome();
                update();
                break;
            }
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

void ShadImageViewer::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadImageViewer);
    event->accept();
    if (event->button() != Qt::LeftButton || !d->_isOpen || d->_isClosing)
    {
        return;
    }
    // 子组件未处理的点击会传到这里，不当作点击遮罩，避免误关闭
    if (childAt(event->position().toPoint()) != nullptr)
    {
        return;
    }
    d->_offsetAnimation->stop();
    d->_isPressed = true;
    d->_isDragging = false;
    d->_pressPos = event->position();
    d->_pressOffset = d->_offset;
}

void ShadImageViewer::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadImageViewer);
    event->accept();
    const QPointF pos = event->position();
    if (d->_isPressed)
    {
        if (!d->_isDragging && d->_state == ShadImageViewerPrivate::ImageState::Ready && (pos - d->_pressPos).manhattanLength() >= QApplication::startDragDistance())
        {
            d->_isDragging = true;
            d->_isFitMode = false;
        }
        if (d->_isDragging)
        {
            d->_offset = d->_pressOffset + (pos - d->_pressPos);
            update();
        }
    }
    d->updateCursor(pos);
}

void ShadImageViewer::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadImageViewer);
    event->accept();
    if (event->button() != Qt::LeftButton || !d->_isPressed)
    {
        return;
    }
    d->_isPressed = false;
    const QPointF pos = event->position();
    if (d->_isDragging)
    {
        d->_isDragging = false;
        // 松开后回弹到允许的范围内
        d->animateOffsetTo(d->clampedOffset(d->_offset));
        d->updateChrome();
    }
    else if (!d->isImageAt(pos))
    {
        d->requestClose();
        return;
    }
    d->updateCursor(pos);
}

void ShadImageViewer::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_D(ShadImageViewer);
    event->accept();
    if (event->button() != Qt::LeftButton || !d->_isOpen || d->_isClosing)
    {
        return;
    }
    if (d->isImageAt(event->position()))
    {
        d->toggleFit(event->position());
    }
}

void ShadImageViewer::contextMenuEvent(QContextMenuEvent* event)
{
    Q_D(ShadImageViewer);
    event->accept();
    if (!d->_isOpen || d->_isClosing || d->_state != ShadImageViewerPrivate::ImageState::Ready)
    {
        return;
    }
    if (event->reason() == QContextMenuEvent::Keyboard)
    {
        d->_contextMenu->popupAt(mapToGlobal(d->imageRect().center().toPoint()));
        return;
    }
    if (d->isImageAt(event->pos()))
    {
        d->_contextMenu->popupAt(event->globalPos());
    }
}

void ShadImageViewer::wheelEvent(QWheelEvent* event)
{
    Q_D(ShadImageViewer);
    // 不把滚轮传给被遮住的组件
    event->accept();
    if (!d->_isOpen || d->_isClosing || d->_state != ShadImageViewerPrivate::ImageState::Ready)
    {
        return;
    }
    const bool isZoom = event->modifiers().testFlag(Qt::ControlModifier);
    const bool isTrackpad = event->phase() != Qt::NoScrollPhase || !event->pixelDelta().isNull();
    if (isTrackpad && !isZoom)
    {
        const QPointF delta = event->pixelDelta().isNull() ? QPointF(event->angleDelta()) / 8.0 : QPointF(event->pixelDelta());
        const QPointF offset = d->clampedOffset(d->_offset + delta);
        if (offset != d->_offset)
        {
            d->_offsetAnimation->stop();
            d->_offset = offset;
            d->_isFitMode = false;
            update();
        }
        return;
    }
    const QPoint angle = event->angleDelta();
    const int delta = angle.y() != 0 ? angle.y() : angle.x();
    if (delta == 0)
    {
        return;
    }
    d->zoomBy(qPow(ShadImageViewerPrivate::kZoomStep, delta / 120.0), event->position());
}

void ShadImageViewer::paintEvent(QPaintEvent* event)
{
    Q_D(ShadImageViewer);
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, qRound(255 * ShadImageViewerPrivate::kMaskAlpha * d->_appear)));
    if (d->_appear <= 0.0)
    {
        return;
    }
    const QRectF area = d->contentRect();
    if (d->_state == ShadImageViewerPrivate::ImageState::Ready && !d->_current.levels.isEmpty())
    {
        const qreal appearScale = ShadImageViewerPrivate::kInitialScale + (1.0 - ShadImageViewerPrivate::kInitialScale) * d->_appear;
        const QSizeF size = d->_current.size * (d->_scale * appearScale);
        // 选取最接近目标尺寸的缩小级别，避免锯齿
        const qreal targetWidth = size.width() * devicePixelRatioF();
        int level = 0;
        while (level + 1 < d->_current.levels.size() && d->_current.levels.at(level + 1).width() >= targetWidth)
        {
            ++level;
        }
        const QPixmap& pixmap = d->_current.levels.at(level);
        painter.save();
        painter.setOpacity(d->_appear);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(area.center() + d->_offset);
        // 翻转按屏幕方向生效，不受旋转影响
        painter.scale(d->_isFlippedHorizontally ? -1.0 : 1.0, d->_isFlippedVertically ? -1.0 : 1.0);
        painter.rotate(d->_rotation);
        painter.drawPixmap(QRectF(-size.width() / 2.0, -size.height() / 2.0, size.width(), size.height()), pixmap, QRectF(pixmap.rect()));
        painter.restore();
        return;
    }
    if (d->_state == ShadImageViewerPrivate::ImageState::Failed)
    {
        painter.setOpacity(d->_appear);
        painter.setRenderHint(QPainter::Antialiasing);
        const QColor color(255, 255, 255, 153);
        const QFont textFont = font();
        const QFontMetricsF metrics(textFont);
        const qreal totalHeight = ShadImageViewerPrivate::kErrorIconSize + ShadImageViewerPrivate::kErrorGap + metrics.height();
        const QPointF center = area.center();
        const QRectF iconRect(center.x() - ShadImageViewerPrivate::kErrorIconSize / 2.0, center.y() - totalHeight / 2.0, ShadImageViewerPrivate::kErrorIconSize, ShadImageViewerPrivate::kErrorIconSize);
        ShadIcon::paint(&painter, iconRect, ShadIconType::ImageOff, color, ShadImageViewerPrivate::kErrorIconSize);
        const QRectF textRect(area.left(), iconRect.bottom() + ShadImageViewerPrivate::kErrorGap, area.width(), metrics.height());
        painter.setFont(textFont);
        painter.setPen(color);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, tr("图片加载失败"));
    }
}
