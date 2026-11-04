#include "ShadComboBox.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSet>
#include <QSizePolicy>
#include <QString>
#include <QVariant>
#include <QWheelEvent>
#include <QtMath>

#include "ShadComboBoxPopup.h"
#include "ShadComboBoxPrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 1px 边框 + px-3 内边距
    constexpr int kBorderWidth = 1;
    constexpr int kPaddingX = 12;
    // 内容与箭头间距 gap-2，箭头与图标 size-4
    constexpr int kContentGap = 8;
    constexpr int kIconSize = 16;
    // 箭头 opacity-50
    constexpr qreal kChevronOpacity = 0.5;
    // 最小宽度下至少显示的文字宽度
    constexpr int kMinTextWidth = 32;

    // 搜索框提示与无匹配文字使用默认文字，使用者改过的保持不变；弹出列表打开时读取，修改后下次打开生效
    void applyDefaultTexts(ShadComboBox* comboBox)
    {
        ShadStyleHelper::applyDefaultText(comboBox, "searchPlaceholderText", comboBox->getSearchPlaceholderText(), ShadComboBox::tr("搜索..."), [comboBox](const QString& text) { comboBox->setSearchPlaceholderText(text); });
        ShadStyleHelper::applyDefaultText(comboBox, "emptyText", comboBox->getEmptyText(), ShadComboBox::tr("未找到结果"), [comboBox](const QString& text) { comboBox->setEmptyText(text); });
    }
} // namespace

ShadComboBox::ShadComboBox(QWidget* parent)
    : QComboBox(parent), d_ptr(new ShadComboBoxPrivate())
{
    Q_D(ShadComboBox);
    d->q_ptr = this;
    applyDefaultTexts(this);
    d->_hoverTransition = new ShadTransition(this);
    setFocusPolicy(Qt::StrongFocus);
    // QComboBox 会从样式读取布局项边距（macOS 上不为 0），组件自绘且 sizeHint 已包含焦点环边距，直接使用控件矩形
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadComboBox::~ShadComboBox()
{
}

void ShadComboBox::setInvalid(bool invalid)
{
    Q_D(ShadComboBox);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadComboBox::isInvalid() const
{
    Q_D(const ShadComboBox);
    return d->_isInvalid;
}

void ShadComboBox::setComboBoxSize(ShadComboBoxType::Size size)
{
    Q_D(ShadComboBox);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    updateGeometry();
    update();
}

ShadComboBoxType::Size ShadComboBox::getComboBoxSize() const
{
    Q_D(const ShadComboBox);
    return d->_size;
}

void ShadComboBox::setSearchEnabled(bool enabled)
{
    Q_D(ShadComboBox);
    if (d->_isSearchEnabled == enabled)
    {
        return;
    }
    d->_isSearchEnabled = enabled;
    // 弹出层打开时按新设置重新打开
    if (isPopupVisible())
    {
        hidePopup();
        showPopup();
    }
}

bool ShadComboBox::isSearchEnabled() const
{
    Q_D(const ShadComboBox);
    return d->_isSearchEnabled;
}

void ShadComboBox::setSearchPlaceholderText(const QString& text)
{
    Q_D(ShadComboBox);
    d->_searchPlaceholderText = text;
}

QString ShadComboBox::getSearchPlaceholderText() const
{
    Q_D(const ShadComboBox);
    return d->_searchPlaceholderText;
}

void ShadComboBox::setEmptyText(const QString& text)
{
    Q_D(ShadComboBox);
    d->_emptyText = text;
}

QString ShadComboBox::getEmptyText() const
{
    Q_D(const ShadComboBox);
    return d->_emptyText;
}

void ShadComboBox::setSelectionMode(ShadComboBoxType::SelectionMode mode)
{
    Q_D(ShadComboBox);
    if (d->_selectionMode == mode)
    {
        return;
    }
    d->_selectionMode = mode;
    hidePopup();
    update();
}

ShadComboBoxType::SelectionMode ShadComboBox::getSelectionMode() const
{
    Q_D(const ShadComboBox);
    return d->_selectionMode;
}

void ShadComboBox::setCheckedRows(const QList<int>& rows)
{
    Q_D(ShadComboBox);
    const QSet<int> targetRows(rows.begin(), rows.end());
    bool isChanged = false;
    for (int row = 0; row < count(); ++row)
    {
        if (d->isSeparatorRow(row))
        {
            continue;
        }
        const bool isChecked = targetRows.contains(row);
        if (isRowChecked(row) != isChecked)
        {
            setItemData(row, static_cast<int>(isChecked ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
            isChanged = true;
        }
    }
    if (isChanged)
    {
        update();
        Q_EMIT checkedRowsChanged();
    }
}

QList<int> ShadComboBox::getCheckedRows() const
{
    QList<int> rows;
    for (int row = 0; row < count(); ++row)
    {
        if (isRowChecked(row))
        {
            rows.append(row);
        }
    }
    return rows;
}

QStringList ShadComboBox::getCheckedTexts() const
{
    QStringList texts;
    for (int row = 0; row < count(); ++row)
    {
        if (isRowChecked(row))
        {
            texts.append(itemText(row));
        }
    }
    return texts;
}

void ShadComboBox::setRowChecked(int row, bool checked)
{
    Q_D(ShadComboBox);
    if (row < 0 || row >= count() || d->isSeparatorRow(row) || isRowChecked(row) == checked)
    {
        return;
    }
    setItemData(row, static_cast<int>(checked ? Qt::Checked : Qt::Unchecked), Qt::CheckStateRole);
    update();
    Q_EMIT checkedRowsChanged();
}

bool ShadComboBox::isRowChecked(int row) const
{
    Q_D(const ShadComboBox);
    if (row < 0 || row >= count() || d->isSeparatorRow(row))
    {
        return false;
    }
    return itemData(row, Qt::CheckStateRole).toInt() == Qt::Checked;
}

void ShadComboBox::clearChecked()
{
    setCheckedRows(QList<int>());
}

bool ShadComboBox::isPopupVisible() const
{
    Q_D(const ShadComboBox);
    return d->_popup != nullptr && d->_popup->isVisible();
}

void ShadComboBox::showPopup()
{
    Q_D(ShadComboBox);
    if (!isEnabled() || count() == 0)
    {
        return;
    }
    // 不调用 QComboBox::showPopup，也不使用 QComboBox 内部的弹出容器
    if (d->_popup == nullptr)
    {
        d->_popup = new ShadComboBoxPopup(this);
        connect(d->_popup, &ShadComboBoxPopup::rowActivated, this, [d](int row) { d->activateRow(row); });
        connect(d->_popup, &ShadComboBoxPopup::rowToggled, this, [d](int row) { d->toggleRow(row); });
        connect(d->_popup, &ShadComboBoxPopup::rowHighlighted, this, [this](int row)
        {
            Q_EMIT highlighted(row);
            Q_EMIT textHighlighted(itemText(row));
        });
        connect(d->_popup, &ShadComboBoxPopup::popupHidden, this, [this]() { update(); });
    }
    d->_typeaheadBuffer.clear();
    d->_popup->open();
    update();
}

void ShadComboBox::hidePopup()
{
    Q_D(ShadComboBox);
    if (isPopupVisible())
    {
        d->_popup->hide();
    }
}

QSize ShadComboBox::sizeHint() const
{
    Q_D(const ShadComboBox);
    // 宽度按最长的选项计算，切换选项时触发框宽度不变
    const QFontMetricsF metrics(font());
    qreal textWidth = metrics.horizontalAdvance(placeholderText());
    bool hasIcon = false;
    for (int row = 0; row < count(); ++row)
    {
        textWidth = qMax(textWidth, metrics.horizontalAdvance(itemText(row)));
        hasIcon = hasIcon || !itemIcon(row).isNull();
    }
    const int contentWidth = qCeil(textWidth) + (hasIcon ? kIconSize + kContentGap : 0);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int chromeWidth = 2 * (kBorderWidth + kPaddingX) + kContentGap + kIconSize;
    return QSize(contentWidth + chromeWidth + margin, ShadComboBoxPrivate::bodyHeight(d->_size) + margin);
}

QSize ShadComboBox::minimumSizeHint() const
{
    Q_D(const ShadComboBox);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int chromeWidth = 2 * (kBorderWidth + kPaddingX) + kContentGap + kIconSize;
    return QSize(kMinTextWidth + chromeWidth + margin, ShadComboBoxPrivate::bodyHeight(d->_size) + margin);
}

void ShadComboBox::enterEvent(QEnterEvent* event)
{
    Q_D(ShadComboBox);
    if (isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QComboBox::enterEvent(event);
}

void ShadComboBox::leaveEvent(QEvent* event)
{
    Q_D(ShadComboBox);
    d->_hoverTransition->animateTo(0.0);
    QComboBox::leaveEvent(event);
}

void ShadComboBox::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadComboBox);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QComboBox::focusInEvent(event);
    update();
}

void ShadComboBox::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadComboBox);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QComboBox::focusOutEvent(event);
    update();
}

void ShadComboBox::changeEvent(QEvent* event)
{
    Q_D(ShadComboBox);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
        if (!isEnabled())
        {
            hidePopup();
        }
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        applyDefaultTexts(this);
    }
    QComboBox::changeEvent(event);
}

void ShadComboBox::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadComboBox);
    // 对应 Radix Select 的打开按键：空格、回车、上下方向键；另外保留 Qt 习惯的 F4
    switch (event->key())
    {
        case Qt::Key_Space:
            {
                // 单选模式按键查找过程中空格作为查找文字的一部分
                if (!d->isMultiSelection() && d->isTypeaheadActive())
                {
                    d->handleTypeahead(event->text());
                    event->accept();
                    return;
                }
                showPopup();
                event->accept();
                return;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_F4:
            {
                showPopup();
                event->accept();
                return;
            }
        default:
            {
                break;
            }
    }
    const QString text = event->text();
    const bool hasCommandModifier = event->modifiers().testAnyFlags(Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier);
    if (!hasCommandModifier && !text.isEmpty() && text.at(0).isPrint())
    {
        d->handleTypeahead(text);
        event->accept();
        return;
    }
    // 不交给 QComboBox：它会用 Home / PageUp 等按键切换选项，并创建内部的弹出容器
    QWidget::keyPressEvent(event);
}

void ShadComboBox::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isEnabled())
    {
        if (isPopupVisible())
        {
            hidePopup();
        }
        else
        {
            showPopup();
        }
        event->accept();
        return;
    }
    // 不交给 QComboBox：它会按原生样式做命中测试，并创建内部的弹出容器
    QWidget::mousePressEvent(event);
}

void ShadComboBox::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
}

void ShadComboBox::wheelEvent(QWheelEvent* event)
{
    // 与 Radix Select 一致，滚轮不切换选项，交给外层滚动
    event->ignore();
}

void ShadComboBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadComboBox);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;
    // 放入 ShadButtonGroup 时与相邻组件拼接的边为直角，起始侧不描边
    const GroupEdges edges = groupEdges(this);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius, edges);
    // 对应 dark:bg-input/30 dark:hover:bg-input/50，亮色背景透明
    if (shadTheme->isDark())
    {
        const QColor input = shadTheme->getColor(ShadThemeType::Input);
        fillRoundedRect(&painter, bodyRect, radius, mix(withAlpha(input, 0.3), withAlpha(input, 0.5), d->_hoverTransition->value()), edges);
    }
    // 对应 focus-visible:border-ring 与 aria-invalid:border-destructive；焦点环只在键盘聚焦时显示，错误态时为 destructive 色
    QColor borderColor = shadTheme->getColor(showFocusRing ? ShadThemeType::Ring : ShadThemeType::Input);
    if (d->_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor, edges);
    if (showFocusRing)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_isInvalid), edges);
    }

    // 按 LTR 计算：箭头在结束侧（右侧），文字在其余区域；RTL 时整体镜像
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF contentRect = bodyRect.adjusted(kBorderWidth + kPaddingX, 0, -(kBorderWidth + kPaddingX), 0);
    const QRectF chevronRect = visualRect(direction, bodyRect, QRectF(contentRect.right() - kIconSize, contentRect.center().y() - kIconSize / 2.0, kIconSize, kIconSize));
    ShadIcon::paint(&painter, chevronRect, ShadIconType::ChevronDown, withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), kChevronOpacity), kIconSize);

    // 未选中时显示 muted-foreground 色的占位文字（data-[placeholder]:text-muted-foreground）
    const QRectF textRect = visualRect(direction, bodyRect, contentRect.adjusted(0, 0, -(kIconSize + kContentGap), 0));
    if (d->isMultiSelection())
    {
        if (getCheckedRows().isEmpty())
        {
            drawIconText(&painter, textRect, ShadIconType::None, QIcon(), placeholderText(), font(), shadTheme->getColor(ShadThemeType::MutedForeground), kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
        }
        else
        {
            d->paintCheckedBadges(&painter, textRect);
        }
        return;
    }
    const int row = currentIndex();
    if (row >= 0)
    {
        drawIconText(&painter, textRect, ShadIconType::None, itemIcon(row), itemText(row), font(), shadTheme->getColor(ShadThemeType::Foreground), kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
    }
    else
    {
        drawIconText(&painter, textRect, ShadIconType::None, QIcon(), placeholderText(), font(), shadTheme->getColor(ShadThemeType::MutedForeground), kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
    }
}
