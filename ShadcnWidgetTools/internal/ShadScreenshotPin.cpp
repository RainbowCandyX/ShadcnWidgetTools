#include "ShadScreenshotPin.h"

#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include "ShadContextMenu.h"
#include "ShadIconType.h"
#include "ShadScreenshotFile.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr qreal kMinScale = 0.1;
    constexpr qreal kMaxScale = 5.0;
    constexpr qreal kScaleStep = 1.1;
    constexpr qreal kWheelUnit = 120.0;
    constexpr qreal kMinOpacity = 0.2;
    constexpr qreal kOpacityStep = 0.1;
    constexpr int kHintDuration = 800;
    constexpr int kHintHeight = 24;
    constexpr int kHintPaddingX = 8;
    constexpr int kHintTextPixelSize = 12;
    constexpr int kHintAlpha = 180;
}

ShadScreenshotPin::ShadScreenshotPin(const QImage& image, const QPoint& globalPosition)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    _image = image;
    const qreal ratio = image.devicePixelRatio() > 0.0 ? image.devicePixelRatio() : 1.0;
    _baseSize = QSizeF(image.width() / ratio, image.height() / ratio);

    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::SizeAllCursor);
    setMinimumSize(1, 1);

    _hintTimer = new QTimer(this);
    _hintTimer->setSingleShot(true);
    _hintTimer->setInterval(kHintDuration);
    connect(_hintTimer, &QTimer::timeout, this, [this]() { update(); });

    _menu = new ShadContextMenu(this);
    _menu->attachTo(this);
    connect(_menu, &ShadContextMenu::aboutToPopup, this, [this]() { rebuildMenu(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });

    setGeometry(QRect(globalPosition, _baseSize.toSize().expandedTo(QSize(1, 1))));
}

ShadScreenshotPin::~ShadScreenshotPin()
{
}

void ShadScreenshotPin::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void ShadScreenshotPin::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ShadScreenshotPin::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        event->accept();
        close();
        return;
    }
    if (event->matches(QKeySequence::Copy))
    {
        event->accept();
        copyImage();
        return;
    }
    if (event->matches(QKeySequence::Save))
    {
        event->accept();
        saveImage();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadScreenshotPin::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    event->accept();
    _isDragging = true;
    _pressGlobalPosition = event->globalPosition().toPoint();
    _pressWindowPosition = pos();
}

void ShadScreenshotPin::mouseMoveEvent(QMouseEvent* event)
{
    if (!_isDragging || !event->buttons().testFlag(Qt::LeftButton))
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    event->accept();
    move(_pressWindowPosition + event->globalPosition().toPoint() - _pressGlobalPosition);
}

void ShadScreenshotPin::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _isDragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadScreenshotPin::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    event->accept();
    close();
}

void ShadScreenshotPin::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0)
    {
        QWidget::wheelEvent(event);
        return;
    }
    event->accept();
    if (event->modifiers().testFlag(Qt::ControlModifier))
    {
        setWindowOpacity(qBound(kMinOpacity, windowOpacity() + (delta > 0 ? kOpacityStep : -kOpacityStep), 1.0));
        return;
    }
    setScale(_scale * qPow(kScaleStep, delta / kWheelUnit), event->position());
}

void ShadScreenshotPin::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(QRectF(rect()), _image);
    painter.setPen(QPen(shadTheme->getColor(_isHovered ? ShadThemeType::Destructive : ShadThemeType::Border), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5));
    if (!_hintTimer->isActive())
    {
        return;
    }
    QFont hintFont = font();
    hintFont.setPixelSize(kHintTextPixelSize);
    hintFont.setWeight(QFont::Medium);
    const QString text = QStringLiteral("%1%").arg(qRound(_scale * 100.0));
    const int hintWidth = ShadStyleHelper::textWidth(hintFont, text) + 2 * kHintPaddingX;
    const QRect hintRect((width() - hintWidth) / 2, (height() - kHintHeight) / 2, hintWidth, kHintHeight);
    painter.setRenderHint(QPainter::Antialiasing, true);
    ShadStyleHelper::fillRoundedRect(&painter, QRectF(hintRect), shadTheme->getRadius(ShadThemeType::Small), QColor(0, 0, 0, kHintAlpha));
    painter.setFont(hintFont);
    painter.setPen(QColor(Qt::white));
    painter.drawText(hintRect, Qt::AlignCenter, text);
}

void ShadScreenshotPin::setScale(qreal scale, const QPointF& anchor)
{
    const QPointF imagePoint = anchor / _scale;
    const QPointF globalAnchor = mapToGlobal(anchor);
    _scale = qBound(kMinScale, scale, kMaxScale);
    const QSize size = (_baseSize * _scale).toSize().expandedTo(QSize(1, 1));
    const QPointF topLeft = globalAnchor - imagePoint * _scale;
    setGeometry(QRect(topLeft.toPoint(), size));
    _hintTimer->start();
    update();
}

void ShadScreenshotPin::copyImage()
{
    QGuiApplication::clipboard()->setImage(_image);
}

void ShadScreenshotPin::saveImage()
{
    const QString filePath = ShadScreenshotFile::choosePath(this);
    if (filePath.isEmpty())
    {
        return;
    }
    const QPointer<ShadScreenshotPin> pin(this);
    ShadScreenshotFile::save(_image, filePath, [pin, filePath](bool isSaved)
    {
        if (pin.isNull())
        {
            return;
        }
        if (isSaved)
        {
            Q_EMIT pin->saved(filePath);
        }
        else
        {
            Q_EMIT pin->saveFailed(filePath);
        }
    });
}

void ShadScreenshotPin::rebuildMenu()
{
    _menu->clear();
    connect(_menu->addItem(tr("复制"), ShadIconType::Copy), &QAction::triggered, this, [this]() { copyImage(); });
    connect(_menu->addItem(tr("保存"), ShadIconType::Download), &QAction::triggered, this, [this]() { saveImage(); });
    connect(_menu->addItem(tr("恢复原始大小")), &QAction::triggered, this, [this]() { setScale(1.0, QPointF(0.0, 0.0)); });
    _menu->addSeparator();
    QAction* closeAction = _menu->addItem(tr("关闭"), ShadIconType::X);
    _menu->setItemDestructive(closeAction, true);
    connect(closeAction, &QAction::triggered, this, [this]() { close(); });
}
