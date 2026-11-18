#ifndef SHADRESULT_H
#define SHADRESULT_H

#include <QColor>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class ShadButton;

class ShadResultPrivate;

class SHADCN_EXPORT ShadResult : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadResult)
    Q_PROPERTY(ShadResultType::Status status READ getStatus WRITE setStatus)
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)

    public:
        explicit ShadResult(QWidget* parent = nullptr);
        ShadResult(ShadResultType::Status status, const QString& title, const QString& description = QString(), QWidget* parent = nullptr);
        ~ShadResult() override;

        void setStatus(ShadResultType::Status status);
        ShadResultType::Status getStatus() const;

        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setMediaWidget(QWidget* widget);
        QWidget* getMediaWidget() const;

        void setTitle(const QString& title);
        QString getTitle() const;

        void setDescription(const QString& description);
        QString getDescription() const;

        ShadButton* addButton(const QString& text, ShadButtonType::Variant variant = ShadButtonType::Default);
        void addActionWidget(QWidget* widget);

        void addContentWidget(QWidget* widget);
};

#endif // SHADRESULT_H
