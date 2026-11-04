#ifndef SHADCHECKBOX_H
#define SHADCHECKBOX_H

#include <QCheckBox>
#include <QPoint>
#include <QSize>
#include <QString>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QPaintEvent;

// 复选框，支持三态（PartiallyChecked 绘制为横线）
class ShadCheckboxPrivate;

class SHADCN_EXPORT ShadCheckbox : public QCheckBox
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCheckbox)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)

    public:
        explicit ShadCheckbox(QWidget* parent = nullptr);
        explicit ShadCheckbox(const QString& text, QWidget* parent = nullptr);
        ~ShadCheckbox() override;

        void setInvalid(bool invalid);
        bool isInvalid() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        bool hitButton(const QPoint& pos) const override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCHECKBOX_H
