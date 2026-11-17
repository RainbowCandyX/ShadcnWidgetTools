#ifndef SHADTRANSFERPANEL_H
#define SHADTRANSFERPANEL_H

#include <QList>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadTransfer.h"

class QEvent;
class QObject;
class QPaintEvent;
class QStandardItem;
class QStandardItemModel;
class QWidget;
class ShadCheckbox;
class ShadInput;
class ShadLabel;
class ShadListView;

class ShadTransferPanel : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadTransferPanel(QWidget* parent = nullptr);
        ~ShadTransferPanel() override;

        void setTitle(const QString& title);
        QString getTitle() const;

        void setItems(const QList<ShadTransferItem>& items);
        QStringList checkedKeys() const;
        void uncheckKeys(const QStringList& keys);

        void setSearchEnabled(bool enabled);
        void setSearchPlaceholderText(const QString& text);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void checkedChanged();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void rebuildModel();
        void updateHeader();
        void toggleItem(QStandardItem* item);
        void handleItemChanged(QStandardItem* item);
        void toggleAllVisible();

        QWidget* _header{nullptr};
        ShadCheckbox* _checkAll{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _countLabel{nullptr};
        QWidget* _searchContainer{nullptr};
        ShadInput* _searchInput{nullptr};
        ShadListView* _listView{nullptr};
        QStandardItemModel* _model{nullptr};
        QList<ShadTransferItem> _items;
        QSet<QString> _checkedKeys;
        bool _isUpdating{false};
};

#endif // SHADTRANSFERPANEL_H
