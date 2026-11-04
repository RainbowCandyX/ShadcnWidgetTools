#include "ShadTimeColumn.h"

#include <QAbstractAnimation>
#include <QChar>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QScrollBar>
#include <QString>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 选中后滚动到列顶的动画时长
    constexpr int kScrollDurationMs = 200;
    // 格子左右留出 p-1，圆角背景不贴边
    constexpr int kCellInset = 4;
} // namespace

// 列的内容：按行绘制 00、01、02……，处理悬停与点击
class ShadTimeColumnContent : public QWidget
{
    public:
        explicit ShadTimeColumnContent(ShadTimeColumn* column)
            : QWidget(column), _column(column)
        {
            setMouseTracking(true);
            setFocusPolicy(Qt::NoFocus);
            // 底部留出 kVisibleRows - 1 格的空白，最后一格也能滚动到列顶
            setFixedHeight((_column->_count + ShadTimeColumn::kVisibleRows - 1) * ShadTimeColumn::kCellHeight);
        }

    protected:
        void mouseMoveEvent(QMouseEvent* event) override
        {
            setHoverIndex(indexAt(event->position().toPoint().y()));
            QWidget::mouseMoveEvent(event);
        }

        void leaveEvent(QEvent* event) override
        {
            setHoverIndex(-1);
            QWidget::leaveEvent(event);
        }

        void mousePressEvent(QMouseEvent* event) override
        {
            const int index = indexAt(event->position().toPoint().y());
            if (event->button() != Qt::LeftButton || index < 0)
            {
                QWidget::mousePressEvent(event);
                return;
            }
            _column->_isFocusVisible = false;
            _column->setFocus(Qt::MouseFocusReason);
            _column->selectByUser(index);
            event->accept();
        }

        void paintEvent(QPaintEvent* event) override
        {
            using namespace ShadStyleHelper;
            const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
            const int cellHeight = ShadTimeColumn::kCellHeight;
            const int first = qMax(0, event->rect().top() / cellHeight);
            const int last = qMin(_column->_count - 1, event->rect().bottom() / cellHeight);
            QPainter painter(this);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
            painter.setFont(font());
            for (int index = first; index <= last; ++index)
            {
                const QRectF cell(kCellInset, index * cellHeight, width() - 2 * kCellInset, cellHeight);
                QColor textColor = shadTheme->getColor(ShadThemeType::Foreground);
                if (index == _column->_selectedValue)
                {
                    fillRoundedRect(&painter, cell, radius, shadTheme->getColor(ShadThemeType::Primary));
                    textColor = shadTheme->getColor(ShadThemeType::PrimaryForeground);
                }
                else if (index == _hoverIndex)
                {
                    fillRoundedRect(&painter, cell, radius, shadTheme->getColor(ShadThemeType::Accent));
                    textColor = shadTheme->getColor(ShadThemeType::AccentForeground);
                }
                painter.setPen(textColor);
                painter.drawText(cell, Qt::AlignCenter, QStringLiteral("%1").arg(index, 2, 10, QLatin1Char('0')));
                // 键盘聚焦时选中格显示 border-ring + ring-[3px]，向内收缩以免被列边缘裁剪
                if (index == _column->_selectedValue && _column->hasFocus() && _column->_isFocusVisible)
                {
                    const QRectF ringRect = cell.adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
                    strokeRoundedRect(&painter, ringRect, radius, shadTheme->getColor(ShadThemeType::Ring));
                    drawFocusRing(&painter, ringRect, radius, focusRingColor(false));
                }
            }
        }

    private:
        int indexAt(int y) const
        {
            const int index = y / ShadTimeColumn::kCellHeight;
            return (y >= 0 && index < _column->_count) ? index : -1;
        }

        void setHoverIndex(int index)
        {
            if (_hoverIndex == index)
            {
                return;
            }
            _hoverIndex = index;
            setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
            update();
        }

        ShadTimeColumn* _column{nullptr};
        int _hoverIndex{-1};
};

ShadTimeColumn::ShadTimeColumn(int count, QWidget* parent)
    : ShadScrollArea(parent), _count(qMax(1, count))
{
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(kColumnWidth, kVisibleRows * kCellHeight);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    _content = new ShadTimeColumnContent(this);
    setWidget(_content);
    // 滚轮一格滚动一行
    verticalScrollBar()->setSingleStep(kCellHeight);
    _scrollAnimation = new QVariantAnimation(this);
    _scrollAnimation->setDuration(kScrollDurationMs);
    _scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(_scrollAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) { verticalScrollBar()->setValue(value.toInt()); });
    connect(shadTheme, &ShadTheme::themeChanged, _content, [this]() { _content->update(); });
}

ShadTimeColumn::~ShadTimeColumn()
{
}

void ShadTimeColumn::setSelectedValue(int value)
{
    const int selected = (value >= 0 && value < _count) ? value : -1;
    if (_selectedValue == selected)
    {
        return;
    }
    _selectedValue = selected;
    _content->update();
}

int ShadTimeColumn::selectedValue() const
{
    return _selectedValue;
}

void ShadTimeColumn::scrollToValue(int value, bool animated)
{
    const int target = qBound(verticalScrollBar()->minimum(), qMax(0, value) * kCellHeight, verticalScrollBar()->maximum());
    _scrollAnimation->stop();
    if (!animated || !isVisible())
    {
        verticalScrollBar()->setValue(target);
        return;
    }
    _scrollAnimation->setStartValue(verticalScrollBar()->value());
    _scrollAnimation->setEndValue(target);
    _scrollAnimation->start();
}

bool ShadTimeColumn::isFocusVisible() const
{
    return _isFocusVisible;
}

void ShadTimeColumn::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Up:
            selectByUser(_selectedValue < 0 ? 0 : (_selectedValue - 1 + _count) % _count);
            break;
        case Qt::Key_Down:
            selectByUser(_selectedValue < 0 ? 0 : (_selectedValue + 1) % _count);
            break;
        case Qt::Key_Home:
            selectByUser(0);
            break;
        case Qt::Key_End:
            selectByUser(_count - 1);
            break;
        case Qt::Key_Left:
            Q_EMIT moveFocusRequested(-1);
            break;
        case Qt::Key_Right:
            Q_EMIT moveFocusRequested(1);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            Q_EMIT confirmRequested();
            break;
        default:
            // 其余按键（包括 Esc）交给父组件，浮层可以按 Esc 关闭
            event->ignore();
            return;
    }
    _isFocusVisible = true;
    _content->update();
    event->accept();
}

void ShadTimeColumn::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    _content->update();
    ShadScrollArea::focusInEvent(event);
}

void ShadTimeColumn::focusOutEvent(QFocusEvent* event)
{
    _content->update();
    ShadScrollArea::focusOutEvent(event);
}

void ShadTimeColumn::selectByUser(int value)
{
    setSelectedValue(value);
    scrollToValue(value, true);
    Q_EMIT valueSelected(value);
}
