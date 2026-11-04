#ifndef SHADBADGE_H
#define SHADBADGE_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;

class ShadBadgePrivate;

class SHADCN_EXPORT ShadBadge : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadBadge)
    Q_PROPERTY(QString text READ getText WRITE setText NOTIFY textChanged)
    Q_PROPERTY(ShadBadgeType::Variant variant READ getVariant WRITE setVariant NOTIFY variantChanged)

    public:
        explicit ShadBadge(QWidget* parent = nullptr);
        explicit ShadBadge(const QString& text, QWidget* parent = nullptr);
        ShadBadge(const QString& text, ShadBadgeType::Variant variant, QWidget* parent = nullptr);
        ~ShadBadge() override;

        void setText(const QString& text);
        QString getText() const;

        void setVariant(ShadBadgeType::Variant variant);
        ShadBadgeType::Variant getVariant() const;

        // 文字左侧的 Lucide 图标，尺寸 size-3；LoaderCircle 在徽章可见时自动旋转（对应 Spinner）
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void textChanged();
        void variantChanged();

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADBADGE_H
