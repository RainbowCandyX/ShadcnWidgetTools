#include "ShadTreeSelect.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QIcon>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QRectF>
#include <QSizePolicy>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVariant>
#include <QtGlobal>

#include "ShadComboBoxPrivate.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadPopover.h"
#include "ShadSelectBadges.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"
#include "ShadTreeSelectPanel.h"
#include "ShadTreeSelectPrivate.h"
#include "ShadTreeView.h"

namespace
{
    // 触发框尺寸与 ShadComboBox 一致
    constexpr int kBorderWidth = 1;
    constexpr int kPaddingX = 12;
    constexpr int kContentGap = 8;
    constexpr int kIconSize = 16;
    constexpr qreal kChevronOpacity = 0.5;
    // 最小宽度下至少显示的文字宽度
    constexpr int kMinTextWidth = 32;

    // 应用默认文字，使用者改过的保持不变
    void applyDefaultTexts(ShadTreeSelect* treeSelect)
    {
        ShadStyleHelper::applyDefaultText(treeSelect, "placeholderText", treeSelect->getPlaceholderText(), ShadTreeSelect::tr("请选择"), [treeSelect](const QString& text) { treeSelect->setPlaceholderText(text); });
        ShadStyleHelper::applyDefaultText(treeSelect, "searchPlaceholderText", treeSelect->getSearchPlaceholderText(), ShadTreeSelect::tr("搜索..."), [treeSelect](const QString& text) { treeSelect->setSearchPlaceholderText(text); });
        ShadStyleHelper::applyDefaultText(treeSelect, "emptyText", treeSelect->getEmptyText(), ShadTreeSelect::tr("未找到结果"), [treeSelect](const QString& text) { treeSelect->setEmptyText(text); });
    }
} // namespace

ShadTreeSelect::ShadTreeSelect(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTreeSelectPrivate())
{
    Q_D(ShadTreeSelect);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_hoverTransition = new ShadTransition(this);
    d->_internalModel = new QStandardItemModel(this);

    // 面板自带内边距
    d->_popover = new ShadPopover(this);
    d->_popover->setContentPadding(0);
    d->_panel = new ShadTreeSelectPanel(d->_popover);
    d->_popover->contentLayout()->addWidget(d->_panel);
    ShadTreeView* treeView = d->_panel->treeView();
    treeView->setModel(d->_internalModel);
    // 选中行只表示键盘高亮项，多选结果保存在复选框中
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->installEventFilter(this);
    d->_panel->searchEdit()->installEventFilter(this);
    d->connectModel(d->_internalModel);
    applyDefaultTexts(this);

    connect(treeView, &ShadTreeView::pressed, this, [d](const QModelIndex& index) { d->onTreePressed(index); });
    connect(treeView, &ShadTreeView::clicked, this, [d](const QModelIndex& index) { d->onTreeClicked(index); });
    connect(d->_panel, &ShadTreeSelectPanel::filtered, this, [d](const QModelIndex& firstMatch) { d->onFiltered(firstMatch); });
    connect(d->_popover, &ShadPopover::aboutToHide, this, [this, d]()
    {
        // 关闭后恢复完整的树
        d->_panel->resetFilter();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTreeSelect::ShadTreeSelect(ShadComboBoxType::SelectionMode mode, QWidget* parent)
    : ShadTreeSelect(parent)
{
    setSelectionMode(mode);
}

ShadTreeSelect::~ShadTreeSelect()
{
    Q_D(ShadTreeSelect);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    d->disconnectModel();
    QObject::disconnect(d->_panel->treeView(), nullptr, this, nullptr);
    QObject::disconnect(d->_panel, nullptr, this, nullptr);
    QObject::disconnect(d->_popover, nullptr, this, nullptr);
}

void ShadTreeSelect::setModel(QAbstractItemModel* model)
{
    Q_D(ShadTreeSelect);
    QAbstractItemModel* targetModel = model != nullptr ? model : d->_internalModel;
    ShadTreeView* treeView = d->_panel->treeView();
    if (treeView->model() == targetModel)
    {
        return;
    }
    hidePopup();
    d->disconnectModel();
    const bool hadCurrentIndex = d->_hasCurrentIndex;
    d->_currentIndex = QPersistentModelIndex();
    d->_hasCurrentIndex = false;
    d->_pressedIndex = QPersistentModelIndex();
    // setModel 会为树创建新的选择模型，旧的不会被自动删除
    QItemSelectionModel* oldSelectionModel = treeView->selectionModel();
    treeView->setModel(targetModel);
    if (oldSelectionModel != nullptr)
    {
        oldSelectionModel->deleteLater();
    }
    d->connectModel(targetModel);
    d->invalidateTextWidth();
    updateGeometry();
    update();
    if (hadCurrentIndex)
    {
        Q_EMIT currentIndexChanged(QModelIndex());
    }
    if (d->isMultiSelection())
    {
        d->emitCheckedChanged();
    }
}

QAbstractItemModel* ShadTreeSelect::model() const
{
    Q_D(const ShadTreeSelect);
    return d->currentModel();
}

ShadTreeView* ShadTreeSelect::treeView() const
{
    Q_D(const ShadTreeSelect);
    return d->_panel->treeView();
}

QStandardItem* ShadTreeSelect::addItem(const QString& text, QStandardItem* parent)
{
    Q_D(ShadTreeSelect);
    if (d->currentModel() != d->_internalModel)
    {
        return nullptr;
    }
    QStandardItem* item = new QStandardItem(text);
    item->setEditable(false);
    d->configureInternalItem(item);
    if (parent != nullptr)
    {
        parent->appendRow(item);
    }
    else
    {
        d->_internalModel->appendRow(item);
    }
    return item;
}

void ShadTreeSelect::clearItems()
{
    Q_D(ShadTreeSelect);
    d->_internalModel->removeRows(0, d->_internalModel->rowCount());
}

void ShadTreeSelect::setInvalid(bool invalid)
{
    Q_D(ShadTreeSelect);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
}

bool ShadTreeSelect::isInvalid() const
{
    Q_D(const ShadTreeSelect);
    return d->_isInvalid;
}

void ShadTreeSelect::setTreeSelectSize(ShadComboBoxType::Size size)
{
    Q_D(ShadTreeSelect);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    updateGeometry();
    update();
}

ShadComboBoxType::Size ShadTreeSelect::getTreeSelectSize() const
{
    Q_D(const ShadTreeSelect);
    return d->_size;
}

void ShadTreeSelect::setSelectionMode(ShadComboBoxType::SelectionMode mode)
{
    Q_D(ShadTreeSelect);
    if (d->_selectionMode == mode)
    {
        return;
    }
    hidePopup();
    d->_selectionMode = mode;
    if (d->currentModel() == d->_internalModel)
    {
        // 合并切换复选框产生的勾选变化
        ++d->_checkBatchDepth;
        d->configureInternalItems(d->_internalModel->invisibleRootItem());
        --d->_checkBatchDepth;
        d->_isCheckedDirty = false;
    }
    d->emitCheckedChanged();
    update();
}

ShadComboBoxType::SelectionMode ShadTreeSelect::getSelectionMode() const
{
    Q_D(const ShadTreeSelect);
    return d->_selectionMode;
}

void ShadTreeSelect::setCheckedStrategy(ShadTreeSelectType::CheckedStrategy strategy)
{
    Q_D(ShadTreeSelect);
    if (d->_checkedStrategy == strategy)
    {
        return;
    }
    d->_checkedStrategy = strategy;
    d->emitCheckedChanged();
    update();
}

ShadTreeSelectType::CheckedStrategy ShadTreeSelect::getCheckedStrategy() const
{
    Q_D(const ShadTreeSelect);
    return d->_checkedStrategy;
}

void ShadTreeSelect::setLeafOnly(bool leafOnly)
{
    Q_D(ShadTreeSelect);
    d->_isLeafOnly = leafOnly;
}

bool ShadTreeSelect::isLeafOnly() const
{
    Q_D(const ShadTreeSelect);
    return d->_isLeafOnly;
}

void ShadTreeSelect::setFullPathVisible(bool visible)
{
    Q_D(ShadTreeSelect);
    if (d->_isFullPathVisible == visible)
    {
        return;
    }
    d->_isFullPathVisible = visible;
    d->invalidateTextWidth();
    updateGeometry();
    update();
}

bool ShadTreeSelect::isFullPathVisible() const
{
    Q_D(const ShadTreeSelect);
    return d->_isFullPathVisible;
}

void ShadTreeSelect::setSearchEnabled(bool enabled)
{
    Q_D(ShadTreeSelect);
    if (d->_isSearchEnabled == enabled)
    {
        return;
    }
    d->_isSearchEnabled = enabled;
    if (isPopupVisible())
    {
        hidePopup();
        showPopup();
    }
}

bool ShadTreeSelect::isSearchEnabled() const
{
    Q_D(const ShadTreeSelect);
    return d->_isSearchEnabled;
}

void ShadTreeSelect::setPlaceholderText(const QString& text)
{
    Q_D(ShadTreeSelect);
    d->_placeholderText = text;
    d->invalidateTextWidth();
    updateGeometry();
    update();
}

QString ShadTreeSelect::getPlaceholderText() const
{
    Q_D(const ShadTreeSelect);
    return d->_placeholderText;
}

void ShadTreeSelect::setSearchPlaceholderText(const QString& text)
{
    Q_D(ShadTreeSelect);
    d->_searchPlaceholderText = text;
}

QString ShadTreeSelect::getSearchPlaceholderText() const
{
    Q_D(const ShadTreeSelect);
    return d->_searchPlaceholderText;
}

void ShadTreeSelect::setEmptyText(const QString& text)
{
    Q_D(ShadTreeSelect);
    d->_emptyText = text;
}

QString ShadTreeSelect::getEmptyText() const
{
    Q_D(const ShadTreeSelect);
    return d->_emptyText;
}

void ShadTreeSelect::setCurrentIndex(const QModelIndex& index)
{
    Q_D(ShadTreeSelect);
    const QModelIndex node = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
    if (node.isValid())
    {
        if (node.model() != d->currentModel() || !node.flags().testFlag(Qt::ItemIsEnabled))
        {
            return;
        }
        if (d->_isLeafOnly && d->currentModel()->hasChildren(node))
        {
            return;
        }
    }
    if (d->_currentIndex == node)
    {
        return;
    }
    d->_currentIndex = QPersistentModelIndex(node);
    d->_hasCurrentIndex = node.isValid();
    update();
    Q_EMIT currentIndexChanged(node);
}

QModelIndex ShadTreeSelect::getCurrentIndex() const
{
    Q_D(const ShadTreeSelect);
    return d->_currentIndex;
}

QString ShadTreeSelect::getCurrentText() const
{
    Q_D(const ShadTreeSelect);
    if (d->isMultiSelection())
    {
        return QString();
    }
    return d->displayText(d->_currentIndex);
}

void ShadTreeSelect::setIndexChecked(const QModelIndex& index, bool checked)
{
    Q_D(ShadTreeSelect);
    const QModelIndex node = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
    if (!node.isValid() || node.model() != d->currentModel() || !ShadTreeSelectPrivate::hasCheckbox(node))
    {
        return;
    }
    const Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;
    if (ShadTreeSelectPrivate::checkState(node) == state)
    {
        return;
    }
    // 联动产生的多次数据变化合并为一次 checkedIndexesChanged
    ++d->_checkBatchDepth;
    d->applyCheckState(node, state);
    --d->_checkBatchDepth;
    if (d->_isCheckedDirty)
    {
        d->_isCheckedDirty = false;
        d->emitCheckedChanged();
    }
}

bool ShadTreeSelect::isIndexChecked(const QModelIndex& index) const
{
    const QModelIndex node = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
    return ShadTreeSelectPrivate::hasCheckbox(node) && ShadTreeSelectPrivate::checkState(node) == Qt::Checked;
}

QList<QModelIndex> ShadTreeSelect::getCheckedIndexes() const
{
    Q_D(const ShadTreeSelect);
    QList<QModelIndex> indexes;
    if (d->isMultiSelection())
    {
        d->collectCheckedIndexes(QModelIndex(), &indexes);
    }
    return indexes;
}

QStringList ShadTreeSelect::getCheckedTexts() const
{
    QStringList texts;
    const QList<QModelIndex> indexes = getCheckedIndexes();
    texts.reserve(indexes.size());
    for (const QModelIndex& index : indexes)
    {
        texts.append(index.data(Qt::DisplayRole).toString());
    }
    return texts;
}

void ShadTreeSelect::clearChecked()
{
    Q_D(ShadTreeSelect);
    ++d->_checkBatchDepth;
    d->uncheckRows(QModelIndex());
    --d->_checkBatchDepth;
    if (d->_isCheckedDirty)
    {
        d->_isCheckedDirty = false;
        d->emitCheckedChanged();
    }
}

void ShadTreeSelect::showPopup()
{
    Q_D(ShadTreeSelect);
    const QAbstractItemModel* currentModel = d->currentModel();
    if (!isEnabled() || isPopupVisible() || currentModel == nullptr || currentModel->rowCount() == 0)
    {
        return;
    }
    d->_panel->setSearchVisible(d->_isSearchEnabled);
    d->_panel->setSearchPlaceholderText(d->_searchPlaceholderText);
    d->_panel->setEmptyText(d->_emptyText);
    d->_panel->resetFilter();
    ShadTreeView* treeView = d->_panel->treeView();
    // 展开当前项的祖先，打开后可直接看到当前项
    if (!d->isMultiSelection() && d->_currentIndex.isValid())
    {
        for (QModelIndex parent = d->_currentIndex.parent(); parent.isValid(); parent = parent.parent())
        {
            treeView->expand(parent);
        }
    }
    // 浮层宽度与触发框一致
    const int bodyWidth = width() - 2 * ShadStyleHelper::kFocusRingWidth;
    d->_popover->setContentWidth(qMax(bodyWidth, ShadTreeSelectPanel::kMinWidth));
    d->_popover->popup(this, ShadPlacementType::Bottom, ShadPlacementType::Start);
    if (d->_isSearchEnabled)
    {
        d->_panel->searchEdit()->setFocus(Qt::PopupFocusReason);
    }
    else
    {
        treeView->setFocus(Qt::PopupFocusReason);
    }
    d->highlightIndex(d->initialHighlightIndex());
    update();
}

void ShadTreeSelect::hidePopup()
{
    Q_D(ShadTreeSelect);
    if (d->_popover->isVisible())
    {
        d->_popover->hide();
    }
}

bool ShadTreeSelect::isPopupVisible() const
{
    Q_D(const ShadTreeSelect);
    return d->_popover->isVisible();
}

QSize ShadTreeSelect::sizeHint() const
{
    Q_D(const ShadTreeSelect);
    // 宽度按最长的节点文字计算，切换选择时触发框宽度不变
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int chromeWidth = 2 * (kBorderWidth + kPaddingX) + kContentGap + kIconSize;
    return QSize(d->contentWidth() + chromeWidth + margin, ShadComboBoxPrivate::bodyHeight(d->_size) + margin);
}

QSize ShadTreeSelect::minimumSizeHint() const
{
    Q_D(const ShadTreeSelect);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int chromeWidth = 2 * (kBorderWidth + kPaddingX) + kContentGap + kIconSize;
    return QSize(kMinTextWidth + chromeWidth + margin, ShadComboBoxPrivate::bodyHeight(d->_size) + margin);
}

bool ShadTreeSelect::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadTreeSelect);
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (watched == d->_panel->treeView() && d->handleTreeKey(keyEvent))
        {
            return true;
        }
        if (watched == d->_panel->searchEdit() && d->handleSearchKey(keyEvent))
        {
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadTreeSelect::changeEvent(QEvent* event)
{
    Q_D(ShadTreeSelect);
    switch (event->type())
    {
        case QEvent::EnabledChange:
            {
                d->_hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
                if (!isEnabled())
                {
                    hidePopup();
                }
                update();
                break;
            }
        case QEvent::FontChange:
            {
                d->invalidateTextWidth();
                updateGeometry();
                break;
            }
        case QEvent::LayoutDirectionChange:
            {
                update();
                break;
            }
        case QEvent::LanguageChange:
            {
                applyDefaultTexts(this);
                break;
            }
        default:
            {
                break;
            }
    }
    QWidget::changeEvent(event);
}

void ShadTreeSelect::enterEvent(QEnterEvent* event)
{
    Q_D(ShadTreeSelect);
    if (isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadTreeSelect::leaveEvent(QEvent* event)
{
    Q_D(ShadTreeSelect);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadTreeSelect::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadTreeSelect);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadTreeSelect::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadTreeSelect);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadTreeSelect::keyPressEvent(QKeyEvent* event)
{
    // 打开弹出层的按键与 ShadComboBox 一致
    switch (event->key())
    {
        case Qt::Key_Space:
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
                QWidget::keyPressEvent(event);
                return;
            }
    }
}

void ShadTreeSelect::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadTreeSelect);
    if (event->button() != Qt::LeftButton || !isEnabled())
    {
        QWidget::mousePressEvent(event);
        return;
    }
    // 弹出层打开时点击触发框由弹出层自身关闭
    d->_isFocusVisible = false;
    setFocus(Qt::MouseFocusReason);
    showPopup();
    event->accept();
}

void ShadTreeSelect::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadTreeSelect);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;
    // 放入 ShadButtonGroup 时与相邻组件拼接
    const GroupEdges edges = groupEdges(this);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius, edges);
    // 只有暗色主题填充背景
    if (shadTheme->isDark())
    {
        const QColor input = shadTheme->getColor(ShadThemeType::Input);
        fillRoundedRect(&painter, bodyRect, radius, mix(withAlpha(input, 0.3), withAlpha(input, 0.5), d->_hoverTransition->value()), edges);
    }
    // 焦点环只在键盘聚焦时显示
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

    // 按 LTR 布局计算，RTL 时镜像
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF contentRect = bodyRect.adjusted(kBorderWidth + kPaddingX, 0, -(kBorderWidth + kPaddingX), 0);
    const QRectF chevronRect = visualRect(direction, bodyRect, QRectF(contentRect.right() - kIconSize, contentRect.center().y() - kIconSize / 2.0, kIconSize, kIconSize));
    ShadIcon::paint(&painter, chevronRect, ShadIconType::ChevronDown, withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), kChevronOpacity), kIconSize);

    const QRectF textRect = visualRect(direction, bodyRect, contentRect.adjusted(0, 0, -(kIconSize + kContentGap), 0));
    const QColor placeholderColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    if (d->isMultiSelection())
    {
        const QStringList texts = getCheckedTexts();
        if (texts.isEmpty())
        {
            drawIconText(&painter, textRect, ShadIconType::None, QIcon(), d->_placeholderText, font(), placeholderColor, kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
        }
        else
        {
            ShadSelectBadges::paint(&painter, textRect, texts, font(), direction);
        }
        return;
    }
    const QModelIndex currentIndex = d->_currentIndex;
    if (!currentIndex.isValid())
    {
        drawIconText(&painter, textRect, ShadIconType::None, QIcon(), d->_placeholderText, font(), placeholderColor, kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
        return;
    }
    // 图标与树中节点一致
    const QVariant iconData = currentIndex.data(ShadTreeView::IconRole);
    const ShadIconType::IconName shadIcon = iconData.isValid() ? static_cast<ShadIconType::IconName>(iconData.toInt()) : ShadIconType::None;
    const QIcon fallbackIcon = shadIcon == ShadIconType::None ? ShadTreeSelectPrivate::decorationIcon(currentIndex) : QIcon();
    drawIconText(&painter, textRect, shadIcon, fallbackIcon, d->displayText(currentIndex), font(), shadTheme->getColor(ShadThemeType::Foreground), kContentGap, Qt::AlignLeft, kIconSize, 0.0, direction);
}
