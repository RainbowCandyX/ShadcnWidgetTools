#include "ShadTableDelegate.h"

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPaintDevice>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTable.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    // text-sm 行高；有说明时两行文字加上下 p-2
    constexpr int kTextLineHeight = 20;
    constexpr int kTwoLineRowHeight = 2 * ShadTableDelegate::kImagePaddingY + 2 * kTextLineHeight;
    // 占位块中 ImageOff 图标的尺寸范围
    constexpr int kPlaceholderMinIconSize = 12;
    constexpr int kPlaceholderMaxIconSize = 24;

    // 普通单元格的 16px 图标：支持 QPixmap、QImage 与 QIcon
    QIcon decorationIcon(const QModelIndex& index)
    {
        const QVariant decoration = index.data(Qt::DecorationRole);
        if (decoration.userType() == QMetaType::QPixmap)
        {
            return QIcon(decoration.value<QPixmap>());
        }
        if (decoration.userType() == QMetaType::QImage)
        {
            return QIcon(QPixmap::fromImage(decoration.value<QImage>()));
        }
        return decoration.value<QIcon>();
    }
} // namespace

ShadTableDelegate::ShadTableDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

ShadTableDelegate::~ShadTableDelegate()
{
}

void ShadTableDelegate::setSelectionColumn(int column)
{
    _selectionColumn = column;
}

void ShadTableDelegate::setActionColumn(int column)
{
    _actionColumn = column;
}

void ShadTableDelegate::setImageColumns(const QHash<int, ImageColumn>& columns)
{
    _imageColumns = columns;
}

void ShadTableDelegate::setHoveredRow(int row)
{
    _hoveredRow = row;
}

void ShadTableDelegate::setHoveredActionRow(int row)
{
    _hoveredActionRow = row;
}

int ShadTableDelegate::getHoveredRow() const
{
    return _hoveredRow;
}

int ShadTableDelegate::getHoveredActionRow() const
{
    return _hoveredActionRow;
}

QRect ShadTableDelegate::actionButtonRect(const QRect& cellRect, const QModelIndex& index, Qt::LayoutDirection direction) const
{
    // 按钮位置跟随该列的 TextAlignmentRole，默认靠起始侧；按 LTR 计算后镜像
    const Qt::Alignment alignment = ShadStyleHelper::logicalAlignment(direction, cellAlignment(index));
    int left = cellRect.left() + kCellPaddingX;
    if (alignment.testFlag(Qt::AlignRight))
    {
        left = cellRect.right() + 1 - kCellPaddingX - kActionButtonSize;
    }
    else if (alignment.testFlag(Qt::AlignHCenter))
    {
        left = cellRect.left() + (cellRect.width() - kActionButtonSize) / 2;
    }
    const QRect logicalRect(left, cellRect.top() + (cellRect.height() - kActionButtonSize) / 2, kActionButtonSize, kActionButtonSize);
    return ShadStyleHelper::visualRect(direction, cellRect, logicalRect);
}

QRect ShadTableDelegate::checkboxRect(const QRect& cellRect, Qt::LayoutDirection direction) const
{
    const QRect logicalRect(cellRect.left() + kCellPaddingX, cellRect.top() + (cellRect.height() - kCheckboxSize) / 2, kCheckboxSize, kCheckboxSize);
    return ShadStyleHelper::visualRect(direction, cellRect, logicalRect);
}

bool ShadTableDelegate::isRowChecked(const QModelIndex& index) const
{
    if (_selectionColumn < 0 || !index.isValid())
    {
        return false;
    }
    return index.siblingAtColumn(_selectionColumn).data(Qt::CheckStateRole).toInt() == Qt::Checked;
}

QFont ShadTableDelegate::cellFont(const QFont& baseFont, const QModelIndex& index)
{
    const QVariant fontData = index.data(Qt::FontRole);
    if (!fontData.canConvert<QFont>())
    {
        return baseFont;
    }
    // 只采用模型字体中显式设置的属性，其余（例如像素字号）沿用表格字体
    return fontData.value<QFont>().resolve(baseFont);
}

void ShadTableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    using namespace ShadStyleHelper;
    const QRect rect = option.rect;
    // option.rect 已是 RTL 下的实际位置，单元格内部元素再按 option.direction 镜像
    const Qt::LayoutDirection direction = option.direction;
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // hover:bg-muted/50 data-[state=selected]:bg-muted
    const QColor muted = shadTheme->getColor(ShadThemeType::Muted);
    if (isRowChecked(index))
    {
        painter->fillRect(rect, muted);
    }
    else if (index.row() == _hoveredRow)
    {
        painter->fillRect(rect, withAlpha(muted, 0.5));
    }
    // border-b，表体最后一行 border-0
    const QAbstractItemModel* model = index.model();
    if (model != nullptr && index.row() < model->rowCount(index.parent()) - 1)
    {
        painter->fillRect(QRect(rect.left(), rect.bottom(), rect.width(), 1), shadTheme->getColor(ShadThemeType::Border));
    }

    if (index.column() == _selectionColumn)
    {
        const Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
        drawCheckbox(painter, QRectF(checkboxRect(rect, direction)), state);
        painter->restore();
        return;
    }
    if (index.column() == _actionColumn)
    {
        // ghost 按钮：hover:bg-accent（暗色 hover:bg-accent/50）
        const QRect buttonRect = actionButtonRect(rect, index, direction);
        const bool isHovered = index.row() == _hoveredActionRow;
        if (isHovered)
        {
            const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
            fillRoundedRect(painter, QRectF(buttonRect), shadTheme->getRadius(ShadThemeType::Medium), shadTheme->isDark() ? withAlpha(accent, 0.5) : accent);
        }
        const QColor iconColor = shadTheme->getColor(isHovered ? ShadThemeType::AccentForeground : ShadThemeType::Foreground);
        ShadIcon::paint(painter, QRectF(buttonRect), ShadIconType::Ellipsis, iconColor, kIconSize);
        painter->restore();
        return;
    }

    // 图片或图标位于起始侧，contentRect 按 LTR 计算后镜像
    QRect contentRect = rect.adjusted(kCellPaddingX, 0, -kCellPaddingX, 0);
    if (_imageColumns.contains(index.column()))
    {
        const ImageColumn image = _imageColumns.value(index.column());
        const QRect logicalImageRect(contentRect.left(), rect.top() + (rect.height() - image.size.height()) / 2, image.size.width(), image.size.height());
        paintImage(painter, visualRect(direction, rect, logicalImageRect), index.data(Qt::DecorationRole), image);
        contentRect.setLeft(logicalImageRect.right() + 1 + kImageTextGap);
    }
    else
    {
        const QIcon icon = decorationIcon(index);
        if (!icon.isNull())
        {
            icon.paint(painter, visualRect(direction, rect, QRect(contentRect.left(), rect.top() + (rect.height() - kIconSize) / 2, kIconSize, kIconSize)));
            contentRect.setLeft(contentRect.left() + kIconSize + kIconGap);
        }
    }
    paintText(painter, option, index, visualRect(direction, rect, contentRect), direction);
    painter->restore();
}

void ShadTableDelegate::paintText(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& textRect, Qt::LayoutDirection direction) const
{
    using namespace ShadStyleHelper;
    const QFont font = cellFont(option.font, index);
    const QVariant foreground = index.data(Qt::ForegroundRole);
    const QColor textColor = foreground.canConvert<QBrush>() ? foreground.value<QBrush>().color() : shadTheme->getColor(ShadThemeType::Foreground);
    const QString text = index.data(Qt::DisplayRole).toString();
    const QString description = index.data(ShadTable::DescriptionRole).toString();
    const Qt::Alignment alignment = visualAlignment(direction, cellAlignment(index));
    painter->setFont(font);
    if (description.isEmpty())
    {
        painter->setPen(textColor);
        painter->drawText(textRect, alignment, elideText(font, text, textRect.width()));
        return;
    }
    // 两行：文字与 muted-foreground 色的说明，作为整体垂直居中
    const Qt::Alignment lineAlignment = (alignment & Qt::AlignHorizontal_Mask) | Qt::AlignVCenter;
    const int top = textRect.top() + (textRect.height() - 2 * kTextLineHeight) / 2;
    const QRect firstLine(textRect.left(), top, textRect.width(), kTextLineHeight);
    const QRect secondLine(textRect.left(), top + kTextLineHeight, textRect.width(), kTextLineHeight);
    painter->setPen(textColor);
    painter->drawText(firstLine, lineAlignment, elideText(font, text, textRect.width()));
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter->drawText(secondLine, lineAlignment, elideText(font, description, textRect.width()));
}

void ShadTableDelegate::paintImage(QPainter* painter, const QRect& imageRect, const QVariant& decoration, const ImageColumn& image) const
{
    using namespace ShadStyleHelper;
    const qreal devicePixelRatio = painter->device() != nullptr ? painter->device()->devicePixelRatioF() : 1.0;
    const QPixmap pixmap = renderedImage(decoration, image.size, image.shape, devicePixelRatio);
    if (!pixmap.isNull())
    {
        painter->drawPixmap(imageRect, pixmap);
        return;
    }
    // 没有图片：bg-muted 占位块，中间为 muted-foreground 色的 ImageOff 图标
    const QRectF placeholderRect(imageRect);
    const qreal radius = image.shape == ShadTableType::Circle ? qMin(placeholderRect.width(), placeholderRect.height()) / 2.0 : shadTheme->getRadius(ShadThemeType::Medium);
    fillRoundedRect(painter, placeholderRect, radius, shadTheme->getColor(ShadThemeType::Muted));
    const int iconSize = qBound(kPlaceholderMinIconSize, qMin(imageRect.width(), imageRect.height()) / 2, kPlaceholderMaxIconSize);
    ShadIcon::paint(painter, placeholderRect, ShadIconType::ImageOff, shadTheme->getColor(ShadThemeType::MutedForeground), iconSize);
}

QPixmap ShadTableDelegate::renderedImage(const QVariant& decoration, const QSize& size, ShadTableType::ImageShape shape, qreal devicePixelRatio)
{
    // 缓存键使用图片自身的 cacheKey，同一张图片只缩放一次
    qint64 sourceKey = 0;
    switch (decoration.userType())
    {
        case QMetaType::QPixmap:
            sourceKey = decoration.value<QPixmap>().cacheKey();
            break;
        case QMetaType::QImage:
            sourceKey = decoration.value<QImage>().cacheKey();
            break;
        case QMetaType::QIcon:
            sourceKey = decoration.value<QIcon>().cacheKey();
            break;
        default:
            return QPixmap();
    }
    const QSize physicalSize = (QSizeF(size) * devicePixelRatio).toSize();
    if (physicalSize.isEmpty())
    {
        return QPixmap();
    }
    const int radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QString key = QStringLiteral("ShadTableImage:%1:%2x%3:%4:%5").arg(sourceKey).arg(physicalSize.width()).arg(physicalSize.height()).arg(static_cast<int>(shape)).arg(radius);
    QPixmap result;
    if (QPixmapCache::find(key, &result))
    {
        return result;
    }
    QImage source;
    switch (decoration.userType())
    {
        case QMetaType::QPixmap:
            source = decoration.value<QPixmap>().toImage();
            break;
        case QMetaType::QImage:
            source = decoration.value<QImage>();
            break;
        default:
            source = decoration.value<QIcon>().pixmap(size, devicePixelRatio).toImage();
            break;
    }
    if (source.isNull())
    {
        return QPixmap();
    }
    // object-cover：按短边缩放填满，再从中间裁剪
    const QImage scaled = source.scaled(physicalSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QImage cropped = scaled.copy((scaled.width() - physicalSize.width()) / 2, (scaled.height() - physicalSize.height()) / 2, physicalSize.width(), physicalSize.height());
    result = QPixmap(physicalSize);
    result.fill(Qt::transparent);
    {
        QPainter imagePainter(&result);
        imagePainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        imagePainter.setPen(Qt::NoPen);
        imagePainter.setBrush(QBrush(cropped));
        const QRectF imageRect(0, 0, physicalSize.width(), physicalSize.height());
        if (shape == ShadTableType::Circle)
        {
            imagePainter.drawEllipse(imageRect);
        }
        else
        {
            imagePainter.drawRoundedRect(imageRect, radius * devicePixelRatio, radius * devicePixelRatio);
        }
    }
    result.setDevicePixelRatio(devicePixelRatio);
    QPixmapCache::insert(key, result);
    return result;
}

QSize ShadTableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() == _selectionColumn)
    {
        // 复选框单元格 pr-0
        return QSize(kCellPaddingX + kCheckboxSize, kDefaultRowHeight);
    }
    if (index.column() == _actionColumn)
    {
        return QSize(2 * kCellPaddingX + kActionButtonSize, kDefaultRowHeight);
    }
    const QFont font = cellFont(option.font, index);
    const QString text = index.data(Qt::DisplayRole).toString();
    const QString description = index.data(ShadTable::DescriptionRole).toString();
    const int contentWidth = qMax(ShadStyleHelper::textWidth(font, text), ShadStyleHelper::textWidth(font, description));
    int width = 2 * kCellPaddingX + contentWidth;
    int height = description.isEmpty() ? kDefaultRowHeight : kTwoLineRowHeight;
    if (_imageColumns.contains(index.column()))
    {
        // 图片列：图片 + 间距 + 文字，高度至少为图片高度加上下 p-2
        const QSize imageSize = _imageColumns.value(index.column()).size;
        width += imageSize.width() + (contentWidth > 0 ? kImageTextGap : 0);
        height = qMax(height, imageSize.height() + 2 * kImagePaddingY);
    }
    else if (!decorationIcon(index).isNull())
    {
        width += kIconSize + kIconGap;
    }
    return QSize(width, height);
}

Qt::Alignment ShadTableDelegate::cellAlignment(const QModelIndex& index)
{
    // 默认 text-left align-middle；模型可通过 TextAlignmentRole 指定（例如金额列 text-right）
    const QVariant value = index.data(Qt::TextAlignmentRole);
    Qt::Alignment alignment = value.isValid() ? Qt::Alignment(value.toInt()) : Qt::Alignment(Qt::AlignLeft);
    if ((alignment & Qt::AlignVertical_Mask) == 0)
    {
        alignment |= Qt::AlignVCenter;
    }
    return alignment;
}
