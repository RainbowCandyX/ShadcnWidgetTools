#include "ShadImageViewerPrivate.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QClipboard>
#include <QCursor>
#include <QDir>
#include <QEasingCurve>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QIODevice>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPair>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QStandardPaths>
#include <QString>
#include <QThreadPool>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <limits>

#include "ShadFrameAnimation.h"
#include "ShadIconType.h"
#include "ShadImageLoadTask.h"
#include "ShadImageViewer.h"
#include "ShadImageViewerButton.h"
#include "ShadImageViewerToolbar.h"
#include "ShadSpinner.h"
#include "ShadStyleHelper.h"
#include "ShadWindowCorner.h"

ShadImageViewerPrivate::ShadImageViewerPrivate()
{
}

ShadImageViewerPrivate::~ShadImageViewerPrivate()
{
}

QWidget* ShadImageViewerPrivate::resolveHost() const
{
    Q_Q(const ShadImageViewer);
    QWidget* anchor = !_anchor.isNull() ? _anchor.data() : q->parentWidget();
    if (anchor != nullptr)
    {
        return anchor->window();
    }
    return QApplication::activeWindow();
}

void ShadImageViewerPrivate::attachHost(QWidget* host)
{
    Q_Q(ShadImageViewer);
    if (q->parentWidget() != host)
    {
        q->setParent(host);
    }
    if (_host == host)
    {
        return;
    }
    if (!_host.isNull())
    {
        _host->removeEventFilter(this);
    }
    _host = host;
    host->installEventFilter(this);
}

void ShadImageViewerPrivate::detachHost()
{
    if (!_host.isNull())
    {
        _host->removeEventFilter(this);
    }
    _host = nullptr;
}

void ShadImageViewerPrivate::restack()
{
    Q_Q(ShadImageViewer);
    q->raise();
    if (_host.isNull())
    {
        return;
    }
    for (ShadWindowCorner* corner : _host->findChildren<ShadWindowCorner*>(QString(), Qt::FindDirectChildrenOnly))
    {
        corner->raise();
    }
}

int ShadImageViewerPrivate::topInset() const
{
    if (_host.isNull() || _host->isFullScreen())
    {
        return 0;
    }
    // 只有自绘标题栏的窗口提供 titleBarHeight，系统标题栏在客户区以外
    const QVariant value = _host->property("titleBarHeight");
    return value.isValid() ? qMax(0, value.toInt()) : 0;
}

int ShadImageViewerPrivate::imageCount() const
{
    return !_paths.isEmpty() ? static_cast<int>(_paths.size()) : static_cast<int>(_pixmaps.size());
}

void ShadImageViewerPrivate::resetSource()
{
    ++_sourceSerial;
    _cache.clear();
    _pending.clear();
    _failed.clear();
    _current = ImageEntry();
    _state = ImageState::Empty;
}

void ShadImageViewerPrivate::showIndex(int index, bool force)
{
    Q_Q(ShadImageViewer);
    const int count = imageCount();
    const int bounded = count > 0 ? qBound(0, index, count - 1) : -1;
    const bool isChanged = bounded != _currentIndex;
    if (!isChanged && !force)
    {
        return;
    }
    _currentIndex = bounded;
    _current = ImageEntry();
    _state = bounded < 0 ? ImageState::Empty : ImageState::Loading;
    resetTransform();
    // 只在打开后加载，避免预先加载用不到的图片
    if (bounded >= 0 && _isOpen)
    {
        requestImage(bounded);
        if (!_paths.isEmpty())
        {
            // 预先加载相邻的图片，切换时可以立即显示
            requestImage(bounded - 1);
            requestImage(bounded + 1);
        }
    }
    updateChrome();
    updateCursor(q->mapFromGlobal(QCursor::pos()));
    q->update();
    if (isChanged)
    {
        Q_EMIT q->currentIndexChanged(bounded);
    }
}

void ShadImageViewerPrivate::requestImage(int index)
{
    Q_Q(ShadImageViewer);
    if (index < 0 || index >= imageCount())
    {
        return;
    }
    const bool isCurrent = index == _currentIndex;
    if (const ImageEntry* cached = _cache.object(index))
    {
        if (isCurrent)
        {
            _current = *cached;
            _state = ImageState::Ready;
            applyFit();
        }
        return;
    }
    if (_failed.contains(index))
    {
        if (isCurrent)
        {
            _state = ImageState::Failed;
        }
        return;
    }
    if (!_pixmaps.isEmpty())
    {
        const QPixmap& pixmap = _pixmaps.at(index);
        if (pixmap.isNull())
        {
            _failed.insert(index);
            if (isCurrent)
            {
                _state = ImageState::Failed;
            }
            return;
        }
        const ImageEntry entry = createEntry(ShadImageLoadTask::buildLevels(pixmap.toImage()), pixmap.deviceIndependentSize());
        _cache.insert(index, new ImageEntry(entry), entryCost(entry));
        if (isCurrent)
        {
            _current = entry;
            _state = ImageState::Ready;
            applyFit();
        }
        return;
    }
    if (_pending.contains(index))
    {
        return;
    }
    _pending.insert(index);
    const quint64 serial = _sourceSerial;
    const QPointer<ShadImageViewer> viewer(q);
    // 回调在主线程执行，丢弃预览已销毁或来源已更换后的结果
    ShadImageLoadTask* task = new ShadImageLoadTask(_paths.at(index), [viewer, serial, index](const QList<QImage>& levels, const QString& errorString)
    {
        if (viewer.isNull())
        {
            return;
        }
        viewer->d_func()->onImageLoaded(serial, index, levels, errorString);
    });
    QThreadPool::globalInstance()->start(task);
}

void ShadImageViewerPrivate::onImageLoaded(quint64 serial, int index, const QList<QImage>& levels, const QString& errorString)
{
    Q_Q(ShadImageViewer);
    if (serial != _sourceSerial)
    {
        return;
    }
    _pending.remove(index);
    const bool isCurrent = index == _currentIndex && _state == ImageState::Loading;
    if (levels.isEmpty())
    {
        _failed.insert(index);
        if (index >= 0 && index < _paths.size())
        {
            qWarning("图片加载失败：%s，%s", qUtf8Printable(_paths.at(index)), qUtf8Printable(errorString));
        }
        if (isCurrent)
        {
            _state = ImageState::Failed;
            updateChrome();
            q->update();
        }
        return;
    }
    const ImageEntry entry = createEntry(levels, QSizeF(levels.first().size()));
    _cache.insert(index, new ImageEntry(entry), entryCost(entry));
    if (!isCurrent)
    {
        return;
    }
    _current = entry;
    _state = ImageState::Ready;
    applyFit();
    updateChrome();
    updateCursor(q->mapFromGlobal(QCursor::pos()));
    q->update();
}

ShadImageViewerPrivate::ImageEntry ShadImageViewerPrivate::createEntry(const QList<QImage>& levels, const QSizeF& size)
{
    ImageEntry entry;
    entry.size = size;
    for (const QImage& image : levels)
    {
        entry.levels.append(QPixmap::fromImage(image));
    }
    return entry;
}

int ShadImageViewerPrivate::entryCost(const ImageEntry& entry)
{
    qint64 bytes = 0;
    for (const QPixmap& pixmap : entry.levels)
    {
        bytes += static_cast<qint64>(pixmap.width()) * pixmap.height() * 4;
    }
    const qint64 cost = bytes / 1024 + 1;
    return static_cast<int>(qMin<qint64>(cost, std::numeric_limits<int>::max()));
}

QRectF ShadImageViewerPrivate::contentRect() const
{
    Q_Q(const ShadImageViewer);
    const qreal horizontal = imageCount() > 1 ? kNavigationInset : kEdgeMargin;
    const qreal top = topInset() + kVerticalInset;
    return QRectF(q->rect()).adjusted(horizontal, top, -horizontal, -kVerticalInset);
}

QSizeF ShadImageViewerPrivate::rotatedSize() const
{
    const bool isTransposed = _rotation == 90 || _rotation == 270;
    return isTransposed ? _current.size.transposed() : _current.size;
}

qreal ShadImageViewerPrivate::fitScale() const
{
    const QSizeF size = rotatedSize();
    const QRectF area = contentRect();
    if (size.isEmpty() || area.width() <= 0.0 || area.height() <= 0.0)
    {
        return 1.0;
    }
    // 小图保持实际大小，不放大
    return qMin(qMin(area.width() / size.width(), area.height() / size.height()), 1.0);
}

QRectF ShadImageViewerPrivate::imageRect() const
{
    const QSizeF size = rotatedSize() * _scale;
    const QPointF center = contentRect().center() + _offset;
    return QRectF(center.x() - size.width() / 2.0, center.y() - size.height() / 2.0, size.width(), size.height());
}

bool ShadImageViewerPrivate::isImageAt(const QPointF& pos) const
{
    return _state == ImageState::Ready && imageRect().contains(pos);
}

void ShadImageViewerPrivate::resetTransform()
{
    _rotation = 0;
    _isFlippedHorizontally = false;
    _isFlippedVertically = false;
    applyFit();
}

void ShadImageViewerPrivate::applyFit()
{
    _offsetAnimation->stop();
    _scale = _state == ImageState::Ready ? fitScale() : 1.0;
    _offset = QPointF();
    _isFitMode = true;
}

void ShadImageViewerPrivate::zoomTo(qreal scale, const QPointF& anchor)
{
    Q_Q(ShadImageViewer);
    if (_state != ImageState::Ready)
    {
        return;
    }
    // 超大图片的适应窗口缩放可能低于下限，此时允许缩小到适应窗口
    const qreal minimum = qMin(kMinScale, fitScale());
    const qreal bounded = qBound(minimum, scale, kMaxScale);
    if (qFuzzyCompare(bounded, _scale))
    {
        return;
    }
    _offsetAnimation->stop();
    // 保持 anchor 下方的图片位置不变
    const QPointF center = contentRect().center();
    const QPointF relative = anchor - center - _offset;
    _offset = anchor - center - relative * (bounded / _scale);
    _scale = bounded;
    _isFitMode = false;
    _offset = clampedOffset(_offset);
    updateChrome();
    updateCursor(q->mapFromGlobal(QCursor::pos()));
    q->update();
}

void ShadImageViewerPrivate::zoomBy(qreal factor, const QPointF& anchor)
{
    zoomTo(_scale * factor, anchor);
}

void ShadImageViewerPrivate::toggleFit(const QPointF& anchor)
{
    Q_Q(ShadImageViewer);
    if (_state != ImageState::Ready)
    {
        return;
    }
    if (_isFitMode)
    {
        zoomTo(1.0, anchor);
        return;
    }
    applyFit();
    updateChrome();
    updateCursor(q->mapFromGlobal(QCursor::pos()));
    q->update();
}

void ShadImageViewerPrivate::rotateBy(int degrees)
{
    Q_Q(ShadImageViewer);
    if (_state != ImageState::Ready)
    {
        return;
    }
    _rotation = ((_rotation + degrees) % 360 + 360) % 360;
    if (_isFitMode)
    {
        applyFit();
    }
    else
    {
        _offsetAnimation->stop();
        _offset = clampedOffset(_offset);
    }
    updateChrome();
    q->update();
}

QPointF ShadImageViewerPrivate::clampedOffset(const QPointF& offset) const
{
    const QSizeF size = rotatedSize() * _scale;
    const QRectF area = contentRect();
    // 图片小于显示区域时居中，大于时边缘不超过显示区域
    const qreal limitX = qMax(0.0, (size.width() - area.width()) / 2.0);
    const qreal limitY = qMax(0.0, (size.height() - area.height()) / 2.0);
    return QPointF(qBound(-limitX, offset.x(), limitX), qBound(-limitY, offset.y(), limitY));
}

void ShadImageViewerPrivate::animateOffsetTo(const QPointF& target)
{
    Q_Q(ShadImageViewer);
    _offsetAnimation->stop();
    if ((target - _offset).manhattanLength() < 0.5)
    {
        _offset = target;
        q->update();
        return;
    }
    _offsetFrom = _offset;
    _offsetTo = target;
    _offsetAnimation->start();
}

void ShadImageViewerPrivate::navigate(int step)
{
    if (step == 0 || _currentIndex < 0)
    {
        return;
    }
    const int target = _currentIndex + step;
    if (target < 0 || target >= imageCount())
    {
        return;
    }
    showIndex(target, false);
}

int ShadImageViewerPrivate::keyStep(int key) const
{
    Q_Q(const ShadImageViewer);
    const int forward = q->layoutDirection() == Qt::RightToLeft ? -1 : 1;
    if (key == Qt::Key_Left)
    {
        return -forward;
    }
    return key == Qt::Key_Right ? forward : 0;
}

void ShadImageViewerPrivate::layoutChrome()
{
    Q_Q(ShadImageViewer);
    const QRect bounds = q->rect();
    const Qt::LayoutDirection direction = q->layoutDirection();
    const int buttonTop = topInset() + kEdgeMargin;
    _closeButton->setGeometry(ShadStyleHelper::visualRect(direction, bounds, QRect(bounds.width() - kEdgeMargin - kRoundButtonSize, buttonTop, kRoundButtonSize, kRoundButtonSize)));
    const QRectF area = contentRect();
    const int navigationTop = qRound(area.center().y() - kRoundButtonSize / 2.0);
    _prevButton->setGeometry(ShadStyleHelper::visualRect(direction, bounds, QRect(kEdgeMargin, navigationTop, kRoundButtonSize, kRoundButtonSize)));
    _nextButton->setGeometry(ShadStyleHelper::visualRect(direction, bounds, QRect(bounds.width() - kEdgeMargin - kRoundButtonSize, navigationTop, kRoundButtonSize, kRoundButtonSize)));
    const QSize toolbarSize = _toolbar->sizeHint();
    _toolbar->setGeometry((bounds.width() - toolbarSize.width()) / 2, bounds.height() - kToolbarBottom - toolbarSize.height(), toolbarSize.width(), toolbarSize.height());
    _spinner->setGeometry(qRound(area.center().x() - kSpinnerSize / 2.0), qRound(area.center().y() - kSpinnerSize / 2.0), kSpinnerSize, kSpinnerSize);
}

void ShadImageViewerPrivate::updateChrome()
{
    Q_Q(ShadImageViewer);
    const int count = imageCount();
    const bool hasMultiple = count > 1;
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    // RTL 时上一张在右侧
    _prevButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft);
    _nextButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight);
    _prevButton->setVisible(hasMultiple);
    _nextButton->setVisible(hasMultiple);
    _prevButton->setEnabled(_currentIndex > 0);
    _nextButton->setEnabled(_currentIndex >= 0 && _currentIndex < count - 1);

    _toolbar->setVisible(_isToolbarVisible);
    _toolbar->setCounterVisible(hasMultiple);
    _toolbar->setCounterText(QStringLiteral("%1 / %2").arg(_currentIndex + 1).arg(count));
    const bool isReady = _state == ImageState::Ready;
    const qreal minimum = isReady ? qMin(kMinScale, fitScale()) : kMinScale;
    _zoomOutButton->setEnabled(isReady && _scale > minimum && !qFuzzyCompare(_scale, minimum));
    _zoomInButton->setEnabled(isReady && _scale < kMaxScale && !qFuzzyCompare(_scale, kMaxScale));
    _zoomLabel->setText(isReady ? QStringLiteral("%1%").arg(qRound(_scale * 100.0)) : QStringLiteral("-"));
    _fitButton->setEnabled(isReady);
    _rotateLeftButton->setEnabled(isReady);
    _rotateRightButton->setEnabled(isReady);
    _flipHorizontalButton->setEnabled(isReady);
    _flipVerticalButton->setEnabled(isReady);
    _saveButton->setEnabled(isReady);
    // 图标表示点击后切换到的状态
    _fitButton->setShadIcon(_isFitMode ? ShadIconType::Expand : ShadIconType::Shrink);

    const QList<QPair<ShadImageViewerButton*, QString>> texts{
        {_closeButton, ShadImageViewer::tr("关闭")},
        {_prevButton, ShadImageViewer::tr("上一张")},
        {_nextButton, ShadImageViewer::tr("下一张")},
        {_zoomOutButton, ShadImageViewer::tr("缩小")},
        {_zoomInButton, ShadImageViewer::tr("放大")},
        {_fitButton, _isFitMode ? ShadImageViewer::tr("实际大小") : ShadImageViewer::tr("适应窗口")},
        {_rotateLeftButton, ShadImageViewer::tr("向左旋转")},
        {_rotateRightButton, ShadImageViewer::tr("向右旋转")},
        {_flipHorizontalButton, ShadImageViewer::tr("水平翻转")},
        {_flipVerticalButton, ShadImageViewer::tr("垂直翻转")},
        {_saveButton, ShadImageViewer::tr("保存图片")},
    };
    for (const QPair<ShadImageViewerButton*, QString>& text : texts)
    {
        text.first->setToolTip(text.second);
        text.first->setAccessibleName(text.second);
    }
    _copyAction->setText(ShadImageViewer::tr("复制图片"));
    _saveAction->setText(ShadImageViewer::tr("保存图片…"));

    _spinner->setVisible(_state == ImageState::Loading);
    layoutChrome();
}

void ShadImageViewerPrivate::updateCursor(const QPointF& pos)
{
    Q_Q(ShadImageViewer);
    if (_isDragging)
    {
        q->setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (isImageAt(pos))
    {
        q->setCursor(Qt::OpenHandCursor);
        return;
    }
    q->unsetCursor();
}

void ShadImageViewerPrivate::setAppear(qreal appear)
{
    Q_Q(ShadImageViewer);
    _appear = qBound(0.0, appear, 1.0);
    for (QGraphicsOpacityEffect* effect : _chromeEffects)
    {
        effect->setOpacity(_appear);
    }
    q->update();
}

void ShadImageViewerPrivate::requestClose()
{
    if (!_isOpen || _isClosing)
    {
        return;
    }
    _isClosing = true;
    _isPressed = false;
    _isDragging = false;
    _offsetAnimation->stop();
    _appearAnimation->stop();
    _appearAnimation->setDuration(kCloseDurationMs);
    _appearAnimation->setEasingCurve(QEasingCurve::InCubic);
    _appearAnimation->setStartValue(_appear);
    _appearAnimation->setEndValue(0.0);
    _appearAnimation->start();
}

void ShadImageViewerPrivate::finishClose()
{
    Q_Q(ShadImageViewer);
    _isOpen = false;
    _isClosing = false;
    QWidget* focus = QApplication::focusWidget();
    const bool hadFocus = focus == q || q->isAncestorOf(focus);
    ShadStyleHelper::clearFocusWithin(q);
    q->hide();
    q->unsetCursor();
    detachHost();
    if (hadFocus && !_focusBeforeOpen.isNull())
    {
        _focusBeforeOpen->setFocus(Qt::OtherFocusReason);
    }
    _focusBeforeOpen = nullptr;
    // closed 的槽函数可能直接销毁预览，之后不再访问私有数据
    const bool isDeleteOnClose = _isDeleteOnClose;
    const QPointer<ShadImageViewer> guard(q);
    Q_EMIT q->closed();
    if (isDeleteOnClose && !guard.isNull())
    {
        guard->deleteLater();
    }
}

QImage ShadImageViewerPrivate::currentImage() const
{
    if (_state != ImageState::Ready || _current.levels.isEmpty())
    {
        return QImage();
    }
    return _current.levels.first().toImage();
}

void ShadImageViewerPrivate::copyImage()
{
    const QImage image = currentImage();
    if (image.isNull())
    {
        return;
    }
    QGuiApplication::clipboard()->setImage(image);
}

void ShadImageViewerPrivate::saveImage()
{
    Q_Q(ShadImageViewer);
    const QImage image = currentImage();
    if (image.isNull())
    {
        return;
    }
    const QString sourcePath = _currentIndex >= 0 && _currentIndex < _paths.size() ? _paths.at(_currentIndex) : QString();
    const QString fileName = sourcePath.isEmpty() ? QStringLiteral("image.png") : QFileInfo(sourcePath).fileName();
#ifdef Q_OS_WASM
    Q_UNUSED(q);
    QByteArray content;
    QFile sourceFile(sourcePath);
    if (!sourcePath.isEmpty() && sourceFile.open(QIODevice::ReadOnly))
    {
        content = sourceFile.readAll();
    }
    QString downloadName = fileName;
    if (content.isEmpty())
    {
        QBuffer buffer(&content);
        if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
        {
            qWarning("编码图片失败: %s", qUtf8Printable(fileName));
            return;
        }
        downloadName = QFileInfo(fileName).completeBaseName() + QStringLiteral(".png");
    }
    QFileDialog::saveFileContent(content, downloadName);
    return;
#endif
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QString path = QFileDialog::getSaveFileName(q, ShadImageViewer::tr("保存图片"), QDir(directory).filePath(fileName), ShadImageViewer::tr("图片 (*.png *.jpg *.jpeg *.bmp *.webp);;所有文件 (*)"));
    if (path.isEmpty())
    {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty())
    {
        path += QStringLiteral(".png");
    }
    const QFileInfo sourceInfo(sourcePath);
    const QFileInfo targetInfo(path);
    const bool isCopy = !sourcePath.isEmpty() && sourceInfo.suffix().compare(targetInfo.suffix(), Qt::CaseInsensitive) == 0;
    if (isCopy && sourceInfo.absoluteFilePath() == targetInfo.absoluteFilePath())
    {
        return;
    }
    QThreadPool::globalInstance()->start([image, sourcePath, path, isCopy]()
    {
        bool isSaved = false;
        if (isCopy)
        {
            QFile::remove(path);
            isSaved = QFile::copy(sourcePath, path);
        }
        else
        {
            isSaved = image.save(path);
        }
        if (!isSaved)
        {
            qWarning("保存图片失败: %s", qUtf8Printable(path));
        }
    });
}

bool ShadImageViewerPrivate::eventFilter(QObject* watched, QEvent* event)
{
    Q_Q(ShadImageViewer);
    if (_host.isNull() || watched != _host.data())
    {
        return QObject::eventFilter(watched, event);
    }
    if (event->type() == QEvent::Resize && q->parentWidget() == _host)
    {
        q->setGeometry(_host->rect());
        if (_isFitMode)
        {
            applyFit();
        }
        else
        {
            _offset = clampedOffset(_offset);
        }
        updateChrome();
        q->update();
    }
    else if (event->type() == QEvent::ChildAdded)
    {
        // 子组件添加过程中不能调整层级，延后置顶
        QMetaObject::invokeMethod(this, [this]()
        {
            if (_isOpen)
            {
                restack();
            }
        }, Qt::QueuedConnection);
    }
    return QObject::eventFilter(watched, event);
}
