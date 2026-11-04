#include "ShadEmptyPrivate.h"

#include <QColor>
#include <QLabel>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadEmpty.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kPadding = 24;
    constexpr int kBorderWidth = 1;
    constexpr int kSectionGap = 24;
    constexpr int kHeaderGap = 8;
    constexpr int kMediaBottomMargin = 8;
    constexpr int kIconMediaSize = 40;
    constexpr int kIconSize = 24;
} // namespace

ShadEmptyPrivate::ShadEmptyPrivate()
{
}

ShadEmptyPrivate::~ShadEmptyPrivate()
{
}

int ShadEmptyPrivate::padding() const
{
    return (_isBorderVisible ? kBorderWidth : 0) + kPadding;
}

QSize ShadEmptyPrivate::mediaSize() const
{
    if (!_mediaWidget.isNull())
    {
        // 媒体组件被使用者隐藏时不显示媒体区
        if (_mediaWidget->isHidden())
        {
            return QSize();
        }
        return _mediaWidget->sizeHint().expandedTo(_mediaWidget->minimumSize()).boundedTo(_mediaWidget->maximumSize());
    }
    if (_icon == ShadIconType::None)
    {
        return QSize();
    }
    const int size = _mediaVariant == ShadEmptyType::Icon ? kIconMediaSize : kIconSize;
    return QSize(size, size);
}

bool ShadEmptyPrivate::hasTitle() const
{
    return !_titleLabel->text().isEmpty();
}

bool ShadEmptyPrivate::hasDescription() const
{
    return !_descriptionLabel->text().isEmpty();
}

bool ShadEmptyPrivate::hasContent() const
{
    return !_contentLayout->isEmpty();
}

int ShadEmptyPrivate::contentHeightForWidth(int width) const
{
    const int height = _contentWidget->heightForWidth(width);
    if (height >= 0)
    {
        return height;
    }
    return _contentWidget->sizeHint().height();
}

ShadEmptyPrivate::LayoutResult ShadEmptyPrivate::computeLayout(int width) const
{
    LayoutResult result;
    const int pad = padding();
    // 标题、描述与操作区同宽，水平居中
    const int innerWidth = qMax(1, width - 2 * pad);
    const int columnWidth = qMin(innerWidth, kMaxContentWidth);
    const int columnLeft = (width - columnWidth) / 2;
    int top = pad;
    bool hasHeaderItem = false;
    const QSize media = mediaSize();
    if (!media.isEmpty())
    {
        result.mediaRect = QRect((width - media.width()) / 2, top, media.width(), media.height());
        top += media.height() + kMediaBottomMargin;
        hasHeaderItem = true;
    }
    if (hasTitle())
    {
        if (hasHeaderItem)
        {
            top += kHeaderGap;
        }
        const int titleHeight = _titleLabel->heightForWidth(columnWidth);
        result.titleRect = QRect(columnLeft, top, columnWidth, titleHeight);
        top += titleHeight;
        hasHeaderItem = true;
    }
    if (hasDescription())
    {
        if (hasHeaderItem)
        {
            top += kHeaderGap;
        }
        const int descriptionHeight = _descriptionLabel->heightForWidth(columnWidth);
        result.descriptionRect = QRect(columnLeft, top, columnWidth, descriptionHeight);
        top += descriptionHeight;
        hasHeaderItem = true;
    }
    if (hasContent())
    {
        if (hasHeaderItem)
        {
            top += kSectionGap;
        }
        const int contentHeight = contentHeightForWidth(columnWidth);
        result.contentRect = QRect(columnLeft, top, columnWidth, contentHeight);
        top += contentHeight;
    }
    result.totalHeight = top + pad;
    return result;
}

void ShadEmptyPrivate::updateColors()
{
    Q_Q(ShadEmpty);
    // 链接默认与描述同色，悬停时高亮
    QPalette titlePalette = _titleLabel->palette();
    titlePalette.setColor(QPalette::WindowText, shadTheme->getColor(ShadThemeType::Foreground));
    _titleLabel->setPalette(titlePalette);
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    QPalette descriptionPalette = _descriptionLabel->palette();
    descriptionPalette.setColor(QPalette::WindowText, mutedColor);
    descriptionPalette.setColor(QPalette::Link, _isLinkHovered ? shadTheme->getColor(ShadThemeType::Primary) : mutedColor);
    descriptionPalette.setColor(QPalette::LinkVisited, descriptionPalette.color(QPalette::Link));
    _descriptionLabel->setPalette(descriptionPalette);
    q->update();
}

void ShadEmptyPrivate::setLinkHovered(bool hovered)
{
    if (_isLinkHovered == hovered)
    {
        return;
    }
    _isLinkHovered = hovered;
    updateColors();
}

void ShadEmptyPrivate::updateContent()
{
    Q_Q(ShadEmpty);
    _titleLabel->setVisible(hasTitle());
    _descriptionLabel->setVisible(hasDescription());
    layoutChildren();
    q->updateGeometry();
    q->update();
}

void ShadEmptyPrivate::layoutChildren()
{
    Q_Q(ShadEmpty);
    const LayoutResult layout = computeLayout(q->width());
    // 高度多余时内容整体垂直居中
    const int offset = qMax(0, (q->height() - layout.totalHeight) / 2);
    _mediaRect = layout.mediaRect.translated(0, offset);
    if (!_mediaWidget.isNull())
    {
        _mediaWidget->setGeometry(_mediaRect);
    }
    _titleLabel->setGeometry(layout.titleRect.translated(0, offset));
    _descriptionLabel->setGeometry(layout.descriptionRect.translated(0, offset));
    _contentWidget->setGeometry(layout.contentRect.translated(0, offset));
    q->update();
}
