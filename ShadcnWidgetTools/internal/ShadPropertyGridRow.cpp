#include "ShadPropertyGridRow.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kFontSize = 14;
    constexpr int kPaddingX = 12;
    constexpr int kEditorGap = 8;
    constexpr int kSplitHitWidth = 4;
} // namespace

ShadPropertyGridRow::ShadPropertyGridRow(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFixedHeight(kRowHeight);
    ShadStyleHelper::applyFont(this, kFontSize, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadPropertyGridRow::~ShadPropertyGridRow()
{
}

void ShadPropertyGridRow::setLabel(const QString& label)
{
    if (_label == label)
    {
        return;
    }
    _label = label;
    update();
}

QString ShadPropertyGridRow::getLabel() const
{
    return _label;
}

void ShadPropertyGridRow::setEditor(QWidget* editor)
{
    if (_editor == editor)
    {
        return;
    }
    if (!_editor.isNull())
    {
        ShadStyleHelper::clearFocusWithin(_editor);
        _editor->hide();
        _editor->deleteLater();
    }
    _editor = editor;
    if (editor != nullptr)
    {
        editor->setParent(this);
        editor->show();
    }
    layoutEditor();
}

QWidget* ShadPropertyGridRow::getEditor() const
{
    return _editor.data();
}

void ShadPropertyGridRow::setLabelWidth(int width)
{
    if (_labelWidth == width)
    {
        return;
    }
    _labelWidth = width;
    layoutEditor();
    update();
}

int ShadPropertyGridRow::effectiveLabelWidth() const
{
    return qBound(kMinLabelWidth, _labelWidth, qMax(kMinLabelWidth, width() - kMinEditorWidth));
}

void ShadPropertyGridRow::setCurrent(bool current)
{
    if (_isCurrent == current)
    {
        return;
    }
    _isCurrent = current;
    update();
}

bool ShadPropertyGridRow::isCurrent() const
{
    return _isCurrent;
}

QSize ShadPropertyGridRow::sizeHint() const
{
    return QSize(_labelWidth + kMinEditorWidth, kRowHeight);
}

QSize ShadPropertyGridRow::minimumSizeHint() const
{
    return QSize(kMinLabelWidth + kMinEditorWidth, kRowHeight);
}

bool ShadPropertyGridRow::event(QEvent* event)
{
    if (event->type() == QEvent::LayoutRequest)
    {
        layoutEditor();
    }
    return QWidget::event(event);
}

void ShadPropertyGridRow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        layoutEditor();
        update();
    }
    QWidget::changeEvent(event);
}

void ShadPropertyGridRow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isNearSplit(event->position().toPoint().x()))
    {
        _isDragging = true;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        Q_EMIT pressed();
    }
    QWidget::mousePressEvent(event);
}

void ShadPropertyGridRow::mouseMoveEvent(QMouseEvent* event)
{
    const int x = event->position().toPoint().x();
    if (_isDragging)
    {
        Q_EMIT labelWidthDragged(logicalX(x));
        event->accept();
        return;
    }
    if (isNearSplit(x))
    {
        setCursor(Qt::SplitHCursor);
    }
    else
    {
        unsetCursor();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadPropertyGridRow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _isDragging)
    {
        _isDragging = false;
        if (!isNearSplit(event->position().toPoint().x()))
        {
            unsetCursor();
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadPropertyGridRow::leaveEvent(QEvent* event)
{
    if (!_isDragging)
    {
        unsetCursor();
    }
    QWidget::leaveEvent(event);
}

void ShadPropertyGridRow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutEditor();
}

void ShadPropertyGridRow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const Qt::LayoutDirection direction = layoutDirection();
    const int labelWidth = effectiveLabelWidth();
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing);
    if (_isCurrent)
    {
        painter.fillRect(rect(), shadTheme->getColor(ShadThemeType::Muted));
    }
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    painter.fillRect(QRect(0, height() - 1, width(), 1), borderColor);
    const QRect splitRect = ShadStyleHelper::visualRect(direction, rect(), QRect(labelWidth, 0, 1, height()));
    painter.fillRect(splitRect, borderColor);
    const QRect labelRect = ShadStyleHelper::visualRect(direction, rect(), QRect(kPaddingX, 0, qMax(0, labelWidth - 2 * kPaddingX), height()));
    painter.setFont(font());
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft) | Qt::AlignVCenter;
    painter.drawText(labelRect, static_cast<int>(alignment), ShadStyleHelper::elideText(font(), _label, labelRect.width()));
}

int ShadPropertyGridRow::logicalX(int x) const
{
    return layoutDirection() == Qt::RightToLeft ? width() - x : x;
}

bool ShadPropertyGridRow::isNearSplit(int x) const
{
    return qAbs(logicalX(x) - effectiveLabelWidth()) <= kSplitHitWidth;
}

void ShadPropertyGridRow::layoutEditor()
{
    if (_editor.isNull())
    {
        return;
    }
    const int left = effectiveLabelWidth() + kEditorGap - ShadStyleHelper::kFocusRingWidth;
    const int right = kPaddingX - ShadStyleHelper::kFocusRingWidth;
    const QSize hint = _editor->sizeHint();
    const int editorHeight = qMin(hint.height(), height());
    int editorWidth = qMax(0, width() - left - right);
    const QSizePolicy::Policy policy = _editor->sizePolicy().horizontalPolicy();
    if (policy == QSizePolicy::Fixed || policy == QSizePolicy::Maximum)
    {
        editorWidth = qMin(editorWidth, hint.width());
    }
    const QRect logicalRect(left, (height() - editorHeight) / 2, editorWidth, editorHeight);
    _editor->setGeometry(ShadStyleHelper::visualRect(layoutDirection(), rect(), logicalRect));
}
