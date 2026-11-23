#ifndef SHADMAPPOPUP_H
#define SHADMAPPOPUP_H

#include <QPointer>
#include <QRectF>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadMapDef.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QVBoxLayout;
class ShadButton;
class ShadLabel;
class ShadMap;

class SHADCN_MAP_EXPORT ShadMapPopup : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(bool closeButtonVisible READ isCloseButtonVisible WRITE setCloseButtonVisible)
    Q_PROPERTY(ShadPlacementType::Side placement READ getPlacement WRITE setPlacement)
    Q_PROPERTY(bool arrowVisible READ isArrowVisible WRITE setArrowVisible)

    public:
        ~ShadMapPopup() override;

        void setCoordinate(const ShadMapCoordinate& coordinate);
        ShadMapCoordinate getCoordinate() const;

        void setTitle(const QString& title);
        QString getTitle() const;

        void setDescription(const QString& description);
        QString getDescription() const;

        void setText(const QString& text);
        QString getText() const;

        void setContentWidget(QWidget* widget);
        QWidget* getContentWidget() const;

        void setCloseButtonVisible(bool visible);
        bool isCloseButtonVisible() const;

        void setPlacement(ShadPlacementType::Side placement);
        ShadPlacementType::Side getPlacement() const;

        void setArrowVisible(bool visible);
        bool isArrowVisible() const;

        void setAnchorOffset(int offset);
        int getAnchorOffset() const;

    Q_SIGNALS:
        void changed();
        void closed();

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        explicit ShadMapPopup(QWidget* parent);
        QRectF cardRect() const;
        void updateLabel(ShadLabel* label, const QString& text);
        void updateMargins();
        void updateCloseButton();
        void retranslate();

        friend class ShadMap;

        ShadMapCoordinate _coordinate;
        QVBoxLayout* _layout{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        ShadLabel* _label{nullptr};
        ShadButton* _closeButton{nullptr};
        QPointer<QWidget> _content;
        ShadPlacementType::Side _placement{ShadPlacementType::Top};
        int _anchorOffset{0};
        bool _isArrowVisible{true};
};

#endif // SHADMAPPOPUP_H
