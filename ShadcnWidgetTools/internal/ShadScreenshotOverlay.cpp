#include "ShadScreenshotOverlay.h"

#include <QAbstractButton>
#include <QApplication>
#include <QChar>
#include <QColor>
#include <QCursor>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMap>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextDocument>
#include <QWindow>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadScreenshotAnnotation.h"
#include "ShadScreenshotToolbar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr qreal kMaskAlpha = 0.5;
    constexpr int kHighlightBorderWidth = 2;
    constexpr int kHandleSize = 6;
    constexpr int kHitTolerance = 6;
    constexpr int kMinSelectionSize = 4;
    constexpr int kLabelGap = 6;
    constexpr int kLabelPaddingX = 6;
    constexpr int kLabelHeight = 22;
    constexpr int kLabelTextPixelSize = 12;
    constexpr int kLabelAlpha = 180;
    constexpr int kToolbarGap = 6;
    constexpr int kTextFramePadding = 2;
    constexpr int kTextExtraHeight = 4;
    constexpr int kTextMinChars = 4;
    constexpr qreal kTextFrameDash = 3.0;

    QRect toPhysical(const QRect& rect, qreal ratio)
    {
        return QRectF(rect.x() * ratio, rect.y() * ratio, rect.width() * ratio, rect.height() * ratio).toAlignedRect();
    }
}

ShadScreenshotOverlay::ShadScreenshotOverlay(QScreen* screen, const QPixmap& screenshot, QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint)
{
    const QRect screenGeometry = screen->geometry();
    _screenshot = screenshot;
    _screenshot.setDevicePixelRatio(1.0);
    _image = _screenshot.toImage();
    _ratio = screenGeometry.width() > 0 ? static_cast<qreal>(_screenshot.width()) / screenGeometry.width() : 1.0;

    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    setLayoutDirection(Qt::LeftToRight);

    _toolbar = new ShadScreenshotToolbar(this);
    _toolbar->hide();
    connect(_toolbar, &ShadScreenshotToolbar::toolChanged, this, [this]() { handleToolChanged(); });
    connect(_toolbar, &ShadScreenshotToolbar::styleChanged, this, [this]() { handleStyleChanged(); });
    connect(_toolbar, &ShadScreenshotToolbar::layoutChanged, this, [this]() { updateToolbar(); });
    connect(_toolbar, &ShadScreenshotToolbar::undoClicked, this, [this]() { undo(); });
    connect(_toolbar, &ShadScreenshotToolbar::cancelClicked, this, &ShadScreenshotOverlay::canceled);
    connect(_toolbar, &ShadScreenshotToolbar::saveClicked, this, [this]() { Q_EMIT saveRequested(selectedImage()); });
    connect(_toolbar, &ShadScreenshotToolbar::copyClicked, this, [this]() { Q_EMIT copyRequested(selectedImage()); });
    connect(_toolbar, &ShadScreenshotToolbar::pinClicked, this, [this]() { requestPin(); });
    _pinButton = new ShadButton(this);
    _pinButton->setVariant(ShadButtonType::Secondary);
    _pinButton->setButtonSize(ShadButtonType::IconSmall);
    _pinButton->setShadIcon(ShadIconType::Pin);
    _pinButton->setFocusPolicy(Qt::NoFocus);
    _pinButton->setCursor(Qt::ArrowCursor);
    _pinButton->hide();
    connect(_pinButton, &QAbstractButton::clicked, this, [this]() { requestPin(); });
    retranslate();
    connect(_toolbar, &ShadScreenshotToolbar::doneClicked, this, [this]() { Q_EMIT doneRequested(selectedImage()); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });

    winId();
    if (QWindow* window = windowHandle())
    {
        window->setScreen(screen);
    }
    setGeometry(screenGeometry);
}

ShadScreenshotOverlay::~ShadScreenshotOverlay()
{
    if (_textEditor != nullptr)
    {
        _textEditor->removeEventFilter(this);
    }
}

void ShadScreenshotOverlay::setWindowRects(const QList<QRect>& rects)
{
    _windowRects.clear();
    const QPoint origin = geometry().topLeft();
    for (const QRect& rect : rects)
    {
        const QRect localRect = rect.translated(-origin).intersected(this->rect());
        if (!localRect.isEmpty())
        {
            _windowRects.append(localRect);
        }
    }
}

void ShadScreenshotOverlay::setLocked(bool locked)
{
    if (_isLocked == locked)
    {
        return;
    }
    _isLocked = locked;
    _dragMode = DragMode::None;
    if (locked)
    {
        update(_highlightRect);
        _highlightRect = QRect();
    }
    updateToolbar();
}

void ShadScreenshotOverlay::setCornerPinButtonVisible(bool visible)
{
    if (_isCornerPinButtonVisible == visible)
    {
        return;
    }
    _isCornerPinButtonVisible = visible;
    updateToolbar();
}

void ShadScreenshotOverlay::syncWithCursor()
{
    updateHighlight(mapFromGlobal(QCursor::pos()));
}

bool ShadScreenshotOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _textEditor && event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
    {
        commitText();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShadScreenshotOverlay::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadScreenshotOverlay::enterEvent(QEnterEvent* event)
{
    activateWindow();
    if (!isEditingText())
    {
        setFocus(Qt::OtherFocusReason);
    }
    QWidget::enterEvent(event);
}

void ShadScreenshotOverlay::keyPressEvent(QKeyEvent* event)
{
    const bool hasSelection = !_selection.isEmpty();
    if (event->matches(QKeySequence::Undo))
    {
        event->accept();
        undo();
        return;
    }
    if (event->matches(QKeySequence::Copy))
    {
        event->accept();
        if (hasSelection)
        {
            Q_EMIT copyRequested(selectedImage());
        }
        return;
    }
    if (event->matches(QKeySequence::Save))
    {
        event->accept();
        if (hasSelection)
        {
            Q_EMIT saveRequested(selectedImage());
        }
        return;
    }
    switch (event->key())
    {
        case Qt::Key_Escape:
            event->accept();
            Q_EMIT canceled();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            event->accept();
            if (hasSelection)
            {
                Q_EMIT doneRequested(selectedImage());
            }
            return;
        default:
            break;
    }
    QWidget::keyPressEvent(event);
}

void ShadScreenshotOverlay::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    const QPoint position = event->position().toPoint();
    if (event->button() == Qt::RightButton)
    {
        if (isEditingText())
        {
            commitText();
            return;
        }
        if (isAnnotating())
        {
            _toolbar->setTool(ShadScreenshotAnnotation::Tool::None);
            return;
        }
        if (_selection.isEmpty())
        {
            Q_EMIT canceled();
            return;
        }
        _dragMode = DragMode::None;
        clearAnnotations();
        setSelection(QRect());
        updateHighlight(position);
        updateCursorShape(position);
        return;
    }
    if (event->button() != Qt::LeftButton || _isLocked)
    {
        return;
    }
    if (isAnnotating())
    {
        commitText();
        if (!_selection.contains(position))
        {
            return;
        }
        const ShadScreenshotAnnotation::Tool tool = _toolbar->currentTool();
        if (tool == ShadScreenshotAnnotation::Tool::Text)
        {
            startText(position);
            return;
        }
        _current = ShadScreenshotAnnotation();
        _current.tool = tool;
        _current.color = _toolbar->currentColor();
        _current.size = _toolbar->currentSize();
        if (tool == ShadScreenshotAnnotation::Tool::Mosaic)
        {
            _current.blockSize = _toolbar->currentMosaicBlockSize();
            ensureMosaic(_current.blockSize);
        }
        _current.points = {QPointF(position)};
        _isDrawing = true;
        update(_selection);
        return;
    }
    _pressPosition = position;
    _dragStartSelection = _selection;
    _dragEdges = edgesAt(position);
    if (_dragEdges != NoEdge)
    {
        _dragMode = DragMode::Resizing;
    }
    else if (_selection.contains(position))
    {
        _dragMode = DragMode::Moving;
    }
    else
    {
        _dragMode = DragMode::Pending;
    }
    updateToolbar();
}

void ShadScreenshotOverlay::mouseMoveEvent(QMouseEvent* event)
{
    event->accept();
    const QPoint position = event->position().toPoint();
    if (_isDrawing)
    {
        const QPointF point(position);
        switch (_current.tool)
        {
            case ShadScreenshotAnnotation::Tool::Rectangle:
            case ShadScreenshotAnnotation::Tool::Ellipse:
            case ShadScreenshotAnnotation::Tool::Arrow:
                if (_current.points.size() < 2)
                {
                    _current.points.append(point);
                }
                else
                {
                    _current.points.last() = point;
                }
                break;
            case ShadScreenshotAnnotation::Tool::Pen:
            case ShadScreenshotAnnotation::Tool::Mosaic:
                _current.points.append(point);
                break;
            case ShadScreenshotAnnotation::Tool::None:
            case ShadScreenshotAnnotation::Tool::Text:
                break;
        }
        update(_selection);
        return;
    }
    const QPoint delta = position - _pressPosition;
    switch (_dragMode)
    {
        case DragMode::None:
            updateHighlight(position);
            updateCursorShape(position);
            break;
        case DragMode::Pending:
            if (delta.manhattanLength() >= QApplication::startDragDistance())
            {
                _dragMode = DragMode::Creating;
                clearAnnotations();
                setSelection(QRect(_pressPosition, position).normalized());
            }
            break;
        case DragMode::Creating:
            setSelection(QRect(_pressPosition, position).normalized());
            break;
        case DragMode::Moving:
            {
                QRect moved = _dragStartSelection.translated(delta);
                moved.moveLeft(qBound(0, moved.left(), qMax(0, width() - moved.width())));
                moved.moveTop(qBound(0, moved.top(), qMax(0, height() - moved.height())));
                setSelection(moved);
                break;
            }
        case DragMode::Resizing:
            {
                QRect resized = _dragStartSelection;
                if ((_dragEdges & LeftEdge) != 0)
                {
                    resized.setLeft(_dragStartSelection.left() + delta.x());
                }
                if ((_dragEdges & RightEdge) != 0)
                {
                    resized.setRight(_dragStartSelection.right() + delta.x());
                }
                if ((_dragEdges & TopEdge) != 0)
                {
                    resized.setTop(_dragStartSelection.top() + delta.y());
                }
                if ((_dragEdges & BottomEdge) != 0)
                {
                    resized.setBottom(_dragStartSelection.bottom() + delta.y());
                }
                setSelection(resized.normalized());
                break;
            }
    }
}

void ShadScreenshotOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    event->accept();
    if (_isDrawing)
    {
        _isDrawing = false;
        if (!_current.isEmpty())
        {
            _annotations.append(_current);
            _toolbar->setUndoEnabled(true);
        }
        _current = ShadScreenshotAnnotation();
        update(_selection);
        return;
    }
    const DragMode mode = _dragMode;
    _dragMode = DragMode::None;
    if (mode == DragMode::Pending && _selection.isEmpty() && !_highlightRect.isEmpty())
    {
        const QRect target = _highlightRect;
        setSelection(target);
    }
    else if (mode == DragMode::Creating && (_selection.width() < kMinSelectionSize || _selection.height() < kMinSelectionSize))
    {
        setSelection(QRect());
    }
    const QPoint position = event->position().toPoint();
    updateHighlight(position);
    updateCursorShape(position);
    updateToolbar();
}

void ShadScreenshotOverlay::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
    if (!isAnnotating() && event->button() == Qt::LeftButton && _selection.contains(event->position().toPoint()))
    {
        Q_EMIT doneRequested(selectedImage());
    }
}

void ShadScreenshotOverlay::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    const QRect dirtyRect = event->rect();
    const QRectF sourceRect(dirtyRect.x() * _ratio, dirtyRect.y() * _ratio, dirtyRect.width() * _ratio, dirtyRect.height() * _ratio);
    painter.drawPixmap(QRectF(dirtyRect), _screenshot, sourceRect);
    const QRect clearRect = _selection.isEmpty() ? _highlightRect : _selection;
    const QColor maskColor(0, 0, 0, qRound(255 * kMaskAlpha));
    for (const QRect& maskRect : QRegion(dirtyRect).subtracted(QRegion(clearRect)))
    {
        painter.fillRect(maskRect, maskColor);
    }
    const QColor accentColor = shadTheme->getColor(ShadThemeType::Destructive);
    if (_selection.isEmpty())
    {
        if (_highlightRect.isEmpty())
        {
            return;
        }
        const QRect& area = _highlightRect;
        const int border = qMin(kHighlightBorderWidth, qMin(area.width(), area.height()) / 2);
        painter.fillRect(QRect(area.left(), area.top(), area.width(), border), accentColor);
        painter.fillRect(QRect(area.left(), area.bottom() - border + 1, area.width(), border), accentColor);
        painter.fillRect(QRect(area.left(), area.top() + border, border, area.height() - 2 * border), accentColor);
        painter.fillRect(QRect(area.right() - border + 1, area.top() + border, border, area.height() - 2 * border), accentColor);
        return;
    }

    const QRectF screenRect(rect());
    painter.save();
    painter.setClipRect(_selection, Qt::IntersectClip);
    for (const ShadScreenshotAnnotation& annotation : std::as_const(_annotations))
    {
        annotation.paint(&painter, _mosaics, screenRect);
    }
    if (_isDrawing)
    {
        _current.paint(&painter, _mosaics, screenRect);
    }
    painter.restore();

    painter.setPen(QPen(accentColor, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(_selection.adjusted(0, 0, -1, -1));
    if (!isAnnotating())
    {
        const int left = _selection.left();
        const int right = _selection.right();
        const int top = _selection.top();
        const int bottom = _selection.bottom();
        const int centerX = (left + right) / 2;
        const int centerY = (top + bottom) / 2;
        const QList<QPoint> handles{QPoint(left, top), QPoint(centerX, top), QPoint(right, top), QPoint(left, centerY), QPoint(right, centerY), QPoint(left, bottom), QPoint(centerX, bottom), QPoint(right, bottom)};
        for (const QPoint& handle : handles)
        {
            painter.fillRect(QRect(handle.x() - kHandleSize / 2, handle.y() - kHandleSize / 2, kHandleSize, kHandleSize), accentColor);
        }
    }

    if (isEditingText())
    {
        QPen framePen(QColor(Qt::white), 1.0, Qt::CustomDashLine);
        framePen.setDashPattern({kTextFrameDash, kTextFrameDash});
        painter.setPen(framePen);
        painter.drawRect(_textFrame.adjusted(0, 0, -1, -1));
    }

    const QRect labelRect = sizeLabelRect(_selection);
    painter.setRenderHint(QPainter::Antialiasing, true);
    ShadStyleHelper::fillRoundedRect(&painter, QRectF(labelRect), shadTheme->getRadius(ShadThemeType::Small), QColor(0, 0, 0, kLabelAlpha));
    QFont labelFont = font();
    labelFont.setPixelSize(kLabelTextPixelSize);
    labelFont.setWeight(QFont::Medium);
    painter.setFont(labelFont);
    painter.setPen(QColor(Qt::white));
    painter.drawText(labelRect, Qt::AlignCenter, sizeText(_selection));
}

bool ShadScreenshotOverlay::isAnnotating() const
{
    return _toolbar->currentTool() != ShadScreenshotAnnotation::Tool::None;
}

bool ShadScreenshotOverlay::isEditingText() const
{
    return _textEditor != nullptr && !_textEditor->isHidden();
}

int ShadScreenshotOverlay::edgesAt(const QPoint& position) const
{
    if (_selection.isEmpty() || isAnnotating())
    {
        return NoEdge;
    }
    if (!_selection.adjusted(-kHitTolerance, -kHitTolerance, kHitTolerance, kHitTolerance).contains(position))
    {
        return NoEdge;
    }
    const int right = _selection.x() + _selection.width();
    const int bottom = _selection.y() + _selection.height();
    int edges = NoEdge;
    if (qAbs(position.x() - _selection.x()) <= kHitTolerance)
    {
        edges |= LeftEdge;
    }
    else if (qAbs(position.x() - right) <= kHitTolerance)
    {
        edges |= RightEdge;
    }
    if (qAbs(position.y() - _selection.y()) <= kHitTolerance)
    {
        edges |= TopEdge;
    }
    else if (qAbs(position.y() - bottom) <= kHitTolerance)
    {
        edges |= BottomEdge;
    }
    return edges;
}

void ShadScreenshotOverlay::setSelection(const QRect& selection)
{
    const QRect bounded = selection.intersected(rect());
    if (bounded == _selection)
    {
        return;
    }
    const bool hadSelection = !_selection.isEmpty();
    update(decorationRect(_selection));
    _selection = bounded;
    update(decorationRect(_selection));
    if (!_selection.isEmpty() && !_highlightRect.isEmpty())
    {
        update(_highlightRect);
        _highlightRect = QRect();
    }
    const bool hasSelection = !_selection.isEmpty();
    if (hadSelection != hasSelection)
    {
        Q_EMIT selectionChanged(hasSelection);
    }
    updateToolbar();
}

void ShadScreenshotOverlay::updateHighlight(const QPoint& position)
{
    QRect highlightRect;
    if (_selection.isEmpty() && !_isLocked && rect().contains(position))
    {
        for (const QRect& windowRect : std::as_const(_windowRects))
        {
            if (windowRect.contains(position))
            {
                highlightRect = windowRect;
                break;
            }
        }
    }
    if (highlightRect == _highlightRect)
    {
        return;
    }
    update(_highlightRect);
    update(highlightRect);
    _highlightRect = highlightRect;
}

void ShadScreenshotOverlay::updateCursorShape(const QPoint& position)
{
    if (isAnnotating())
    {
        if (!_selection.contains(position))
        {
            setCursor(Qt::ArrowCursor);
            return;
        }
        setCursor(_toolbar->currentTool() == ShadScreenshotAnnotation::Tool::Text ? Qt::IBeamCursor : Qt::CrossCursor);
        return;
    }
    const int edges = edgesAt(position);
    const bool isLeftTop = (edges & LeftEdge) != 0 && (edges & TopEdge) != 0;
    const bool isRightBottom = (edges & RightEdge) != 0 && (edges & BottomEdge) != 0;
    const bool isLeftBottom = (edges & LeftEdge) != 0 && (edges & BottomEdge) != 0;
    const bool isRightTop = (edges & RightEdge) != 0 && (edges & TopEdge) != 0;
    Qt::CursorShape shape = Qt::CrossCursor;
    if (isLeftTop || isRightBottom)
    {
        shape = Qt::SizeFDiagCursor;
    }
    else if (isLeftBottom || isRightTop)
    {
        shape = Qt::SizeBDiagCursor;
    }
    else if ((edges & (LeftEdge | RightEdge)) != 0)
    {
        shape = Qt::SizeHorCursor;
    }
    else if ((edges & (TopEdge | BottomEdge)) != 0)
    {
        shape = Qt::SizeVerCursor;
    }
    else if (_selection.contains(position))
    {
        shape = Qt::SizeAllCursor;
    }
    setCursor(shape);
}

void ShadScreenshotOverlay::updateToolbar()
{
    if (_selection.isEmpty() || _dragMode != DragMode::None || _isLocked)
    {
        _toolbar->hide();
        _pinButton->hide();
        return;
    }
    const QSize size = _toolbar->sizeHint();
    _toolbar->resize(size);
    const int selectionRight = _selection.x() + _selection.width();
    const int selectionBottom = _selection.y() + _selection.height();
    const int x = qBound(0, selectionRight - size.width(), qMax(0, width() - size.width()));
    int y = selectionBottom + kToolbarGap;
    if (y + size.height() > height())
    {
        y = _selection.y() - kToolbarGap - size.height();
    }
    if (y < 0)
    {
        y = selectionBottom - kToolbarGap - size.height();
    }
    _toolbar->move(x, qMax(0, y));
    _toolbar->show();
    _toolbar->raise();

    if (!_isCornerPinButtonVisible)
    {
        _pinButton->hide();
        return;
    }
    const QSize pinSize = _pinButton->sizeHint();
    QRect pinRect(selectionRight - pinSize.width(), _selection.y() - pinSize.height(), pinSize.width(), pinSize.height());
    if (pinRect.top() < 0 || pinRect.intersects(_toolbar->geometry()))
    {
        pinRect.moveTop(_selection.y());
    }
    pinRect.moveLeft(qBound(0, pinRect.left(), qMax(0, width() - pinSize.width())));
    _pinButton->setGeometry(pinRect);
    _pinButton->show();
    _pinButton->raise();
}

void ShadScreenshotOverlay::requestPin()
{
    const QImage image = selectedImage();
    Q_EMIT pinRequested(image, mapToGlobal(_selection.topLeft()));
}

void ShadScreenshotOverlay::retranslate()
{
    _pinButton->setToolTip(tr("钉在屏幕上"));
}

void ShadScreenshotOverlay::handleToolChanged()
{
    commitText();
    _isDrawing = false;
    _dragMode = DragMode::None;
    update(decorationRect(_selection));
    updateCursorShape(mapFromGlobal(QCursor::pos()));
}

void ShadScreenshotOverlay::handleStyleChanged()
{
    if (!isEditingText())
    {
        return;
    }
    _textAnnotation.color = _toolbar->currentColor();
    _textAnnotation.size = _toolbar->currentSize();
    updateTextEditor();
    _textEditor->setFocus(Qt::OtherFocusReason);
}

void ShadScreenshotOverlay::ensureMosaic(int blockSize)
{
    if (_mosaics.contains(blockSize) || _image.isNull())
    {
        return;
    }
    const int block = qMax(1, qRound(blockSize * _ratio));
    const QImage small = _image.scaled(qMax(1, _image.width() / block), qMax(1, _image.height() / block), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    _mosaics.insert(blockSize, QPixmap::fromImage(small.scaled(_image.size(), Qt::IgnoreAspectRatio, Qt::FastTransformation)));
}

void ShadScreenshotOverlay::startText(const QPoint& position)
{
    if (_textEditor == nullptr)
    {
        _textEditor = new QPlainTextEdit(this);
        _textEditor->setFrameShape(QFrame::NoFrame);
        _textEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _textEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _textEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
        _textEditor->setContentsMargins(0, 0, 0, 0);
        _textEditor->document()->setDocumentMargin(0);
        _textEditor->setAutoFillBackground(false);
        _textEditor->viewport()->setAutoFillBackground(false);
        _textEditor->installEventFilter(this);
        connect(_textEditor, &QPlainTextEdit::textChanged, this, [this]() { updateTextEditor(); });
    }
    _textAnnotation = ShadScreenshotAnnotation();
    _textAnnotation.tool = ShadScreenshotAnnotation::Tool::Text;
    _textAnnotation.color = _toolbar->currentColor();
    _textAnnotation.size = _toolbar->currentSize();
    _textAnnotation.points = {QPointF(position)};
    _textEditor->clear();
    updateTextEditor();
    _textEditor->show();
    _textEditor->raise();
    _toolbar->raise();
    _textEditor->setFocus(Qt::OtherFocusReason);
}

void ShadScreenshotOverlay::updateTextEditor()
{
    if (_textEditor == nullptr || _textAnnotation.points.isEmpty())
    {
        return;
    }
    const QFont font = ShadScreenshotAnnotation::textFont(_textAnnotation.size);
    QPalette palette = _textEditor->palette();
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Text, _textAnnotation.color);
    _textEditor->setPalette(palette);
    _textEditor->setFont(font);
    const QFontMetrics metrics(font);
    const QStringList lines = _textEditor->toPlainText().split(QLatin1Char('\n'));
    int textWidth = kTextMinChars * metrics.averageCharWidth();
    for (const QString& line : lines)
    {
        textWidth = qMax(textWidth, metrics.horizontalAdvance(line) + 2 * metrics.averageCharWidth());
    }
    const QPoint topLeft = _textAnnotation.points.first().toPoint();
    const QRect oldFrame = _textFrame;
    _textEditor->setGeometry(topLeft.x(), topLeft.y(), textWidth, static_cast<int>(lines.size()) * metrics.height() + kTextExtraHeight);
    _textFrame = _textEditor->geometry().adjusted(-kTextFramePadding, -kTextFramePadding, kTextFramePadding, kTextFramePadding);
    update(oldFrame);
    update(_textFrame);
}

void ShadScreenshotOverlay::commitText()
{
    if (!isEditingText())
    {
        return;
    }
    ShadScreenshotAnnotation annotation = _textAnnotation;
    annotation.text = _textEditor->toPlainText();
    _textEditor->hide();
    _textEditor->clear();
    _textAnnotation = ShadScreenshotAnnotation();
    update(_textFrame);
    _textFrame = QRect();
    if (!annotation.isEmpty())
    {
        _annotations.append(annotation);
        _toolbar->setUndoEnabled(true);
        update(_selection);
    }
    setFocus(Qt::OtherFocusReason);
}

void ShadScreenshotOverlay::clearAnnotations()
{
    if (_annotations.isEmpty())
    {
        return;
    }
    _annotations.clear();
    _toolbar->setUndoEnabled(false);
    update(_selection);
}

void ShadScreenshotOverlay::undo()
{
    commitText();
    if (_annotations.isEmpty())
    {
        return;
    }
    _annotations.removeLast();
    _toolbar->setUndoEnabled(!_annotations.isEmpty());
    update(_selection);
}

QRect ShadScreenshotOverlay::sizeLabelRect(const QRect& selection) const
{
    QFont labelFont = font();
    labelFont.setPixelSize(kLabelTextPixelSize);
    labelFont.setWeight(QFont::Medium);
    const int labelWidth = ShadStyleHelper::textWidth(labelFont, sizeText(selection)) + 2 * kLabelPaddingX;
    const int x = qBound(0, selection.x(), qMax(0, width() - labelWidth));
    int y = selection.y() - kLabelGap - kLabelHeight;
    if (y < 0)
    {
        y = selection.y() + kLabelGap;
    }
    return QRect(x, y, labelWidth, kLabelHeight);
}

QRect ShadScreenshotOverlay::decorationRect(const QRect& selection) const
{
    if (selection.isEmpty())
    {
        return QRect();
    }
    return selection.adjusted(-kHandleSize, -kHandleSize, kHandleSize, kHandleSize).united(sizeLabelRect(selection));
}

QString ShadScreenshotOverlay::sizeText(const QRect& selection) const
{
    const QRect physical = toPhysical(selection, _ratio);
    return QStringLiteral("%1 × %2").arg(physical.width()).arg(physical.height());
}

QImage ShadScreenshotOverlay::selectedImage()
{
    commitText();
    const QRect source = toPhysical(_selection, _ratio).intersected(_image.rect());
    QImage image = _image.copy(source);
    if (!_annotations.isEmpty())
    {
        QPainter painter(&image);
        painter.translate(-source.topLeft());
        painter.scale(_ratio, _ratio);
        const QRectF screenRect(rect());
        for (const ShadScreenshotAnnotation& annotation : std::as_const(_annotations))
        {
            annotation.paint(&painter, _mosaics, screenRect);
        }
    }
    image.setDevicePixelRatio(_ratio);
    return image;
}
