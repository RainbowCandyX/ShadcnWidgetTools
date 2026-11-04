#include "ShadListDelegate.h"

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadListView.h"
#include "ShadStyleHelper.h"
#include "ShadTableDelegate.h"
#include "ShadTheme.h"

namespace
{
    // 焦点环线宽，与 ShadTreeView 一致
    constexpr qreal kFocusRingStroke = 2.0;
    constexpr int kCheckboxSize = 16;
    constexpr int kIconSize = 16;
    // 标题与描述的字号、行高与间距
    constexpr int kTitlePixelSize = 14;
    constexpr int kSmallDescriptionPixelSize = 12;
    constexpr qreal kTitleLineHeightRatio = 1.375;
    constexpr qreal kDescriptionLineHeightRatio = 1.5;
    constexpr int kTextGap = 4;
    constexpr int kTrailingPixelSize = 12;
} // namespace

ShadListDelegate::ShadListDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

ShadListDelegate::~ShadListDelegate()
{
}

int ShadListDelegate::rowHeight(ShadItemType::Size size, bool descriptionVisible)
{
    switch (size)
    {
        case ShadItemType::Small:
            return descriptionVisible ? 48 : 32;
        case ShadItemType::ExtraSmall:
            return 28;
        case ShadItemType::Medium:
            break;
    }
    return descriptionVisible ? 56 : 40;
}

void ShadListDelegate::setItemSize(ShadItemType::Size size)
{
    _itemSize = size;
}

ShadItemType::Size ShadListDelegate::getItemSize() const
{
    return _itemSize;
}

void ShadListDelegate::setDescriptionVisible(bool visible)
{
    _isDescriptionVisible = visible;
}

bool ShadListDelegate::isDescriptionVisible() const
{
    return _isDescriptionVisible;
}

void ShadListDelegate::setImageShape(ShadTableType::ImageShape shape)
{
    _imageShape = shape;
}

ShadTableType::ImageShape ShadListDelegate::getImageShape() const
{
    return _imageShape;
}

void ShadListDelegate::setHoveredIndex(const QModelIndex& index)
{
    _hoveredIndex = QPersistentModelIndex(index);
}

QModelIndex ShadListDelegate::getHoveredIndex() const
{
    return _hoveredIndex;
}

void ShadListDelegate::setFocusVisible(bool visible)
{
    _isFocusVisible = visible;
}

void ShadListDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    using namespace ShadStyleHelper;
    const QRect rect = option.rect;
    // 行内元素按 LTR 计算，再按布局方向镜像
    const Qt::LayoutDirection direction = option.direction;
    const bool isEnabled = option.state.testFlag(QStyle::State_Enabled) && index.flags().testFlag(Qt::ItemIsEnabled);
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    const bool isHovered = isEnabled && _hoveredIndex.isValid() && _hoveredIndex == index;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    if (!isEnabled)
    {
        painter->setOpacity(kDisabledOpacity);
    }

    const QRectF backgroundRect(rect);
    if (isSelected)
    {
        fillRoundedRect(painter, backgroundRect, radius, shadTheme->getColor(ShadThemeType::Accent));
    }
    else if (isHovered)
    {
        fillRoundedRect(painter, backgroundRect, radius, shadTheme->getColor(ShadThemeType::Muted));
    }
    // QListView 只在视图有焦点时给当前行加 State_HasFocus
    if (_isFocusVisible && option.state.testFlag(QStyle::State_HasFocus))
    {
        strokeRoundedRect(painter, backgroundRect, radius, shadTheme->getColor(ShadThemeType::Ring), kFocusRingStroke);
    }

    const int spacing = gap();
    int left = rect.left() + horizontalPadding();
    int right = rect.right() + 1 - horizontalPadding();
    if (hasCheckbox(index))
    {
        const Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
        drawCheckbox(painter, QRectF(checkboxRect(option)), state);
        left += kCheckboxSize + spacing;
    }

    // 标题颜色，图标与标题同色
    QColor titleColor = shadTheme->getColor(ShadThemeType::Foreground);
    const QVariant foreground = index.data(Qt::ForegroundRole);
    if (isSelected)
    {
        titleColor = shadTheme->getColor(ShadThemeType::AccentForeground);
    }
    else if (foreground.canConvert<QBrush>())
    {
        titleColor = foreground.value<QBrush>().color();
    }

    // 媒体区：图标优先，否则绘制图片
    const QVariant iconData = index.data(ShadListView::IconRole);
    const ShadIconType::IconName icon = iconData.isValid() ? static_cast<ShadIconType::IconName>(iconData.toInt()) : ShadIconType::None;
    if (icon != ShadIconType::None)
    {
        const QRect iconRect = visualRect(direction, rect, QRect(left, rect.top() + (rect.height() - kIconSize) / 2, kIconSize, kIconSize));
        ShadIcon::paint(painter, QRectF(iconRect), icon, titleColor, kIconSize);
        left += kIconSize + spacing;
    }
    else
    {
        const QVariant decoration = index.data(Qt::DecorationRole);
        const int side = imageSide();
        const QPixmap pixmap = decoration.isValid() ? ShadTableDelegate::renderedImage(decoration, QSize(side, side), _imageShape, painter->device()->devicePixelRatioF()) : QPixmap();
        if (!pixmap.isNull())
        {
            const QRect imageRect = visualRect(direction, rect, QRect(left, rect.top() + (rect.height() - side) / 2, side, side));
            painter->drawPixmap(imageRect, pixmap);
            left += side + spacing;
        }
    }

    // 结束侧文字限制宽度，避免挤占标题
    const QString trailingText = index.data(ShadListView::TrailingTextRole).toString();
    if (!trailingText.isEmpty())
    {
        QFont trailingFont = option.font;
        trailingFont.setPixelSize(kTrailingPixelSize);
        trailingFont.setWeight(QFont::Normal);
        const int trailingWidth = qMin(textWidth(trailingFont, trailingText), qMax(0, (right - left) / 2));
        const QRect trailingRect = visualRect(direction, rect, QRect(right - trailingWidth, rect.top(), trailingWidth, rect.height()));
        painter->setFont(trailingFont);
        painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter->drawText(trailingRect, visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter), elideText(trailingFont, trailingText, trailingWidth));
        right -= trailingWidth + spacing;
    }

    // 模型 FontRole 中显式设置的字体属性优先
    const int availableWidth = qMax(0, right - left);
    QFont titleFont = option.font;
    titleFont.setPixelSize(kTitlePixelSize);
    titleFont.setWeight(QFont::Medium);
    const QVariant fontData = index.data(Qt::FontRole);
    if (fontData.canConvert<QFont>())
    {
        titleFont = fontData.value<QFont>().resolve(titleFont);
    }
    const QString title = index.data(Qt::DisplayRole).toString();
    const QString description = showsDescription() ? index.data(ShadListView::DescriptionRole).toString() : QString();
    const Qt::Alignment alignment = visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter);
    if (description.isEmpty())
    {
        const QRect titleRect = visualRect(direction, rect, QRect(left, rect.top(), availableWidth, rect.height()));
        painter->setFont(titleFont);
        painter->setPen(titleColor);
        painter->drawText(titleRect, alignment, elideText(titleFont, title, availableWidth));
        painter->restore();
        return;
    }
    QFont descriptionFont = option.font;
    descriptionFont.setPixelSize(_itemSize == ShadItemType::Small ? kSmallDescriptionPixelSize : kTitlePixelSize);
    descriptionFont.setWeight(QFont::Normal);
    const int titleHeight = qRound(kTitlePixelSize * kTitleLineHeightRatio);
    const int descriptionHeight = qRound(descriptionFont.pixelSize() * kDescriptionLineHeightRatio);
    const int top = rect.top() + (rect.height() - titleHeight - kTextGap - descriptionHeight) / 2;
    const QRect titleRect = visualRect(direction, rect, QRect(left, top, availableWidth, titleHeight));
    const QRect descriptionRect = visualRect(direction, rect, QRect(left, top + titleHeight + kTextGap, availableWidth, descriptionHeight));
    painter->setFont(titleFont);
    painter->setPen(titleColor);
    painter->drawText(titleRect, alignment, elideText(titleFont, title, availableWidth));
    painter->setFont(descriptionFont);
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter->drawText(descriptionRect, alignment, elideText(descriptionFont, description, availableWidth));
    painter->restore();
}

QSize ShadListDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    // 所有行等高，宽度由视图决定
    return QSize(2 * horizontalPadding(), rowHeight(_itemSize, showsDescription()));
}

bool ShadListDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (model == nullptr || !hasCheckbox(index) || !index.flags().testFlag(Qt::ItemIsEnabled))
    {
        return false;
    }
    switch (event->type())
    {
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            {
                const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() != Qt::LeftButton || !checkboxRect(option).contains(mouseEvent->position().toPoint()))
                {
                    return false;
                }
                // 双击复选框只在松开时切换一次
                if (event->type() == QEvent::MouseButtonDblClick)
                {
                    return true;
                }
                break;
            }
        case QEvent::KeyPress:
            {
                const int key = static_cast<QKeyEvent*>(event)->key();
                if (key != Qt::Key_Space && key != Qt::Key_Select)
                {
                    return false;
                }
                break;
            }
        default:
            return false;
    }
    const Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    Qt::CheckState nextState = state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    // 用户三态：未选中、半选、选中循环
    if (index.flags().testFlag(Qt::ItemIsUserTristate))
    {
        nextState = static_cast<Qt::CheckState>((static_cast<int>(state) + 1) % 3);
    }
    return model->setData(index, static_cast<int>(nextState), Qt::CheckStateRole);
}

bool ShadListDelegate::hasCheckbox(const QModelIndex& index)
{
    return index.flags().testFlag(Qt::ItemIsUserCheckable) && index.data(Qt::CheckStateRole).isValid();
}

bool ShadListDelegate::showsDescription() const
{
    return _isDescriptionVisible && _itemSize != ShadItemType::ExtraSmall;
}

int ShadListDelegate::horizontalPadding() const
{
    switch (_itemSize)
    {
        case ShadItemType::Small:
            return 10;
        case ShadItemType::ExtraSmall:
            return 8;
        case ShadItemType::Medium:
            break;
    }
    return 12;
}

int ShadListDelegate::gap() const
{
    switch (_itemSize)
    {
        case ShadItemType::Small:
            return 10;
        case ShadItemType::ExtraSmall:
            return 8;
        case ShadItemType::Medium:
            break;
    }
    return 12;
}

int ShadListDelegate::imageSide() const
{
    switch (_itemSize)
    {
        case ShadItemType::Small:
            return showsDescription() ? 32 : 24;
        case ShadItemType::ExtraSmall:
            return 20;
        case ShadItemType::Medium:
            break;
    }
    return showsDescription() ? 40 : 32;
}

QRect ShadListDelegate::checkboxRect(const QStyleOptionViewItem& option) const
{
    const QRect rect = option.rect;
    const QRect logicalRect(rect.left() + horizontalPadding(), rect.top() + (rect.height() - kCheckboxSize) / 2, kCheckboxSize, kCheckboxSize);
    return ShadStyleHelper::visualRect(option.direction, rect, logicalRect);
}
