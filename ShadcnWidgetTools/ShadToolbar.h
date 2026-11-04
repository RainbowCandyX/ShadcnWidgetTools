#ifndef SHADTOOLBAR_H
#define SHADTOOLBAR_H

#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QEvent;
class QPaintEvent;
class QResizeEvent;
class ShadDropdownMenu;

class ShadToolbarPrivate;

class SHADCN_EXPORT ShadToolbar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadToolbar)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(ShadToolbarType::ButtonStyle buttonStyle READ getButtonStyle WRITE setButtonStyle)
    Q_PROPERTY(bool bordered READ isBordered WRITE setBordered)

    public:
        explicit ShadToolbar(QWidget* parent = nullptr);
        explicit ShadToolbar(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadToolbar() override;

        QAction* addAction(ShadIconType::IconName icon, const QString& text);
        void addAction(QAction* action, ShadIconType::IconName icon = ShadIconType::None);
        ShadDropdownMenu* addMenu(ShadIconType::IconName icon, const QString& text);
        void setMenuTitle(ShadDropdownMenu* menu, const QString& title);
        void addWidget(QWidget* widget);
        void addSeparator();
        void addStretch();
        void removeAction(QAction* action);
        void clear();

        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        void setButtonStyle(ShadToolbarType::ButtonStyle style);
        ShadToolbarType::ButtonStyle getButtonStyle() const;

        void setBordered(bool bordered);
        bool isBordered() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTOOLBAR_H
