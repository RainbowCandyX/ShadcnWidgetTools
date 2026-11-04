#ifndef SHADSTATUSBAR_H
#define SHADSTATUSBAR_H

#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class ShadLabel;

class ShadStatusBarPrivate;

class SHADCN_EXPORT ShadStatusBar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadStatusBar)
    Q_PROPERTY(bool separatorVisible READ isSeparatorVisible WRITE setSeparatorVisible)

    public:
        explicit ShadStatusBar(QWidget* parent = nullptr);
        ~ShadStatusBar() override;

        void addWidget(QWidget* widget, int stretch = 0);
        void addPermanentWidget(QWidget* widget, int stretch = 0);
        ShadLabel* addText(const QString& text);
        void removeWidget(QWidget* widget);

        void showMessage(const QString& message, int timeout = 0);
        void clearMessage();
        QString currentMessage() const;

        void setSeparatorVisible(bool visible);
        bool isSeparatorVisible() const;

    Q_SIGNALS:
        void messageChanged(const QString& message);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSTATUSBAR_H
