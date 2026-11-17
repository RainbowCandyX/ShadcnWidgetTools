#include "ShadTransferPanel.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadCheckbox.h"
#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadListView.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kKeyRole = Qt::UserRole + 1;
    constexpr int kBorderWidth = 1;
    constexpr int kHeaderHeight = 40;
    constexpr int kHeaderPaddingX = 12;
    constexpr int kHeaderGap = 8;
    constexpr int kSearchPadding = 8;
    constexpr int kListPadding = 4;
    constexpr int kFontPixelSize = 14;
    constexpr int kPreferredWidth = 240;
    constexpr int kPreferredHeight = 320;
    constexpr int kMinimumWidth = 160;
    constexpr int kMinimumHeight = 200;
} // namespace

ShadTransferPanel::ShadTransferPanel(QWidget* parent)
    : QWidget(parent)
{
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kBorderWidth, kBorderWidth, kBorderWidth, kBorderWidth);
    layout->setSpacing(0);

    _header = new QWidget(this);
    _header->setFixedHeight(kHeaderHeight);
    QHBoxLayout* headerLayout = new QHBoxLayout(_header);
    headerLayout->setContentsMargins(kHeaderPaddingX - focusMargin, 0, kHeaderPaddingX, 0);
    headerLayout->setSpacing(kHeaderGap - focusMargin);
    _checkAll = new ShadCheckbox(_header);
    _titleLabel = new ShadLabel(_header);
    _titleLabel->setTextFormat(Qt::PlainText);
    ShadStyleHelper::applyFont(_titleLabel, kFontPixelSize, QFont::Medium);
    _countLabel = new ShadLabel(_header);
    _countLabel->setTextFormat(Qt::PlainText);
    _countLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(_countLabel, kFontPixelSize, QFont::Normal);
    headerLayout->addWidget(_checkAll);
    headerLayout->addWidget(_titleLabel, 1);
    headerLayout->addWidget(_countLabel);
    layout->addWidget(_header);

    _searchContainer = new QWidget(this);
    QVBoxLayout* searchLayout = new QVBoxLayout(_searchContainer);
    const int searchPadding = kSearchPadding - focusMargin;
    searchLayout->setContentsMargins(searchPadding, searchPadding, searchPadding, 0);
    _searchInput = new ShadInput(_searchContainer);
    _searchInput->setLeadingIcon(ShadIconType::Search);
    _searchInput->setTrailingAction(ShadInputType::ClearAction);
    searchLayout->addWidget(_searchInput);
    _searchContainer->hide();
    layout->addWidget(_searchContainer);

    _model = new QStandardItemModel(this);
    _listView = new ShadListView(this);
    _listView->setItemSize(ShadItemType::Small);
    _listView->setSelectionMode(QAbstractItemView::NoSelection);
    _listView->setModel(_model);
    _listView->viewport()->installEventFilter(this);
    QWidget* listContainer = new QWidget(this);
    QVBoxLayout* listLayout = new QVBoxLayout(listContainer);
    listLayout->setContentsMargins(kListPadding, kListPadding, kListPadding, kListPadding);
    listLayout->addWidget(_listView);
    layout->addWidget(listContainer, 1);

    connect(_checkAll, &QAbstractButton::clicked, this, [this]() { toggleAllVisible(); });
    connect(_searchInput, &QLineEdit::textChanged, this, [this]() { rebuildModel(); });
    connect(_model, &QStandardItemModel::itemChanged, this, [this](QStandardItem* item) { handleItemChanged(item); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    updateHeader();
}

ShadTransferPanel::~ShadTransferPanel()
{
    _listView->viewport()->removeEventFilter(this);
}

void ShadTransferPanel::setTitle(const QString& title)
{
    _titleLabel->setText(title);
}

QString ShadTransferPanel::getTitle() const
{
    return _titleLabel->text();
}

void ShadTransferPanel::setItems(const QList<ShadTransferItem>& items)
{
    _items = items;
    QSet<QString> checkedKeys;
    for (const ShadTransferItem& item : _items)
    {
        if (!item.isDisabled && _checkedKeys.contains(item.key))
        {
            checkedKeys.insert(item.key);
        }
    }
    _checkedKeys = checkedKeys;
    rebuildModel();
}

QStringList ShadTransferPanel::checkedKeys() const
{
    QStringList keys;
    for (const ShadTransferItem& item : _items)
    {
        if (!item.isDisabled && _checkedKeys.contains(item.key))
        {
            keys.append(item.key);
        }
    }
    return keys;
}

void ShadTransferPanel::uncheckKeys(const QStringList& keys)
{
    for (const QString& key : keys)
    {
        _checkedKeys.remove(key);
    }
}

void ShadTransferPanel::setSearchEnabled(bool enabled)
{
    _searchContainer->setVisible(enabled);
    if (!enabled)
    {
        _searchInput->clear();
    }
}

void ShadTransferPanel::setSearchPlaceholderText(const QString& text)
{
    _searchInput->setPlaceholderText(text);
}

QSize ShadTransferPanel::sizeHint() const
{
    return QSize(kPreferredWidth, kPreferredHeight);
}

QSize ShadTransferPanel::minimumSizeHint() const
{
    return QSize(kMinimumWidth, kMinimumHeight);
}

bool ShadTransferPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _listView->viewport() && (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick))
    {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            const QModelIndex index = _listView->indexAt(mouseEvent->position().toPoint());
            if (index.isValid())
            {
                _listView->setFocus(Qt::MouseFocusReason);
                _listView->setCurrentIndex(index);
                toggleItem(_model->itemFromIndex(index));
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadTransferPanel::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        updateHeader();
    }
    QWidget::changeEvent(event);
}

void ShadTransferPanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF bounds(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    strokeRoundedRect(&painter, bounds, radius, borderColor);
    const qreal separatorY = _header->geometry().bottom() + 1;
    painter.fillRect(QRectF(kBorderWidth, separatorY, width() - 2 * kBorderWidth, 1), borderColor);
}

void ShadTransferPanel::rebuildModel()
{
    const QString query = _searchInput->text().trimmed();
    _isUpdating = true;
    _model->clear();
    for (const ShadTransferItem& item : std::as_const(_items))
    {
        if (!query.isEmpty() && !item.title.contains(query, Qt::CaseInsensitive))
        {
            continue;
        }
        QStandardItem* row = new QStandardItem(item.title);
        row->setData(item.key, kKeyRole);
        row->setToolTip(item.title);
        if (!item.description.isEmpty())
        {
            row->setData(item.description, ShadListView::DescriptionRole);
        }
        if (item.icon != ShadIconType::None)
        {
            row->setData(static_cast<int>(item.icon), ShadListView::IconRole);
        }
        Qt::ItemFlags flags = Qt::ItemIsUserCheckable;
        if (!item.isDisabled)
        {
            flags |= Qt::ItemIsEnabled;
        }
        row->setFlags(flags);
        row->setCheckState(_checkedKeys.contains(item.key) ? Qt::Checked : Qt::Unchecked);
        _model->appendRow(row);
    }
    _isUpdating = false;
    updateHeader();
}

void ShadTransferPanel::updateHeader()
{
    int enabledCount = 0;
    int checkedCount = 0;
    for (int row = 0; row < _model->rowCount(); ++row)
    {
        const QStandardItem* item = _model->item(row);
        if (!item->isEnabled())
        {
            continue;
        }
        ++enabledCount;
        if (item->checkState() == Qt::Checked)
        {
            ++checkedCount;
        }
    }
    _checkAll->setEnabled(enabledCount > 0);
    if (checkedCount == 0)
    {
        _checkAll->setTristate(false);
        _checkAll->setCheckState(Qt::Unchecked);
    }
    else if (checkedCount == enabledCount)
    {
        _checkAll->setTristate(false);
        _checkAll->setCheckState(Qt::Checked);
    }
    else
    {
        _checkAll->setCheckState(Qt::PartiallyChecked);
    }
    const int totalCount = int(_items.size());
    const int selectedCount = int(checkedKeys().size());
    _countLabel->setText(selectedCount > 0 ? tr("%1/%2 项").arg(selectedCount).arg(totalCount) : tr("%1 项").arg(totalCount));
}

void ShadTransferPanel::toggleItem(QStandardItem* item)
{
    if (item == nullptr || !item->isEnabled())
    {
        return;
    }
    item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
}

void ShadTransferPanel::handleItemChanged(QStandardItem* item)
{
    if (_isUpdating)
    {
        return;
    }
    const QString key = item->data(kKeyRole).toString();
    const bool isChecked = item->checkState() == Qt::Checked;
    if (isChecked == _checkedKeys.contains(key))
    {
        return;
    }
    if (isChecked)
    {
        _checkedKeys.insert(key);
    }
    else
    {
        _checkedKeys.remove(key);
    }
    updateHeader();
    Q_EMIT checkedChanged();
}

void ShadTransferPanel::toggleAllVisible()
{
    bool isAllChecked = true;
    bool hasEnabled = false;
    for (int row = 0; row < _model->rowCount(); ++row)
    {
        const QStandardItem* item = _model->item(row);
        if (item->isEnabled())
        {
            hasEnabled = true;
            isAllChecked = isAllChecked && item->checkState() == Qt::Checked;
        }
    }
    if (!hasEnabled)
    {
        updateHeader();
        return;
    }
    const Qt::CheckState state = isAllChecked ? Qt::Unchecked : Qt::Checked;
    _isUpdating = true;
    for (int row = 0; row < _model->rowCount(); ++row)
    {
        QStandardItem* item = _model->item(row);
        if (!item->isEnabled())
        {
            continue;
        }
        item->setCheckState(state);
        const QString key = item->data(kKeyRole).toString();
        if (state == Qt::Checked)
        {
            _checkedKeys.insert(key);
        }
        else
        {
            _checkedKeys.remove(key);
        }
    }
    _isUpdating = false;
    updateHeader();
    Q_EMIT checkedChanged();
}
