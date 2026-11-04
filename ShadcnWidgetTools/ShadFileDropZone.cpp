#include "ShadFileDropZone.h"

#include <QByteArray>
#include <QColor>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEnterEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFont>
#include <QIODevice>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QRectF>
#include <QSizePolicy>
#include <QString>

#include "ShadFileDropZonePrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // p-6；圆形图标区 size-10 rounded-full，图标 size-5
    constexpr int kPadding = 24;
    constexpr int kIconCircleSize = 40;
    constexpr int kIconSize = 20;
    // 图标与标题 gap-3，标题与描述 gap-1
    constexpr int kIconGap = 12;
    constexpr int kTextGap = 4;
    // 标题 text-sm font-medium（行高 20），描述 text-xs（行高 16）
    constexpr int kTitlePixelSize = 14;
    constexpr int kTitleLineHeight = 20;
    constexpr int kDescriptionPixelSize = 12;
    constexpr int kDescriptionLineHeight = 16;
    constexpr int kMinCardWidth = 240;

    QFont titleFont(const QFont& baseFont)
    {
        QFont font = baseFont;
        font.setPixelSize(kTitlePixelSize);
        font.setWeight(QFont::Medium);
        return font;
    }

    QFont descriptionFont(const QFont& baseFont)
    {
        QFont font = baseFont;
        font.setPixelSize(kDescriptionPixelSize);
        font.setWeight(QFont::Normal);
        return font;
    }

    // 标题与描述使用默认文字，使用者改过的保持不变
    void applyDefaultTexts(ShadFileDropZone* zone)
    {
        ShadStyleHelper::applyDefaultText(zone, "title", zone->getTitle(), ShadFileDropZone::tr("拖拽文件到这里"), [zone](const QString& text) { zone->setTitle(text); });
        ShadStyleHelper::applyDefaultText(zone, "description", zone->getDescription(), ShadFileDropZone::tr("或点击选择文件"), [zone](const QString& text) { zone->setDescription(text); });
    }
} // namespace

ShadFileDropZone::ShadFileDropZone(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadFileDropZonePrivate())
{
    Q_D(ShadFileDropZone);
    d->q_ptr = this;
    applyDefaultTexts(this);
    d->_hoverTransition = new ShadTransition(this);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadFileDropZone::~ShadFileDropZone()
{
}

void ShadFileDropZone::setTitle(const QString& title)
{
    Q_D(ShadFileDropZone);
    d->_title = title;
    updateGeometry();
    update();
}

QString ShadFileDropZone::getTitle() const
{
    Q_D(const ShadFileDropZone);
    return d->_title;
}

void ShadFileDropZone::setDescription(const QString& description)
{
    Q_D(ShadFileDropZone);
    d->_description = description;
    updateGeometry();
    update();
}

QString ShadFileDropZone::getDescription() const
{
    Q_D(const ShadFileDropZone);
    return d->_description;
}

void ShadFileDropZone::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadFileDropZone);
    d->_icon = icon;
    update();
}

ShadIconType::IconName ShadFileDropZone::getShadIcon() const
{
    Q_D(const ShadFileDropZone);
    return d->_icon;
}

void ShadFileDropZone::setNameFilters(const QStringList& filters)
{
    Q_D(ShadFileDropZone);
    d->_nameFilters = filters;
    d->rebuildPatterns();
}

QStringList ShadFileDropZone::getNameFilters() const
{
    Q_D(const ShadFileDropZone);
    return d->_nameFilters;
}

void ShadFileDropZone::setMultipleSelection(bool multiple)
{
    Q_D(ShadFileDropZone);
    d->_isMultipleSelection = multiple;
}

bool ShadFileDropZone::isMultipleSelection() const
{
    Q_D(const ShadFileDropZone);
    return d->_isMultipleSelection;
}

void ShadFileDropZone::setMaximumFileSize(qint64 bytes)
{
    Q_D(ShadFileDropZone);
    d->_maximumFileSize = qMax<qint64>(0, bytes);
}

qint64 ShadFileDropZone::getMaximumFileSize() const
{
    Q_D(const ShadFileDropZone);
    return d->_maximumFileSize;
}

QSize ShadFileDropZone::sizeHint() const
{
    Q_D(const ShadFileDropZone);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    int textWidth = 0;
    int height = 2 * kPadding + kIconCircleSize;
    if (!d->_title.isEmpty())
    {
        textWidth = qMax(textWidth, ShadStyleHelper::textWidth(titleFont(font()), d->_title));
        height += kIconGap + kTitleLineHeight;
    }
    if (!d->_description.isEmpty())
    {
        textWidth = qMax(textWidth, ShadStyleHelper::textWidth(descriptionFont(font()), d->_description));
        height += (d->_title.isEmpty() ? kIconGap : kTextGap) + kDescriptionLineHeight;
    }
    return QSize(qMax(kMinCardWidth, textWidth + 2 * kPadding) + margin, height + margin);
}

QSize ShadFileDropZone::minimumSizeHint() const
{
    // 文字放不下时省略，宽度只保留图标区与内边距
    const QSize hint = sizeHint();
    return QSize(kIconCircleSize + 2 * kPadding + 2 * ShadStyleHelper::kFocusRingWidth, hint.height());
}

void ShadFileDropZone::openFileDialog()
{
    Q_D(ShadFileDropZone);
    const QString caption = tr("选择文件");
    const QString filter = d->_nameFilters.join(QStringLiteral(";;"));
    const QPointer<ShadFileDropZone> guard(this);
#ifdef Q_OS_WASM
    Q_UNUSED(caption);
    QFileDialog::getOpenFileContent(filter, [guard](const QString& fileName, const QByteArray& content)
    {
        if (guard.isNull() || fileName.isEmpty())
        {
            return;
        }
        const QString path = QDir(QDir::tempPath()).filePath(QFileInfo(fileName).fileName());
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content) != content.size())
        {
            qWarning("写入临时文件失败: %s", qUtf8Printable(path));
            return;
        }
        file.close();
        guard->d_func()->processFiles(QStringList{path});
    });
    return;
#endif
    QStringList paths;
    if (d->_isMultipleSelection)
    {
        paths = QFileDialog::getOpenFileNames(this, caption, d->_lastDirectory, filter);
    }
    else
    {
        const QString path = QFileDialog::getOpenFileName(this, caption, d->_lastDirectory, filter);
        if (!path.isEmpty())
        {
            paths.append(path);
        }
    }
    // 模态对话框运行期间组件可能已被删除
    if (guard.isNull() || paths.isEmpty())
    {
        return;
    }
    d->processFiles(paths);
}

void ShadFileDropZone::changeEvent(QEvent* event)
{
    Q_D(ShadFileDropZone);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
        d->_isDragOver = false;
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        applyDefaultTexts(this);
    }
    QWidget::changeEvent(event);
}

void ShadFileDropZone::enterEvent(QEnterEvent* event)
{
    Q_D(ShadFileDropZone);
    if (isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadFileDropZone::leaveEvent(QEvent* event)
{
    Q_D(ShadFileDropZone);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadFileDropZone::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadFileDropZone);
    if (event->button() == Qt::LeftButton && d->cardRect().contains(event->position().toPoint()))
    {
        d->_isPressed = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadFileDropZone::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadFileDropZone);
    if (d->_isPressed && event->button() == Qt::LeftButton)
    {
        d->_isPressed = false;
        event->accept();
        if (d->cardRect().contains(event->position().toPoint()))
        {
            openFileDialog();
        }
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadFileDropZone::keyPressEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat())
    {
        // 与 button 一致：回车按下时触发，空格松开时触发
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            event->accept();
            openFileDialog();
            return;
        }
        if (event->key() == Qt::Key_Space)
        {
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void ShadFileDropZone::keyReleaseEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat() && event->key() == Qt::Key_Space)
    {
        event->accept();
        openFileDialog();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void ShadFileDropZone::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadFileDropZone);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadFileDropZone::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadFileDropZone);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadFileDropZone::dragEnterEvent(QDragEnterEvent* event)
{
    Q_D(ShadFileDropZone);
    // 只接受本地文件，网页链接等拖拽数据保持默认（不接受）
    if (ShadFileDropZonePrivate::localPaths(event->mimeData()).isEmpty())
    {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    d->_isDragOver = true;
    update();
}

void ShadFileDropZone::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void ShadFileDropZone::dragLeaveEvent(QDragLeaveEvent* event)
{
    Q_D(ShadFileDropZone);
    d->_isDragOver = false;
    update();
    QWidget::dragLeaveEvent(event);
}

void ShadFileDropZone::dropEvent(QDropEvent* event)
{
    Q_D(ShadFileDropZone);
    d->_isDragOver = false;
    update();
    const QStringList paths = ShadFileDropZonePrivate::localPaths(event->mimeData());
    if (paths.isEmpty())
    {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
    d->processFiles(paths);
}

void ShadFileDropZone::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadFileDropZone);
    using namespace ShadStyleHelper;
    const QRectF card(d->cardRect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge);
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;
    const Qt::LayoutDirection direction = layoutDirection();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 悬停与拖入时 bg-muted/50；拖入时边框变为 ring 色，键盘聚焦时为实线 ring 边框加焦点环
    const qreal highlight = d->_isDragOver ? 1.0 : d->_hoverTransition->value();
    fillRoundedRect(&painter, card, radius, withAlpha(shadTheme->getColor(ShadThemeType::Muted), 0.5 * highlight));
    if (showFocusRing)
    {
        strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, card, radius, focusRingColor(false));
    }
    else
    {
        strokeDashedRoundedRect(&painter, card, radius, shadTheme->getColor(d->_isDragOver ? ShadThemeType::Ring : ShadThemeType::Border));
    }

    // 内容整体垂直居中
    const int contentHeight = sizeHint().height() - 2 * ShadStyleHelper::kFocusRingWidth - 2 * kPadding;
    qreal top = card.center().y() - contentHeight / 2.0;
    const QRectF circleRect(card.center().x() - kIconCircleSize / 2.0, top, kIconCircleSize, kIconCircleSize);
    fillRoundedRect(&painter, circleRect, kIconCircleSize / 2.0, shadTheme->getColor(ShadThemeType::Muted));
    const QRectF iconRect(circleRect.center().x() - kIconSize / 2.0, circleRect.center().y() - kIconSize / 2.0, kIconSize, kIconSize);
    ShadIcon::paint(&painter, iconRect, d->_icon, shadTheme->getColor(d->_isDragOver ? ShadThemeType::Foreground : ShadThemeType::MutedForeground), kIconSize);
    top += kIconCircleSize;

    const QRectF textBounds = card.adjusted(kPadding, 0, -kPadding, 0);
    if (!d->_title.isEmpty())
    {
        top += kIconGap;
        const QFont font = titleFont(this->font());
        const QRectF titleRect(textBounds.left(), top, textBounds.width(), kTitleLineHeight);
        painter.setFont(font);
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(titleRect, visualAlignment(direction, Qt::AlignHCenter | Qt::AlignVCenter), elideText(font, d->_title, titleRect.width()));
        top += kTitleLineHeight;
    }
    if (!d->_description.isEmpty())
    {
        top += d->_title.isEmpty() ? kIconGap : kTextGap;
        const QFont font = descriptionFont(this->font());
        const QRectF descriptionRect(textBounds.left(), top, textBounds.width(), kDescriptionLineHeight);
        painter.setFont(font);
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(descriptionRect, visualAlignment(direction, Qt::AlignHCenter | Qt::AlignVCenter), elideText(font, d->_description, descriptionRect.width()));
    }
}
