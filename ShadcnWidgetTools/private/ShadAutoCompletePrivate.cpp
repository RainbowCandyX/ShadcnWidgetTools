#include "ShadAutoCompletePrivate.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QIcon>
#include <QMetaType>
#include <QModelIndex>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringListModel>
#include <QVariant>
#include <QtCore/qnamespace.h>

#include <algorithm>
#include <utility>

#include "ShadAutoComplete.h"
#include "ShadAutoCompletePopup.h"
#include "ShadComboBoxSearch.h"

ShadAutoCompletePrivate::ShadAutoCompletePrivate()
{
}

ShadAutoCompletePrivate::~ShadAutoCompletePrivate()
{
}

QAbstractItemModel* ShadAutoCompletePrivate::activeModel() const
{
    if (!_customModel.isNull())
    {
        return _customModel.data();
    }
    return _defaultModel;
}

void ShadAutoCompletePrivate::connectModel()
{
    QAbstractItemModel* model = activeModel();
    if (model == nullptr)
    {
        return;
    }
    const auto onModelChanged = [this]()
    {
        if (_popup->isVisible())
        {
            refresh(false);
        }
        else if (q->hasFocus() && !q->text().isEmpty())
        {
            refresh(true);
        }
    };
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsInserted, q, onModelChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsRemoved, q, onModelChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsMoved, q, onModelChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::modelReset, q, onModelChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::layoutChanged, q, onModelChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::dataChanged, q, onModelChanged));
}

void ShadAutoCompletePrivate::disconnectModel()
{
    for (const QMetaObject::Connection& connection : std::as_const(_modelConnections))
    {
        QObject::disconnect(connection);
    }
    _modelConnections.clear();
}

QList<ShadComboBoxPopupItem> ShadAutoCompletePrivate::filteredItems() const
{
    QList<ShadComboBoxPopupItem> items;
    const QAbstractItemModel* model = activeModel();
    if (model == nullptr)
    {
        return items;
    }
    const QString query = q->text();
    const bool isFuzzySearch = _filterMode == ShadAutoCompleteType::Fuzzy && !query.trimmed().isEmpty();
    QList<std::pair<int, ShadComboBoxPopupItem>> scoredItems;
    const int rowCount = model->rowCount();
    for (int row = 0; row < rowCount; ++row)
    {
        const QModelIndex index = model->index(row, 0);
        ShadComboBoxPopupItem item;
        item.row = row;
        item.text = index.data(Qt::DisplayRole).toString();
        if (item.text.isEmpty())
        {
            continue;
        }
        const QVariant decoration = index.data(Qt::DecorationRole);
        item.icon = decoration.userType() == QMetaType::QPixmap ? QIcon(decoration.value<QPixmap>()) : decoration.value<QIcon>();
        item.isEnabled = model->flags(index).testFlag(Qt::ItemIsEnabled);
        int score = 0;
        if (isFuzzySearch)
        {
            score = ShadComboBoxSearch::fuzzyScore(item.text, query);
            if (score <= 0)
            {
                continue;
            }
        }
        else if (_filterMode == ShadAutoCompleteType::Prefix && !query.isEmpty() && !item.text.startsWith(query, Qt::CaseInsensitive))
        {
            continue;
        }
        scoredItems.append(std::make_pair(score, item));
    }
    if (isFuzzySearch)
    {
        std::stable_sort(scoredItems.begin(), scoredItems.end(), [](const std::pair<int, ShadComboBoxPopupItem>& left, const std::pair<int, ShadComboBoxPopupItem>& right) { return left.first > right.first; });
    }
    items.reserve(scoredItems.size());
    for (const std::pair<int, ShadComboBoxPopupItem>& scoredItem : std::as_const(scoredItems))
    {
        items.append(scoredItem.second);
    }
    return items;
}

void ShadAutoCompletePrivate::refresh(bool open)
{
    const QList<ShadComboBoxPopupItem> items = filteredItems();
    if (items.isEmpty())
    {
        _popup->hide();
        return;
    }
    _popup->setItems(items);
    if ((open || _popup->isVisible()) && q->isVisible())
    {
        _popup->open();
        watchOutside(true);
    }
}

void ShadAutoCompletePrivate::select(int row)
{
    const QAbstractItemModel* model = activeModel();
    if (model == nullptr || row < 0 || row >= model->rowCount())
    {
        return;
    }
    const QString text = model->index(row, 0).data(Qt::DisplayRole).toString();
    _popup->hide();
    q->setText(text);
    Q_EMIT q->optionSelected(text, row);
}

void ShadAutoCompletePrivate::watchOutside(bool watch)
{
    if (_isWatchingOutside == watch)
    {
        return;
    }
    _isWatchingOutside = watch;
    if (watch)
    {
        qApp->installEventFilter(q);
        _watchedWindow = q->window();
        _watchedWindow->installEventFilter(q);
        return;
    }
    qApp->removeEventFilter(q);
    if (!_watchedWindow.isNull())
    {
        _watchedWindow->removeEventFilter(q);
    }
    _watchedWindow = nullptr;
}
