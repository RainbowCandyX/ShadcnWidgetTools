#include "ShadAttachmentPrivate.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFontMetricsF>
#include <QImage>
#include <QImageReader>
#include <QLatin1String>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QSize>
#include <QSizePolicy>
#include <QVariantAnimation>
#include <QtGlobal>
#include <QtMath>

#include <initializer_list>

#include "ShadAttachment.h"
#include "ShadButton.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 纵向宽度 w-24，有文字时 w-30
    constexpr int kVerticalWidth = 96;
    constexpr int kVerticalContentWidth = 120;
    // 纵向时文字区域 px-1，操作按钮位于 top-3 right-3，按钮间距 gap-1
    constexpr int kVerticalContentInset = 4;
    constexpr int kVerticalActionsInset = 12;
    constexpr int kVerticalActionsGap = 4;
    // 纵向时图标 size-6，xs 尺寸保持 size-3.5
    constexpr int kVerticalIconSize = 24;
    // 操作按钮 icon-xs 的可视边长
    constexpr int kActionSize = 24;
    // 描述 text-xs mt-0.5，行高 leading-tight（1.25）
    constexpr int kDescriptionPixelSize = 12;
    constexpr int kDescriptionTop = 2;
    constexpr qreal kLineHeightRatio = 1.25;

    int lineHeight(int pixelSize)
    {
        return qCeil(pixelSize * kLineHeightRatio);
    }
} // namespace

ShadAttachmentPrivate::ShadAttachmentPrivate()
{
}

ShadAttachmentPrivate::~ShadAttachmentPrivate()
{
}

ShadAttachmentPrivate::SizeMetrics ShadAttachmentPrivate::sizeMetrics(ShadAttachmentType::Size size)
{
    // default: gap-2 text-sm px-2.5 py-2（只有图标区时 p-2），图标区 w-10 rounded-lg，卡片 rounded-xl
    // sm: gap-2.5 text-xs px-2 py-1.5（p-1.5），图标区 w-8
    // xs: gap-1.5 text-xs px-1.5 py-1（p-1），图标区 w-7 rounded-md，图标 size-3.5，卡片 rounded-lg
    switch (size)
    {
        case ShadAttachmentType::Small:
            return {8, 6, 6, 10, 32, 16, 12, ShadThemeType::ExtraLarge, ShadThemeType::Large};
        case ShadAttachmentType::ExtraSmall:
            return {6, 4, 4, 6, 28, 14, 12, ShadThemeType::Large, ShadThemeType::Medium};
        case ShadAttachmentType::Medium:
            break;
    }
    return {10, 8, 8, 8, 40, 16, 14, ShadThemeType::ExtraLarge, ShadThemeType::Large};
}

bool ShadAttachmentPrivate::isInteractive() const
{
    return _isClickable || !_url.isEmpty();
}

void ShadAttachmentPrivate::activate()
{
    Q_Q(ShadAttachment);
    // 信号处理中可能删除附件，之后的步骤先确认对象仍然存在
    const QPointer<ShadAttachment> guard(q);
    const QUrl url = _url;
    Q_EMIT q->clicked();
    if (guard.isNull() || url.isEmpty())
    {
        return;
    }
    Q_EMIT q->linkActivated(url);
    if (!guard.isNull() && _isOpenExternalLinks)
    {
        openUrl();
    }
}

void ShadAttachmentPrivate::openUrl() const
{
    if (_url.isEmpty())
    {
        return;
    }
    if (!QDesktopServices::openUrl(_url))
    {
        qWarning("无法打开链接：%s", qUtf8Printable(urlDisplayText()));
    }
}

QString ShadAttachmentPrivate::urlDisplayText() const
{
    if (_url.isLocalFile())
    {
        return QDir::toNativeSeparators(_url.toLocalFile());
    }
    return _url.toString();
}

ShadIconType::IconName ShadAttachmentPrivate::iconForSuffix(const QString& suffix)
{
    const QString lower = suffix.toLower();
    const auto isOneOf = [&lower](std::initializer_list<const char*> suffixes)
    {
        for (const char* item : suffixes)
        {
            if (lower == QLatin1String(item))
            {
                return true;
            }
        }
        return false;
    };
    if (isOneOf({"pdf", "doc", "docx", "txt", "md", "rtf", "odt", "pages"}))
    {
        return ShadIconType::FileText;
    }
    if (isOneOf({"png", "jpg", "jpeg", "gif", "webp", "bmp", "svg", "ico", "tif", "tiff", "heic"}))
    {
        return ShadIconType::FileImage;
    }
    if (isOneOf({"xls", "xlsx", "csv", "tsv", "ods", "numbers"}))
    {
        return ShadIconType::FileSpreadsheet;
    }
    if (isOneOf({"zip", "rar", "7z", "tar", "gz", "bz2", "xz", "tgz"}))
    {
        return ShadIconType::FileArchive;
    }
    if (isOneOf({"mp3", "wav", "flac", "aac", "ogg", "m4a"}))
    {
        return ShadIconType::FileAudio;
    }
    if (isOneOf({"mp4", "mov", "avi", "mkv", "webm", "wmv"}))
    {
        return ShadIconType::FileVideoCamera;
    }
    if (isOneOf({"json"}))
    {
        return ShadIconType::FileJson;
    }
    if (isOneOf({"c", "cc", "cpp", "cxx", "h", "hpp", "rs", "py", "js", "jsx", "ts", "tsx", "java", "kt", "go", "swift", "cs", "html", "css", "xml", "yaml", "yml", "toml", "sh"}))
    {
        return ShadIconType::FileCode;
    }
    return ShadIconType::File;
}

QString ShadAttachmentPrivate::formatFileSize(qint64 bytes)
{
    // 1024 进制；KB 取整，MB 及以上保留一位小数，例如 "820 KB"、"1.8 MB"
    constexpr qreal kUnit = 1024.0;
    if (bytes < 1024)
    {
        return QStringLiteral("%1 B").arg(bytes);
    }
    const qreal kilobytes = bytes / kUnit;
    if (kilobytes < kUnit)
    {
        return QStringLiteral("%1 KB").arg(qRound(kilobytes));
    }
    const qreal megabytes = kilobytes / kUnit;
    if (megabytes < kUnit)
    {
        return QStringLiteral("%1 MB").arg(megabytes, 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(megabytes / kUnit, 0, 'f', 1);
}

QImage ShadAttachmentPrivate::loadThumbnail(const QString& path)
{
    // 纵向图标区最大约 108px，按 2 倍像素比取 256px，只按该尺寸解码，大图不会整张读入内存
    constexpr int kThumbnailSize = 256;
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize originalSize = reader.size();
    if (originalSize.isValid() && (originalSize.width() > kThumbnailSize || originalSize.height() > kThumbnailSize))
    {
        reader.setScaledSize(originalSize.scaled(kThumbnailSize, kThumbnailSize, Qt::KeepAspectRatioByExpanding));
    }
    const QImage image = reader.read();
    if (image.isNull())
    {
        qWarning("读取图片缩略图失败：%s，%s", qUtf8Printable(QDir::toNativeSeparators(path)), qUtf8Printable(reader.errorString()));
    }
    return image;
}

bool ShadAttachmentPrivate::hasMedia() const
{
    return !_image.isNull() || _icon != ShadIconType::None;
}

bool ShadAttachmentPrivate::hasContent() const
{
    return !_title.isEmpty() || !_description.isEmpty();
}

bool ShadAttachmentPrivate::isVertical() const
{
    return _orientation == Qt::Vertical;
}

int ShadAttachmentPrivate::visibleActionCount() const
{
    int count = 0;
    for (const QPointer<ShadButton>& action : _actions)
    {
        if (!action.isNull() && !action->isHidden())
        {
            ++count;
        }
    }
    return count;
}

QRect ShadAttachmentPrivate::cardRect() const
{
    Q_Q(const ShadAttachment);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    return q->rect().adjusted(margin, margin, -margin, -margin);
}

int ShadAttachmentPrivate::textBlockHeight() const
{
    const SizeMetrics metrics = sizeMetrics(_size);
    int height = 0;
    if (!_title.isEmpty())
    {
        height += lineHeight(metrics.titlePixelSize);
    }
    if (!_description.isEmpty())
    {
        height += (_title.isEmpty() ? 0 : kDescriptionTop) + lineHeight(kDescriptionPixelSize);
    }
    return height;
}

int ShadAttachmentPrivate::contentTextWidth() const
{
    int width = 0;
    if (!_title.isEmpty())
    {
        width = qMax(width, qCeil(QFontMetricsF(titleFont()).horizontalAdvance(_title)));
    }
    if (!_description.isEmpty())
    {
        width = qMax(width, qCeil(QFontMetricsF(descriptionFont()).horizontalAdvance(_description)));
    }
    return width;
}

int ShadAttachmentPrivate::iconSize() const
{
    const SizeMetrics metrics = sizeMetrics(_size);
    if (isVertical() && _size != ShadAttachmentType::ExtraSmall)
    {
        return kVerticalIconSize;
    }
    return metrics.iconSize;
}

QSize ShadAttachmentPrivate::cardSize() const
{
    const SizeMetrics metrics = sizeMetrics(_size);
    const bool isMediaShown = hasMedia();
    const bool isContentShown = hasContent();
    const int paddingX = isContentShown ? metrics.paddingX : metrics.mediaPadding;
    const int paddingY = isContentShown ? metrics.paddingY : metrics.mediaPadding;
    if (isVertical())
    {
        // 图标区铺满宽度并保持正方形，文字在下方
        const int width = isContentShown ? kVerticalContentWidth : kVerticalWidth;
        const int innerWidth = width - 2 * paddingX;
        int height = 2 * paddingY;
        if (isMediaShown)
        {
            height += innerWidth;
        }
        if (isMediaShown && isContentShown)
        {
            height += metrics.gap;
        }
        if (isContentShown)
        {
            height += textBlockHeight();
        }
        return QSize(width, height);
    }
    const int actionCount = visibleActionCount();
    int width = 2 * paddingX;
    int height = 0;
    int partCount = 0;
    if (isMediaShown)
    {
        width += metrics.mediaSize;
        height = qMax(height, metrics.mediaSize);
        ++partCount;
    }
    if (isContentShown)
    {
        width += contentTextWidth();
        height = qMax(height, textBlockHeight());
        ++partCount;
    }
    if (actionCount > 0)
    {
        width += actionCount * kActionSize;
        height = qMax(height, kActionSize);
        ++partCount;
    }
    width += qMax(0, partCount - 1) * metrics.gap;
    return QSize(qMax(kHorizontalMinWidth, width), height + 2 * paddingY);
}

ShadAttachmentPrivate::ContentLayout ShadAttachmentPrivate::computeLayout() const
{
    const SizeMetrics metrics = sizeMetrics(_size);
    const QRectF card(cardRect());
    const bool isMediaShown = hasMedia();
    const bool isContentShown = hasContent();
    const int paddingX = isContentShown ? metrics.paddingX : metrics.mediaPadding;
    const int paddingY = isContentShown ? metrics.paddingY : metrics.mediaPadding;
    const QRectF inner = card.adjusted(paddingX, paddingY, -paddingX, -paddingY);
    const int titleLine = lineHeight(metrics.titlePixelSize);
    const int descriptionLine = lineHeight(kDescriptionPixelSize);
    const int actionCount = visibleActionCount();
    ContentLayout layout;

    // 标题与描述自 top 起依次排列
    const auto placeText = [&](qreal left, qreal top, qreal width)
    {
        if (!_title.isEmpty())
        {
            layout.titleRect = QRectF(left, top, width, titleLine);
            top += titleLine + kDescriptionTop;
        }
        if (!_description.isEmpty())
        {
            layout.descriptionRect = QRectF(left, top, width, descriptionLine);
        }
    };

    if (isVertical())
    {
        qreal top = inner.top();
        if (isMediaShown)
        {
            layout.mediaRect = QRectF(inner.left(), top, inner.width(), inner.width());
            top += inner.width() + metrics.gap;
        }
        if (isContentShown)
        {
            placeText(inner.left() + kVerticalContentInset, top, qMax<qreal>(0.0, inner.width() - 2 * kVerticalContentInset));
        }
        if (actionCount > 0)
        {
            // absolute top-3 right-3，相对卡片边框外缘定位
            const qreal width = actionCount * kActionSize + (actionCount - 1) * kVerticalActionsGap;
            layout.actionsRect = QRectF(card.right() - kVerticalActionsInset - width, card.top() + kVerticalActionsInset, width, kActionSize);
        }
        return layout;
    }

    // 横向：各部分垂直居中（items-center），文字区域占据剩余宽度（flex-1 min-w-0）
    const qreal centerY = inner.center().y();
    qreal left = inner.left();
    qreal right = inner.right();
    if (isMediaShown)
    {
        layout.mediaRect = QRectF(left, centerY - metrics.mediaSize / 2.0, metrics.mediaSize, metrics.mediaSize);
        left += metrics.mediaSize + metrics.gap;
    }
    if (actionCount > 0)
    {
        const qreal width = actionCount * kActionSize;
        layout.actionsRect = QRectF(right - width, centerY - kActionSize / 2.0, width, kActionSize);
        right -= width + metrics.gap;
    }
    if (isContentShown)
    {
        placeText(left, centerY - textBlockHeight() / 2.0, qMax<qreal>(0.0, right - left));
    }
    return layout;
}

QFont ShadAttachmentPrivate::titleFont() const
{
    Q_Q(const ShadAttachment);
    // font-medium，default 为 text-sm，sm / xs 为 text-xs
    QFont font = q->font();
    font.setPixelSize(sizeMetrics(_size).titlePixelSize);
    font.setWeight(QFont::Medium);
    return font;
}

QFont ShadAttachmentPrivate::descriptionFont() const
{
    Q_Q(const ShadAttachment);
    QFont font = q->font();
    font.setPixelSize(kDescriptionPixelSize);
    font.setWeight(QFont::Normal);
    return font;
}

void ShadAttachmentPrivate::layoutActions()
{
    Q_Q(ShadAttachment);
    if (_actions.isEmpty())
    {
        return;
    }
    const ContentLayout layout = computeLayout();
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRectF card(cardRect());
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const qreal step = kActionSize + (isVertical() ? kVerticalActionsGap : 0);
    int index = 0;
    for (const QPointer<ShadButton>& action : _actions)
    {
        if (action.isNull() || action->isHidden())
        {
            continue;
        }
        // 逻辑上按添加顺序从起始侧排列，RTL 时整体镜像；按钮组件四周带焦点环边距
        const QRectF bodyRect(layout.actionsRect.left() + index * step, layout.actionsRect.top(), kActionSize, kActionSize);
        const QRectF visualBody = ShadStyleHelper::visualRect(direction, card, bodyRect);
        action->setGeometry(visualBody.adjusted(-margin, -margin, margin, margin).toRect());
        action->raise();
        ++index;
    }
}

void ShadAttachmentPrivate::updateContent()
{
    Q_Q(ShadAttachment);
    const bool isClickable = isInteractive();
    q->setFocusPolicy(isClickable ? Qt::StrongFocus : Qt::NoFocus);
    if (isClickable)
    {
        q->setCursor(Qt::PointingHandCursor);
    }
    else
    {
        q->unsetCursor();
    }
    // 纵向宽度固定；横向可以被布局拉伸（对应示例中的 w-full），宽度不足时文字省略
    q->setSizePolicy(isVertical() ? QSizePolicy::Fixed : QSizePolicy::Preferred, QSizePolicy::Fixed);
    q->updateGeometry();
    layoutActions();
    updateAnimations(q->isVisible());
    q->update();
}

void ShadAttachmentPrivate::updateAnimations(bool isShown)
{
    const bool isSpinning = isShown && _image.isNull() && _icon == ShadIconType::LoaderCircle;
    if (isSpinning && _spinAnimation->state() != QAbstractAnimation::Running)
    {
        _spinAnimation->start();
    }
    else if (!isSpinning && _spinAnimation->state() == QAbstractAnimation::Running)
    {
        _spinAnimation->stop();
        _spinAngle = 0.0;
    }
    const bool isShimmering = isShown && !_title.isEmpty() && (_state == ShadAttachmentType::Uploading || _state == ShadAttachmentType::Processing);
    if (isShimmering && _shimmerAnimation->state() != QAbstractAnimation::Running)
    {
        _shimmerAnimation->start();
    }
    else if (!isShimmering && _shimmerAnimation->state() == QAbstractAnimation::Running)
    {
        _shimmerAnimation->stop();
        _shimmerProgress = 0.0;
    }
}

void ShadAttachmentPrivate::paintMedia(QPainter* painter, const QRectF& rect) const
{
    using namespace ShadStyleHelper;
    const qreal radius = shadTheme->getRadius(sizeMetrics(_size).mediaRadius);
    const bool isError = _state == ShadAttachmentType::Error;
    // bg-muted text-foreground；错误态 bg-destructive/10 text-destructive
    const QColor destructive = shadTheme->getColor(ShadThemeType::Destructive);
    fillRoundedRect(painter, rect, radius, isError ? withAlpha(destructive, 0.1) : shadTheme->getColor(ShadThemeType::Muted));
    if (!_image.isNull())
    {
        // object-cover：按短边裁剪居中的正方形；上传中等状态 opacity-60
        const bool isOpaque = _state == ShadAttachmentType::Done || _state == ShadAttachmentType::Idle;
        const qreal side = qMin(_image.width(), _image.height());
        const QRectF source((_image.width() - side) / 2.0, (_image.height() - side) / 2.0, side, side);
        painter->save();
        painter->setClipPath(roundedRectPath(rect, radius));
        painter->setOpacity(painter->opacity() * (isOpaque ? 1.0 : 0.6));
        painter->drawPixmap(rect, _image, source);
        painter->restore();
        return;
    }
    const int size = iconSize();
    const QRectF iconRect(rect.center().x() - size / 2.0, rect.center().y() - size / 2.0, size, size);
    const QColor iconColor = isError ? destructive : shadTheme->getColor(ShadThemeType::Foreground);
    painter->save();
    if (_spinAngle != 0.0)
    {
        const QPointF center = iconRect.center();
        painter->translate(center);
        painter->rotate(_spinAngle);
        painter->translate(-center);
    }
    ShadIcon::paint(painter, iconRect, _icon, iconColor, size);
    painter->restore();
}

void ShadAttachmentPrivate::paintTitle(QPainter* painter, const QRectF& rect) const
{
    Q_Q(const ShadAttachment);
    const QFont font = titleFont();
    const QFontMetricsF metrics(font);
    const QColor base = shadTheme->getColor(ShadThemeType::CardForeground);
    const QString text = metrics.elidedText(_title, Qt::ElideRight, rect.width());
    painter->setFont(font);
    const bool isShimmering = _state == ShadAttachmentType::Uploading || _state == ShadAttachmentType::Processing;
    if (!isShimmering)
    {
        painter->setPen(base);
        painter->drawText(rect, ShadStyleHelper::visualAlignment(q->layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), text);
        return;
    }
    // 高亮带从起始侧扫到结束侧（RTL 反向）
    ShadStyleHelper::drawShimmerText(painter, rect, text, font, base, _shimmerProgress, Qt::AlignLeft | Qt::AlignVCenter, q->layoutDirection());
}
