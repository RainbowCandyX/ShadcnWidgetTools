#ifndef SHADFIELDSEPARATOR_H
#define SHADFIELDSEPARATOR_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QPaintEvent;

// 表单分隔线，对应 shadcn FieldSeparator：高 20（h-5），中间一条 1px border 色横线
// 可带居中文字（例如"或者"），text-sm text-muted-foreground px-2，横线在文字处断开
class ShadFieldSeparatorPrivate;

class SHADCN_EXPORT ShadFieldSeparator : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadFieldSeparator)
    Q_PROPERTY(QString text READ getText WRITE setText)

    public:
        explicit ShadFieldSeparator(QWidget* parent = nullptr);
        explicit ShadFieldSeparator(const QString& text, QWidget* parent = nullptr);
        ~ShadFieldSeparator() override;

        void setText(const QString& text);
        QString getText() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADFIELDSEPARATOR_H
