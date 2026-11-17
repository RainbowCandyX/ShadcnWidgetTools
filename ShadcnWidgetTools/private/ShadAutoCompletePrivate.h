#ifndef SHADAUTOCOMPLETEPRIVATE_H
#define SHADAUTOCOMPLETEPRIVATE_H

#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QWidget>

#include "ShadComboBoxPopup.h"
#include "ShadDef.h"

class QAbstractItemModel;
class QStringListModel;
class ShadAutoComplete;
class ShadAutoCompletePopup;

class ShadAutoCompletePrivate
{
    public:
        ShadAutoCompletePrivate();
        ~ShadAutoCompletePrivate();

        static constexpr int kDefaultMaxVisibleItems = 8;

        QAbstractItemModel* activeModel() const;
        void connectModel();
        void disconnectModel();
        QList<ShadComboBoxPopupItem> filteredItems() const;
        void refresh(bool open);
        void select(int row);
        void watchOutside(bool watch);

        ShadAutoComplete* q{nullptr};
        ShadAutoCompletePopup* _popup{nullptr};
        QStringListModel* _defaultModel{nullptr};
        QPointer<QAbstractItemModel> _customModel;
        QList<QMetaObject::Connection> _modelConnections;
        QPointer<QWidget> _watchedWindow;
        ShadAutoCompleteType::FilterMode _filterMode{ShadAutoCompleteType::Fuzzy};
        int _maxVisibleItems{kDefaultMaxVisibleItems};
        bool _isWatchingOutside{false};
};

#endif // SHADAUTOCOMPLETEPRIVATE_H
