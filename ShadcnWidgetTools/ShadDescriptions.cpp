#include "ShadDescriptions.h"

#include <QColor>
#include <QEvent>
#include <QFont>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>

#include "ShadDef.h"
#include "ShadDescriptionsPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kTextFontSize = 14;
    constexpr int kTitleFontSize = 16;
    constexpr int kPreferredColumnWidth = 240;
    constexpr int kMinimumColumnWidth = 64;

    // 创建标签与内容使用的 QLabel，内容可选中复制
    QLabel* createLabel(QWidget* parent, bool isSelectable)
    {
        QLabel* label = new QLabel(parent);
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeading | Qt::AlignTop);
        if (isSelectable)
        {
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        }
        ShadStyleHelper::applyFont(label, kTextFontSize, QFont::Normal);
        label->show();
        return label;
    }
} // namespace

ShadDescriptions::ShadDescriptions(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDescriptionsPrivate())
{
    Q_D(ShadDescriptions);
    d->q_ptr = this;
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    ShadStyleHelper::applyFont(this, kTextFontSize, QFont::Normal);
    d->_titleLabel = createLabel(this, false);
    ShadStyleHelper::applyFont(d->_titleLabel, kTitleFontSize, QFont::DemiBold);
    d->_titleLabel->hide();
    d->updateColors();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->updateColors(); });
}

ShadDescriptions::ShadDescriptions(const QString& title, QWidget* parent)
    : ShadDescriptions(parent)
{
    setTitle(title);
}

ShadDescriptions::~ShadDescriptions()
{
}

void ShadDescriptions::setTitle(const QString& title)
{
    Q_D(ShadDescriptions);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    d->_titleLabel->setText(title);
    d->updateContent();
}

QString ShadDescriptions::getTitle() const
{
    Q_D(const ShadDescriptions);
    return d->_title;
}

void ShadDescriptions::setExtra(QWidget* widget)
{
    Q_D(ShadDescriptions);
    if (d->_extra == widget)
    {
        return;
    }
    if (!d->_extra.isNull())
    {
        ShadStyleHelper::clearFocusWithin(d->_extra);
        d->_extra->hide();
        d->_extra->deleteLater();
    }
    d->_extra = widget;
    if (widget != nullptr)
    {
        // 操作区不参与布局，由 layoutChildren 手动放置
        widget->setParent(this);
        widget->show();
    }
    d->updateContent();
}

QWidget* ShadDescriptions::getExtra() const
{
    Q_D(const ShadDescriptions);
    return d->_extra.data();
}

void ShadDescriptions::setColumnCount(int count)
{
    Q_D(ShadDescriptions);
    const int columns = qMax(1, count);
    if (d->_columnCount == columns)
    {
        return;
    }
    d->_columnCount = columns;
    d->updateContent();
}

int ShadDescriptions::getColumnCount() const
{
    Q_D(const ShadDescriptions);
    return d->_columnCount;
}

void ShadDescriptions::setItemLayout(ShadDescriptionsType::Layout layout)
{
    Q_D(ShadDescriptions);
    if (d->_itemLayout == layout)
    {
        return;
    }
    d->_itemLayout = layout;
    d->updateContent();
}

ShadDescriptionsType::Layout ShadDescriptions::getItemLayout() const
{
    Q_D(const ShadDescriptions);
    return d->_itemLayout;
}

void ShadDescriptions::setBordered(bool bordered)
{
    Q_D(ShadDescriptions);
    if (d->_isBordered == bordered)
    {
        return;
    }
    d->_isBordered = bordered;
    d->updateContent();
}

bool ShadDescriptions::isBordered() const
{
    Q_D(const ShadDescriptions);
    return d->_isBordered;
}

void ShadDescriptions::setDescriptionsSize(ShadDescriptionsType::Size size)
{
    Q_D(ShadDescriptions);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    d->updateContent();
}

ShadDescriptionsType::Size ShadDescriptions::getDescriptionsSize() const
{
    Q_D(const ShadDescriptions);
    return d->_size;
}

void ShadDescriptions::setColonVisible(bool visible)
{
    Q_D(ShadDescriptions);
    if (d->_isColonVisible == visible)
    {
        return;
    }
    d->_isColonVisible = visible;
    for (const ShadDescriptionsPrivate::Item& item : d->_items)
    {
        item.labelLabel->setText(d->labelText(item.label));
    }
    d->updateContent();
}

bool ShadDescriptions::isColonVisible() const
{
    Q_D(const ShadDescriptions);
    return d->_isColonVisible;
}

int ShadDescriptions::addItem(const QString& label, const QString& text, int span)
{
    Q_D(ShadDescriptions);
    ShadDescriptionsPrivate::Item item;
    item.label = label;
    item.span = qMax(1, span);
    item.labelLabel = createLabel(this, false);
    item.labelLabel->setText(d->labelText(label));
    item.labelLabel->setVisible(!label.isEmpty());
    item.textLabel = createLabel(this, true);
    item.textLabel->setText(text);
    d->_items.append(item);
    d->updateColors();
    d->updateContent();
    return static_cast<int>(d->_items.size()) - 1;
}

int ShadDescriptions::addItem(const QString& label, QWidget* content, int span)
{
    Q_D(ShadDescriptions);
    ShadDescriptionsPrivate::Item item;
    item.label = label;
    item.span = qMax(1, span);
    item.labelLabel = createLabel(this, false);
    item.labelLabel->setText(d->labelText(label));
    item.labelLabel->setVisible(!label.isEmpty());
    item.widget = content;
    if (content != nullptr)
    {
        // 内容组件不参与布局，由 layoutChildren 手动放置
        content->setParent(this);
        content->show();
    }
    d->_items.append(item);
    d->updateColors();
    d->updateContent();
    return static_cast<int>(d->_items.size()) - 1;
}

void ShadDescriptions::setItemLabel(int index, const QString& label)
{
    Q_D(ShadDescriptions);
    ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    if (item == nullptr || item->label == label)
    {
        return;
    }
    item->label = label;
    item->labelLabel->setText(d->labelText(label));
    item->labelLabel->setVisible(!label.isEmpty());
    d->updateContent();
}

QString ShadDescriptions::getItemLabel(int index) const
{
    Q_D(const ShadDescriptions);
    const ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    return item != nullptr ? item->label : QString();
}

void ShadDescriptions::setItemText(int index, const QString& text)
{
    Q_D(ShadDescriptions);
    ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    if (item == nullptr || item->textLabel == nullptr || item->textLabel->text() == text)
    {
        return;
    }
    item->textLabel->setText(text);
    d->updateContent();
}

QString ShadDescriptions::getItemText(int index) const
{
    Q_D(const ShadDescriptions);
    const ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    return item != nullptr && item->textLabel != nullptr ? item->textLabel->text() : QString();
}

QWidget* ShadDescriptions::getItemWidget(int index) const
{
    Q_D(const ShadDescriptions);
    const ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    return item != nullptr ? item->widget.data() : nullptr;
}

void ShadDescriptions::setItemSpan(int index, int span)
{
    Q_D(ShadDescriptions);
    ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    const int value = qMax(1, span);
    if (item == nullptr || item->span == value)
    {
        return;
    }
    item->span = value;
    d->updateContent();
}

int ShadDescriptions::getItemSpan(int index) const
{
    Q_D(const ShadDescriptions);
    const ShadDescriptionsPrivate::Item* item = d->itemAt(index);
    return item != nullptr ? item->span : 0;
}

void ShadDescriptions::removeItem(int index)
{
    Q_D(ShadDescriptions);
    if (d->itemAt(index) == nullptr)
    {
        return;
    }
    const ShadDescriptionsPrivate::Item item = d->_items.takeAt(index);
    delete item.labelLabel;
    delete item.textLabel;
    if (!item.widget.isNull())
    {
        ShadStyleHelper::clearFocusWithin(item.widget);
        item.widget->hide();
        item.widget->deleteLater();
    }
    d->updateContent();
}

void ShadDescriptions::clear()
{
    Q_D(ShadDescriptions);
    while (!d->_items.isEmpty())
    {
        removeItem(static_cast<int>(d->_items.size()) - 1);
    }
}

int ShadDescriptions::count() const
{
    Q_D(const ShadDescriptions);
    return static_cast<int>(d->_items.size());
}

bool ShadDescriptions::hasHeightForWidth() const
{
    return true;
}

int ShadDescriptions::heightForWidth(int width) const
{
    Q_D(const ShadDescriptions);
    // 与实际排版宽度保持一致
    return qMax(d->computeLayout(qMin(width, maximumWidth())).totalHeight, minimumHeight());
}

QSize ShadDescriptions::sizeHint() const
{
    Q_D(const ShadDescriptions);
    const int columns = d->_columnCount;
    const int width = qMin(columns * kPreferredColumnWidth + (columns - 1) * d->metrics().columnGap, maximumWidth());
    return QSize(width, heightForWidth(width));
}

QSize ShadDescriptions::minimumSizeHint() const
{
    Q_D(const ShadDescriptions);
    // 高度由 heightForWidth 决定，这里只给出最小宽度
    const int columns = d->_columnCount;
    return QSize(columns * kMinimumColumnWidth + (columns - 1) * d->metrics().columnGap, minimumHeight());
}

bool ShadDescriptions::event(QEvent* event)
{
    Q_D(ShadDescriptions);
    if (event->type() == QEvent::LayoutRequest)
    {
        // 子组件尺寸变化时重新排版
        d->layoutChildren();
        updateGeometry();
    }
    else if (event->type() == QEvent::Show)
    {
        // 隐藏期间内容可能变化，显示时重新排版
        d->layoutChildren();
    }
    return QWidget::event(event);
}

void ShadDescriptions::changeEvent(QEvent* event)
{
    Q_D(ShadDescriptions);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutChildren();
    }
    QWidget::changeEvent(event);
}

void ShadDescriptions::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadDescriptions);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadDescriptions::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDescriptions);
    if (!d->_isBordered || d->_layout.tableRect.isEmpty())
    {
        return;
    }
    using namespace ShadStyleHelper;
    const QRect bounds = rect();
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF tableRect(visualRect(direction, bounds, d->_layout.tableRect));
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    QPainter painter(this);
    // 标签格底色裁剪到圆角内
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipPath(roundedRectPath(tableRect, radius));
    const QColor labelBackground = withAlpha(shadTheme->getColor(ShadThemeType::Muted), 0.5);
    for (const ShadDescriptionsPrivate::ItemGeometry& geometry : d->_layout.items)
    {
        if (!geometry.labelCell.isEmpty())
        {
            painter.fillRect(visualRect(direction, bounds, geometry.labelCell), labelBackground);
        }
    }
    painter.restore();
    // 分隔线交叉点只画一次，避免半透明边框色叠加加深
    const QRect logicalTable = d->_layout.tableRect;
    for (const ShadDescriptionsPrivate::ItemGeometry& geometry : d->_layout.items)
    {
        for (const QRect& cell : {geometry.labelCell, geometry.contentCell})
        {
            if (cell.isEmpty())
            {
                continue;
            }
            const bool hasEndLine = cell.right() + 1 < logicalTable.right();
            const bool hasBottomLine = cell.bottom() + 1 < logicalTable.bottom();
            if (hasEndLine)
            {
                painter.fillRect(visualRect(direction, bounds, QRect(cell.right() + 1, cell.top(), 1, cell.height())), borderColor);
            }
            if (hasBottomLine)
            {
                painter.fillRect(visualRect(direction, bounds, QRect(cell.left(), cell.bottom() + 1, cell.width() + (hasEndLine ? 1 : 0), 1)), borderColor);
            }
        }
    }
    painter.setRenderHint(QPainter::Antialiasing);
    strokeRoundedRect(&painter, tableRect, radius, borderColor);
}
