#ifndef SHADLOADINGOVERLAY_H
#define SHADLOADINGOVERLAY_H

#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;

class ShadLoadingOverlayPrivate;

class SHADCN_EXPORT ShadLoadingOverlay : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadLoadingOverlay)
    Q_PROPERTY(bool loading READ isLoading WRITE setLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(int delay READ getDelay WRITE setDelay)
    Q_PROPERTY(int radius READ getRadius WRITE setRadius)
    Q_PROPERTY(int spinnerSize READ getSpinnerSize WRITE setSpinnerSize)

    public:
        explicit ShadLoadingOverlay(QWidget* target = nullptr);
        ~ShadLoadingOverlay() override;

        void setTarget(QWidget* target);
        QWidget* getTarget() const;

        bool isLoading() const;

        void setText(const QString& text);
        QString getText() const;

        void setDelay(int delayMs);
        int getDelay() const;

        void setRadius(int radius);
        int getRadius() const;

        void setSpinnerSize(int size);
        int getSpinnerSize() const;

    public Q_SLOTS:
        void setLoading(bool loading);
        void start();
        void stop();

    Q_SIGNALS:
        void loadingChanged(bool loading);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADLOADINGOVERLAY_H
