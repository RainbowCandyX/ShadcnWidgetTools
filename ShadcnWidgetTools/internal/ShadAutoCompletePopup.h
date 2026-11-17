#ifndef SHADAUTOCOMPLETEPOPUP_H
#define SHADAUTOCOMPLETEPOPUP_H

#include <QList>
#include <QRect>
#include <QString>
#include <QWidget>

#include "ShadComboBoxPopup.h"

class QHideEvent;
class QPaintEvent;
class QVariantAnimation;
class ShadScrollArea;

class ShadAutoCompletePopup : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadAutoCompletePopup(QWidget* anchor);
        ~ShadAutoCompletePopup() override;

        void setItems(const QList<ShadComboBoxPopupItem>& items);
        bool isEmpty() const;
        void setMaxVisibleItems(int count);

        void open();
        void updatePopupGeometry();

        void moveHighlight(int step);
        int highlightedRow() const;
        QString highlightedText() const;

    Q_SIGNALS:
        void rowClicked(int row);
        void popupHidden();

    protected:
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void layoutContent();
        void scrollToHighlight();
        QRect cardRect() const;

        QWidget* _anchor{nullptr};
        ShadScrollArea* _scrollArea{nullptr};
        ShadComboBoxList* _list{nullptr};
        QVariantAnimation* _openAnimation{nullptr};
        qreal _openProgress{1.0};
        bool _isAbove{false};
        int _maxVisibleItems{8};
};

#endif // SHADAUTOCOMPLETEPOPUP_H
