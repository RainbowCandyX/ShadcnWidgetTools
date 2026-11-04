#ifndef SHADNAVIGATIONMENULINK_H
#define SHADNAVIGATIONMENULINK_H

#include <QAbstractButton>
#include <QSize>
#include <QString>

#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;

// 导航菜单内容中的链接，点击后关闭导航菜单
class ShadNavigationMenuLinkPrivate;

class SHADCN_EXPORT ShadNavigationMenuLink : public QAbstractButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadNavigationMenuLink)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)

    public:
        explicit ShadNavigationMenuLink(QWidget* parent = nullptr);
        ShadNavigationMenuLink(const QString& title, const QString& description, QWidget* parent = nullptr);
        ~ShadNavigationMenuLink() override;

        void setTitle(const QString& title);
        QString getTitle() const;
        // 最多两行
        void setDescription(const QString& description);
        QString getDescription() const;

        QSize sizeHint() const override;

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADNAVIGATIONMENULINK_H
