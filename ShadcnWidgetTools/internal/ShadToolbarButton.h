#ifndef SHADTOOLBARBUTTON_H
#define SHADTOOLBARBUTTON_H

#include <QAbstractButton>
#include <QPointer>
#include <QSize>

#include "ShadDef.h"
#include "ShadIconType.h"

class QAction;
class QEnterEvent;
class QEvent;
class QPaintEvent;
class ShadDropdownMenu;
class ShadTransition;

class ShadToolbarButton : public QAbstractButton
{
    Q_OBJECT

    public:
        ShadToolbarButton(QAction* action, ShadIconType::IconName icon, ShadDropdownMenu* menu, QWidget* parent = nullptr);
        ~ShadToolbarButton() override;

        QAction* getAction() const;
        ShadDropdownMenu* getMenu() const;
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        void setButtonStyle(ShadToolbarType::ButtonStyle style);
        ShadToolbarType::ButtonStyle getButtonStyle() const;

        void setStartAligned(bool startAligned);
        void setMenuSide(ShadPlacementType::Side side);
        void setChevronVisible(bool visible);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void contentChanged();
        void aboutToShowMenu();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void syncAction();
        void openMenu();
        bool isIconShown() const;
        bool isTextShown() const;
        bool hasChevron() const;

        QPointer<QAction> _action;
        QPointer<ShadDropdownMenu> _menu;
        ShadIconType::IconName _icon{ShadIconType::None};
        ShadToolbarType::ButtonStyle _style{ShadToolbarType::IconOnly};
        ShadPlacementType::Side _menuSide{ShadPlacementType::Bottom};
        bool _isStartAligned{false};
        bool _isChevronVisible{true};
        bool _isMenuOpen{false};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADTOOLBARBUTTON_H
