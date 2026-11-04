#ifndef SHADMARKER_H
#define SHADMARKER_H

#include <QSize>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class ShadMarkerPrivate;

class SHADCN_EXPORT ShadMarker : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadMarker)
    Q_PROPERTY(ShadMarkerType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(bool shimmer READ isShimmer WRITE setShimmer)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable)
    Q_PROPERTY(QUrl url READ getUrl WRITE setUrl)

    public:
        explicit ShadMarker(QWidget* parent = nullptr);
        explicit ShadMarker(const QString& text, QWidget* parent = nullptr);
        ShadMarker(ShadIconType::IconName icon, const QString& text, QWidget* parent = nullptr);
        ~ShadMarker() override;

        void setVariant(ShadMarkerType::Variant variant);
        ShadMarkerType::Variant getVariant() const;

        void setText(const QString& text);
        QString getText() const;

        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        void setIconWidget(QWidget* widget);
        QWidget* getIconWidget() const;

        void setShimmer(bool shimmer);
        bool isShimmer() const;

        void setClickable(bool clickable);
        bool isClickable() const;

        void setUrl(const QUrl& url);
        QUrl getUrl() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void clicked();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADMARKER_H
