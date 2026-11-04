#include "ShadComboBoxPopup.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QScreen>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStringList>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include <algorithm>
#include <utility>

#include "ShadComboBox.h"
#include "ShadComboBoxSearch.h"
#include "ShadIcon.h"
#include "ShadPopupEffect.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 列表项：pl-2 pr-8，对勾位于 right-2 的 size-3.5 区域内，图标 size-4 与文字间距 gap-2
    constexpr int kItemPaddingLeft = 8;
    constexpr int kItemPaddingRight = 32;
    constexpr int kIndicatorRight = 8;
    constexpr int kIndicatorSize = 14;
    constexpr int kCheckIconSize = 16;
    constexpr int kItemIconSize = 16;
    constexpr int kItemIconGap = 8;
    // 多选复选框 size-4，与文字间距 gap-2；多选时右侧不显示对勾，右内边距与左侧相同
    constexpr int kCheckboxSize = 16;
    constexpr int kCheckboxGap = 8;
    // 分隔线 h-px + my-1
    constexpr int kSeparatorHeight = 9;
    // 空状态 py-6 + text-sm 行高
    constexpr int kEmptyHeight = 68;

    // 弹出层四周留给 shadow-md 与打开动画位移的透明边距
    constexpr int kShadowMargin = 12;
    // 与触发框的间距，对应 sideOffset=4
    constexpr int kSideOffset = 4;
    // 最小宽度，对应 min-w-[8rem]
    constexpr int kMinWidth = 128;
    // 与屏幕边缘保持的最小距离
    constexpr int kScreenMargin = 8;
    constexpr int kBorderWidth = 1;
    // 搜索栏 h-9 px-3，搜索图标 size-4 与输入框间距 gap-2，下方 1px 分隔线
    constexpr int kSearchBarHeight = 36;
    constexpr int kSearchPaddingX = 12;
    constexpr int kSearchIconSize = 16;
    constexpr int kSearchIconGap = 8;
    // 打开动画：150ms 淡入，并从触发框方向滑入 8px（slide-in-from-top-2）
    constexpr int kOpenDurationMs = 150;
    constexpr int kSlideDistance = 8;
} // namespace

ShadComboBoxList::ShadComboBoxList(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    // 固定为 sizeHint 的高度，由外层 ShadScrollArea 负责滚动
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadComboBoxList::~ShadComboBoxList()
{
}

void ShadComboBoxList::setItems(const QList<ShadComboBoxPopupItem>& items)
{
    _items = items;
    _highlightIndex = -1;
    updateItemTops();
    updateGeometry();
    update();
}

const QList<ShadComboBoxPopupItem>& ShadComboBoxList::getItems() const
{
    return _items;
}

void ShadComboBoxList::setCurrentRow(int row)
{
    _currentRow = row;
    update();
}

void ShadComboBoxList::setMultiSelection(bool isMultiSelection)
{
    _isMultiSelection = isMultiSelection;
    updateGeometry();
    update();
}

void ShadComboBoxList::setEmptyText(const QString& text)
{
    _emptyText = text;
    update();
}

void ShadComboBoxList::setHighlightIndex(int index)
{
    const int validIndex = isSelectable(index) ? index : -1;
    if (_highlightIndex == validIndex)
    {
        return;
    }
    _highlightIndex = validIndex;
    update();
    Q_EMIT highlightChanged(validIndex);
}

int ShadComboBoxList::getHighlightIndex() const
{
    return _highlightIndex;
}

void ShadComboBoxList::moveHighlight(int step)
{
    const int count = int(_items.size());
    const int direction = step > 0 ? 1 : -1;
    int index = _highlightIndex;
    if (index < 0)
    {
        index = direction > 0 ? -1 : count;
    }
    for (index += direction; index >= 0 && index < count; index += direction)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
}

void ShadComboBoxList::highlightFirst()
{
    for (int index = 0; index < int(_items.size()); ++index)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
    setHighlightIndex(-1);
}

void ShadComboBoxList::highlightLast()
{
    for (int index = int(_items.size()) - 1; index >= 0; --index)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
    setHighlightIndex(-1);
}

void ShadComboBoxList::highlightRow(int row)
{
    for (int index = 0; index < int(_items.size()); ++index)
    {
        if (_items.at(index).row == row && isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
    highlightFirst();
}

bool ShadComboBoxList::isSelectable(int index) const
{
    if (index < 0 || index >= int(_items.size()))
    {
        return false;
    }
    const ShadComboBoxPopupItem& item = _items.at(index);
    return !item.isSeparator && item.isEnabled;
}

QRect ShadComboBoxList::itemRect(int index) const
{
    if (index < 0 || index >= int(_items.size()))
    {
        return QRect();
    }
    return QRect(kPadding, _itemTops.at(index), width() - 2 * kPadding, itemHeight(index));
}

QSize ShadComboBoxList::sizeHint() const
{
    using ShadStyleHelper::textWidth;
    if (_items.isEmpty())
    {
        return QSize(textWidth(font(), _emptyText) + 2 * kItemPaddingLeft, kEmptyHeight);
    }
    const int checkboxWidth = _isMultiSelection ? kCheckboxSize + kCheckboxGap : 0;
    const int paddingRight = _isMultiSelection ? kItemPaddingLeft : kItemPaddingRight;
    int contentWidth = 0;
    for (const ShadComboBoxPopupItem& item : _items)
    {
        if (item.isSeparator)
        {
            continue;
        }
        const int iconWidth = item.icon.isNull() ? 0 : kItemIconSize + kItemIconGap;
        contentWidth = qMax(contentWidth, kItemPaddingLeft + checkboxWidth + iconWidth + textWidth(font(), item.text) + paddingRight);
    }
    return QSize(contentWidth + 2 * kPadding, _itemTops.last() + kPadding);
}

void ShadComboBoxList::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (isSelectable(index))
    {
        setHighlightIndex(index);
    }
    QWidget::mouseMoveEvent(event);
}

void ShadComboBoxList::mousePressEvent(QMouseEvent* event)
{
    // 列表内的按下不交给弹出层，避免被当作点击外部
    event->accept();
}

void ShadComboBoxList::mouseReleaseEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (event->button() == Qt::LeftButton && isSelectable(index))
    {
        Q_EMIT itemClicked(index);
    }
    event->accept();
}

void ShadComboBoxList::paintEvent(QPaintEvent* event)
{
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setFont(font());
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);
    if (_items.isEmpty())
    {
        painter.setPen(foreground);
        painter.drawText(rect(), Qt::AlignCenter, _emptyText);
        return;
    }

    const qreal radius = shadTheme->getRadius(ShadThemeType::Small);
    const Qt::LayoutDirection direction = layoutDirection();
    for (int index = 0; index < int(_items.size()); ++index)
    {
        const ShadComboBoxPopupItem& item = _items.at(index);
        const QRect rowRect = itemRect(index);
        if (!rowRect.intersects(event->rect()))
        {
            continue;
        }
        if (item.isSeparator)
        {
            // -mx-1 抵消列表内边距，分隔线横跨整个列表
            painter.fillRect(QRect(0, rowRect.top() + (kSeparatorHeight - 1) / 2, width(), 1), shadTheme->getColor(ShadThemeType::Border));
            continue;
        }
        const bool isHighlighted = index == _highlightIndex;
        painter.setOpacity(item.isEnabled ? 1.0 : kDisabledOpacity);
        if (isHighlighted)
        {
            fillRoundedRect(&painter, QRectF(rowRect), radius, shadTheme->getColor(ShadThemeType::Accent));
        }
        const QColor textColor = isHighlighted ? shadTheme->getColor(ShadThemeType::AccentForeground) : foreground;
        // 以下按 LTR 计算逻辑位置，绘制时在行内镜像：RTL 时复选框与图标在右侧，对勾在左侧
        int textLeft = rowRect.left() + kItemPaddingLeft;
        if (_isMultiSelection)
        {
            const QRectF boxRect(textLeft, rowRect.top() + (rowRect.height() - kCheckboxSize) / 2, kCheckboxSize, kCheckboxSize);
            drawCheckbox(&painter, visualRect(direction, QRectF(rowRect), boxRect), item.isChecked ? Qt::Checked : Qt::Unchecked);
            textLeft += kCheckboxSize + kCheckboxGap;
        }
        if (!item.icon.isNull())
        {
            item.icon.paint(&painter, visualRect(direction, rowRect, QRect(textLeft, rowRect.top() + (rowRect.height() - kItemIconSize) / 2, kItemIconSize, kItemIconSize)));
            textLeft += kItemIconSize + kItemIconGap;
        }
        const int paddingRight = _isMultiSelection ? kItemPaddingLeft : kItemPaddingRight;
        const QRect textRect = visualRect(direction, rowRect, QRect(textLeft, rowRect.top(), rowRect.right() + 1 - paddingRight - textLeft, rowRect.height()));
        painter.setPen(textColor);
        painter.drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(font(), item.text, textRect.width()));
        if (!_isMultiSelection && item.row == _currentRow)
        {
            const QRectF indicatorRect(rowRect.right() + 1 - kIndicatorRight - kIndicatorSize, rowRect.top() + (rowRect.height() - kIndicatorSize) / 2.0, kIndicatorSize, kIndicatorSize);
            ShadIcon::paint(&painter, visualRect(direction, QRectF(rowRect), indicatorRect), ShadIconType::Check, textColor, kCheckIconSize);
        }
    }
    painter.setOpacity(1.0);
}

int ShadComboBoxList::itemHeight(int index) const
{
    return _items.at(index).isSeparator ? kSeparatorHeight : kItemHeight;
}

int ShadComboBoxList::indexAt(const QPoint& pos) const
{
    if (pos.x() < kPadding || pos.x() >= width() - kPadding)
    {
        return -1;
    }
    for (int index = 0; index < int(_items.size()); ++index)
    {
        if (pos.y() >= _itemTops.at(index) && pos.y() < _itemTops.at(index + 1))
        {
            return index;
        }
    }
    return -1;
}

void ShadComboBoxList::updateItemTops()
{
    _itemTops.clear();
    _itemTops.reserve(_items.size() + 1);
    int top = kPadding;
    for (int index = 0; index < int(_items.size()); ++index)
    {
        _itemTops.append(top);
        top += itemHeight(index);
    }
    _itemTops.append(top);
}

ShadComboBoxPopup::ShadComboBoxPopup(ShadComboBox* comboBox)
    : QWidget(comboBox, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), _comboBox(comboBox)
{
    // 圆角、边框与阴影自绘，窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]() { return cardRect(); });

    _content = new QWidget(this);
    _searchEdit = new QLineEdit(_content);
    _searchEdit->setFrame(false);
#ifdef Q_OS_MACOS
    // setFrame(false) 会开启该属性，macOS 样式会在获得焦点时额外绘制系统焦点框
    _searchEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    ShadStyleHelper::applyFont(_searchEdit, 14, QFont::Normal);
    _searchEdit->installEventFilter(this);

    _scrollArea = new ShadScrollArea(_content);
    // 滚动条两端避开卡片圆角
    _scrollArea->getOverlayScrollBar(Qt::Vertical)->setTrackInset(ShadComboBoxList::kPadding);
    _list = new ShadComboBoxList();
    _scrollArea->setWidget(_list);

    _openAnimation = new QVariantAnimation(this);
    _openAnimation->setDuration(kOpenDurationMs);
    _openAnimation->setStartValue(0.0);
    _openAnimation->setEndValue(1.0);
    // 对应 CSS 的 ease 缓动
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.25, 0.1), QPointF(0.25, 1.0), QPointF(1.0, 1.0));
    _openAnimation->setEasingCurve(easingCurve);
    connect(_openAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _openProgress = value.toReal();
        setWindowOpacity(_openProgress);
        layoutContent();
        update();
    });

    connect(_searchEdit, &QLineEdit::textChanged, this, [this]() { applyFilter(); });
    connect(_list, &ShadComboBoxList::itemClicked, this, [this](int index)
    {
        _list->setHighlightIndex(index);
        activateHighlighted();
    });
    connect(_list, &ShadComboBoxList::highlightChanged, this, [this](int index)
    {
        if (_list->isSelectable(index))
        {
            Q_EMIT rowHighlighted(_list->getItems().at(index).row);
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        applyThemeColors();
        update();
    });
    applyThemeColors();
}

ShadComboBoxPopup::~ShadComboBoxPopup()
{
}

void ShadComboBoxPopup::open()
{
    // 弹出层是独立窗口，不会继承下拉框所在区域的布局方向，打开时同步
    setLayoutDirection(_comboBox->layoutDirection());
    _isSearchEnabled = _comboBox->isSearchEnabled();
    _isMultiSelection = _comboBox->getSelectionMode() == ShadComboBoxType::MultiSelection;
    _list->setMultiSelection(_isMultiSelection);
    _searchEdit->setVisible(_isSearchEnabled);
    _searchEdit->setPlaceholderText(_comboBox->getSearchPlaceholderText());
    {
        // 清空上次的搜索文字时不触发过滤，下面统一加载
        const QSignalBlocker blocker(_searchEdit);
        _searchEdit->clear();
    }
    _typeaheadBuffer.clear();
    _list->setEmptyText(_comboBox->getEmptyText());
    _list->setCurrentRow(_isMultiSelection ? -1 : _comboBox->currentIndex());
    loadItems();
    _list->setItems(_allItems);
    _list->highlightRow(initialHighlightRow());

    // 打开期间行数或顺序变化时关闭弹出层，避免选中已失效的行；数据变化（包括多选切换勾选）时重新读取
    disconnectModel();
    if (QAbstractItemModel* model = _comboBox->model())
    {
        const auto closePopup = [this]() { hide(); };
        _modelConnections.append(connect(model, &QAbstractItemModel::rowsInserted, this, closePopup));
        _modelConnections.append(connect(model, &QAbstractItemModel::rowsRemoved, this, closePopup));
        _modelConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this, closePopup));
        _modelConnections.append(connect(model, &QAbstractItemModel::modelReset, this, closePopup));
        _modelConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this, closePopup));
        _modelConnections.append(connect(model, &QAbstractItemModel::dataChanged, this, [this]() { reloadItems(); }));
    }

    _openAnimation->stop();
    _openProgress = 0.0;
    updatePopupGeometry(false);
    setWindowOpacity(0.0);
    show();
    raise();
    if (_isSearchEnabled)
    {
        _searchEdit->setFocus(Qt::PopupFocusReason);
    }
    else
    {
        setFocus(Qt::PopupFocusReason);
    }
    // 显示后滚动区域才会按实际尺寸布局，此时再滚动到高亮项
    scrollToHighlight();
    _openAnimation->start();
}

bool ShadComboBoxPopup::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _searchEdit && event->type() == QEvent::KeyPress && handleNavigationKey(static_cast<QKeyEvent*>(event)))
    {
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShadComboBoxPopup::keyPressEvent(QKeyEvent* event)
{
    if (handleNavigationKey(event))
    {
        return;
    }
    const QString text = event->text();
    const bool hasCommandModifier = event->modifiers().testAnyFlags(Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier);
    if (!_isSearchEnabled && !hasCommandModifier && !text.isEmpty() && text.at(0).isPrint())
    {
        handleTypeahead(text);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadComboBoxPopup::mousePressEvent(QMouseEvent* event)
{
    if (!cardRect().contains(event->position().toPoint()))
    {
        // 点击触发框关闭弹出层时不再把这次按下转发给触发框，否则会立刻重新打开
        const QPoint comboPos = _comboBox->mapFromGlobal(event->globalPosition().toPoint());
        if (_comboBox->rect().contains(comboPos))
        {
            setAttribute(Qt::WA_NoMouseReplay);
        }
        hide();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadComboBoxPopup::hideEvent(QHideEvent* event)
{
    _openAnimation->stop();
    disconnectModel();
    QWidget::hideEvent(event);
    Q_EMIT popupHidden();
}

void ShadComboBoxPopup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF card = cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    drawShadowMd(&painter, card, radius);
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
    if (_isSearchEnabled)
    {
        // 搜索图标 opacity-50，位于起始侧（RTL 时在右侧）；搜索栏下方 border-b
        const QRectF iconRect(card.left() + kBorderWidth + kSearchPaddingX, card.top() + kBorderWidth + (kSearchBarHeight - kSearchIconSize) / 2.0, kSearchIconSize, kSearchIconSize);
        ShadIcon::paint(&painter, visualRect(layoutDirection(), card, iconRect), ShadIconType::Search, withAlpha(shadTheme->getColor(ShadThemeType::PopoverForeground), 0.5), kSearchIconSize);
        painter.fillRect(QRectF(card.left() + kBorderWidth, card.top() + kBorderWidth + kSearchBarHeight, card.width() - 2 * kBorderWidth, 1), shadTheme->getColor(ShadThemeType::Border));
    }
}

void ShadComboBoxPopup::applyThemeColors()
{
    using ShadStyleHelper::withAlpha;
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    // 背景与分隔线由弹出层自绘，样式表只清空原生外观并设置文字颜色
    const QString styleSheet = QStringLiteral("QLineEdit{background-color:transparent;border:none;padding:0px;color:%1;selection-background-color:%2;selection-color:%3;}").arg(foreground.name(QColor::HexArgb), selection.name(QColor::HexArgb), selectionText.name(QColor::HexArgb));
    _searchEdit->setStyleSheet(styleSheet);
    // 占位符颜色样式表无法设置，通过调色板设置
    QPalette palette = _searchEdit->palette();
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, withAlpha(placeholder, ShadStyleHelper::kDisabledOpacity));
    _searchEdit->setPalette(palette);
}

void ShadComboBoxPopup::loadItems()
{
    _allItems.clear();
    QAbstractItemModel* model = _comboBox->model();
    if (model == nullptr)
    {
        return;
    }
    const QModelIndex root = _comboBox->rootModelIndex();
    const int column = _comboBox->modelColumn();
    const int rowCount = model->rowCount(root);
    _allItems.reserve(rowCount);
    for (int row = 0; row < rowCount; ++row)
    {
        const QModelIndex index = model->index(row, column, root);
        ShadComboBoxPopupItem item;
        item.row = row;
        item.text = index.data(Qt::DisplayRole).toString();
        // 与 QComboBox::itemIcon 一致：DecorationRole 可能是 QPixmap 或 QIcon
        const QVariant decoration = index.data(Qt::DecorationRole);
        item.icon = decoration.userType() == QMetaType::QPixmap ? QIcon(decoration.value<QPixmap>()) : decoration.value<QIcon>();
        item.isEnabled = model->flags(index).testFlag(Qt::ItemIsEnabled);
        // QComboBox::insertSeparator 插入的分隔项
        item.isSeparator = index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
        item.isChecked = !item.isSeparator && index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        _allItems.append(item);
    }
}

void ShadComboBoxPopup::reloadItems()
{
    const int highlightIndex = _list->getHighlightIndex();
    const int highlightRow = _list->isSelectable(highlightIndex) ? _list->getItems().at(highlightIndex).row : -1;
    loadItems();
    applyFilter();
    if (highlightRow >= 0)
    {
        _list->highlightRow(highlightRow);
    }
}

int ShadComboBoxPopup::initialHighlightRow() const
{
    if (!_isMultiSelection)
    {
        return _comboBox->currentIndex();
    }
    for (const ShadComboBoxPopupItem& item : _allItems)
    {
        if (item.isChecked && item.isEnabled)
        {
            return item.row;
        }
    }
    return -1;
}

void ShadComboBoxPopup::applyFilter()
{
    const QString query = _searchEdit->text();
    if (query.trimmed().isEmpty())
    {
        _list->setItems(_allItems);
        _list->highlightRow(initialHighlightRow());
    }
    else
    {
        // 搜索时隐藏分隔线，按相关度从高到低排序，相关度相同保持原顺序
        QList<std::pair<int, ShadComboBoxPopupItem>> scoredItems;
        for (const ShadComboBoxPopupItem& item : std::as_const(_allItems))
        {
            if (item.isSeparator)
            {
                continue;
            }
            const int score = ShadComboBoxSearch::fuzzyScore(item.text, query);
            if (score > 0)
            {
                scoredItems.append(std::make_pair(score, item));
            }
        }
        std::stable_sort(scoredItems.begin(), scoredItems.end(), [](const std::pair<int, ShadComboBoxPopupItem>& left, const std::pair<int, ShadComboBoxPopupItem>& right) { return left.first > right.first; });
        QList<ShadComboBoxPopupItem> filteredItems;
        filteredItems.reserve(scoredItems.size());
        for (const std::pair<int, ShadComboBoxPopupItem>& scoredItem : std::as_const(scoredItems))
        {
            filteredItems.append(scoredItem.second);
        }
        _list->setItems(filteredItems);
        // 与 cmdk 一致，过滤后高亮第一个匹配项
        _list->highlightFirst();
    }
    updatePopupGeometry(true);
    scrollToHighlight();
}

void ShadComboBoxPopup::updatePopupGeometry(bool keepSide)
{
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    const QRect localBody = _comboBox->rect().adjusted(focusMargin, focusMargin, -focusMargin, -focusMargin);
    const QRect body(_comboBox->mapToGlobal(localBody.topLeft()), localBody.size());
    const QScreen* screen = _comboBox->screen();
    const QRect available = screen != nullptr ? screen->availableGeometry() : QRect();

    const int chromeHeight = 2 * kBorderWidth + searchBarHeight();
    int listHeight = qMin(_list->sizeHint().height(), maxListHeight());
    int cardWidth = qMax(qMax(body.width(), kMinWidth), _list->sizeHint().width() + 2 * kBorderWidth);
    if (available.isValid())
    {
        cardWidth = qMin(cardWidth, available.width() - 2 * kScreenMargin);
    }

    if (available.isValid())
    {
        const int spaceBelow = available.bottom() - kScreenMargin - body.bottom() - kSideOffset;
        const int spaceAbove = body.top() - kSideOffset - available.top() - kScreenMargin;
        if (!keepSide)
        {
            // 下方放不下且上方空间更大时显示在上方
            _isAbove = chromeHeight + listHeight > spaceBelow && spaceAbove > spaceBelow;
        }
        // 两侧都放不下时缩短列表，至少保留一项的高度
        const int space = _isAbove ? spaceAbove : spaceBelow;
        listHeight = qMax(qMin(listHeight, space - chromeHeight), ShadComboBoxList::kItemHeight);
    }
    const int cardHeight = chromeHeight + listHeight;

    // 与触发框起始侧对齐：LTR 左对齐，RTL 右对齐
    int cardX = layoutDirection() == Qt::RightToLeft ? body.right() + 1 - cardWidth : body.left();
    if (available.isValid())
    {
        cardX = qBound(available.left() + kScreenMargin, cardX, available.right() + 1 - kScreenMargin - cardWidth);
    }
    const int cardY = _isAbove ? body.top() - kSideOffset - cardHeight : body.bottom() + 1 + kSideOffset;
    setGeometry(cardX - kShadowMargin, cardY - kShadowMargin, cardWidth + 2 * kShadowMargin, cardHeight + 2 * kShadowMargin);
    layoutContent();
    update();
}

void ShadComboBoxPopup::layoutContent()
{
    const QRect inner = cardRect().adjusted(kBorderWidth, kBorderWidth, -kBorderWidth, -kBorderWidth);
    _content->setGeometry(inner);
    const int searchHeight = searchBarHeight();
    if (_isSearchEnabled)
    {
        const int editLeft = kSearchPaddingX + kSearchIconSize + kSearchIconGap;
        const QRect editRect(editLeft, 0, inner.width() - editLeft - kSearchPaddingX, kSearchBarHeight);
        _searchEdit->setGeometry(ShadStyleHelper::visualRect(layoutDirection(), QRect(0, 0, inner.width(), inner.height()), editRect));
    }
    _scrollArea->setGeometry(0, searchHeight, inner.width(), inner.height() - searchHeight);
}

bool ShadComboBoxPopup::handleNavigationKey(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Down:
        case Qt::Key_Up:
            {
                _list->moveHighlight(event->key() == Qt::Key_Down ? 1 : -1);
                scrollToHighlight();
                event->accept();
                return true;
            }
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            {
                const int pageItems = qMax(1, _scrollArea->viewport()->height() / ShadComboBoxList::kItemHeight - 1);
                for (int step = 0; step < pageItems; ++step)
                {
                    _list->moveHighlight(event->key() == Qt::Key_PageDown ? 1 : -1);
                }
                scrollToHighlight();
                event->accept();
                return true;
            }
        case Qt::Key_Home:
        case Qt::Key_End:
            {
                // 有搜索框时 Home / End 用于移动输入光标
                if (_isSearchEnabled)
                {
                    return false;
                }
                if (event->key() == Qt::Key_Home)
                {
                    _list->highlightFirst();
                }
                else
                {
                    _list->highlightLast();
                }
                scrollToHighlight();
                event->accept();
                return true;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
            {
                activateHighlighted();
                event->accept();
                return true;
            }
        case Qt::Key_Space:
            {
                // 没有搜索框且不在按键查找中时，空格选中高亮项（多选时切换勾选）；按键查找过程中空格作为查找文字的一部分
                const bool isTypeaheadActive = !_typeaheadBuffer.isEmpty() && _typeaheadTimer.isValid() && _typeaheadTimer.elapsed() <= ShadComboBoxSearch::kTypeaheadTimeoutMs;
                if (_isSearchEnabled || isTypeaheadActive)
                {
                    return false;
                }
                activateHighlighted();
                event->accept();
                return true;
            }
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            {
                hide();
                event->accept();
                return true;
            }
        default:
            {
                return false;
            }
    }
}

void ShadComboBoxPopup::handleTypeahead(const QString& text)
{
    if (!_typeaheadTimer.isValid() || _typeaheadTimer.elapsed() > ShadComboBoxSearch::kTypeaheadTimeoutMs)
    {
        _typeaheadBuffer.clear();
    }
    _typeaheadBuffer += text;
    _typeaheadTimer.restart();
    QStringList texts;
    const QList<ShadComboBoxPopupItem>& items = _list->getItems();
    texts.reserve(items.size());
    for (int index = 0; index < int(items.size()); ++index)
    {
        texts.append(_list->isSelectable(index) ? items.at(index).text : QString());
    }
    const int index = ShadComboBoxSearch::typeaheadIndex(texts, _typeaheadBuffer, _list->getHighlightIndex());
    if (index >= 0)
    {
        _list->setHighlightIndex(index);
        scrollToHighlight();
    }
}

void ShadComboBoxPopup::activateHighlighted()
{
    const int index = _list->getHighlightIndex();
    if (!_list->isSelectable(index))
    {
        return;
    }
    const int row = _list->getItems().at(index).row;
    // 多选模式只切换勾选状态并保持打开；勾选变化会通过模型的 dataChanged 回到 reloadItems 刷新列表
    if (_isMultiSelection)
    {
        Q_EMIT rowToggled(row);
        return;
    }
    hide();
    Q_EMIT rowActivated(row);
}

void ShadComboBoxPopup::scrollToHighlight()
{
    const int index = _list->getHighlightIndex();
    if (index < 0)
    {
        return;
    }
    const QRect rowRect = _list->itemRect(index);
    _scrollArea->ensureVisible(rowRect.center().x(), rowRect.center().y(), 0, rowRect.height() / 2 + ShadComboBoxList::kPadding);
}

QRect ShadComboBoxPopup::cardRect() const
{
    // 显示在下方时从上方滑入，显示在上方时从下方滑入
    const int slide = qRound((1.0 - _openProgress) * kSlideDistance);
    QRect card = rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    card.translate(0, _isAbove ? slide : -slide);
    return card;
}

int ShadComboBoxPopup::searchBarHeight() const
{
    return _isSearchEnabled ? kSearchBarHeight + 1 : 0;
}

int ShadComboBoxPopup::maxListHeight() const
{
    return qMax(1, _comboBox->maxVisibleItems()) * ShadComboBoxList::kItemHeight + 2 * ShadComboBoxList::kPadding;
}

void ShadComboBoxPopup::disconnectModel()
{
    for (const QMetaObject::Connection& connection : std::as_const(_modelConnections))
    {
        disconnect(connection);
    }
    _modelConnections.clear();
}
