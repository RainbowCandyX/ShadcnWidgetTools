#ifndef SHADSTATISTIC_H
#define SHADSTATISTIC_H

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;

class ShadStatisticPrivate;

class SHADCN_EXPORT ShadStatistic : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadStatistic)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(double value READ getValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int precision READ getPrecision WRITE setPrecision)
    Q_PROPERTY(QString prefix READ getPrefix WRITE setPrefix)
    Q_PROPERTY(QString suffix READ getSuffix WRITE setSuffix)
    Q_PROPERTY(ShadStatisticType::Trend trend READ getTrend WRITE setTrend)

    public:
        explicit ShadStatistic(QWidget* parent = nullptr);
        explicit ShadStatistic(const QString& title, QWidget* parent = nullptr);
        ~ShadStatistic() override;

        void setTitle(const QString& title);
        QString getTitle() const;
        void setTitleIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getTitleIcon() const;

        void setValue(double value);
        double getValue() const;
        void setPrecision(int precision);
        int getPrecision() const;
        void setGroupSeparatorShown(bool isShown);
        bool isGroupSeparatorShown() const;
        void setValueFontSize(int pixelSize);
        int getValueFontSize() const;

        void setPrefix(const QString& prefix);
        QString getPrefix() const;
        void setSuffix(const QString& suffix);
        QString getSuffix() const;
        void setPrefixIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getPrefixIcon() const;

        void setTrend(ShadStatisticType::Trend trend);
        ShadStatisticType::Trend getTrend() const;

        void setAnimationDuration(int milliseconds);
        int getAnimationDuration() const;

        void setCountdown(const QDateTime& target, const QString& format = QStringLiteral("HH:mm:ss"));
        bool isCountdownActive() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void valueChanged(double value);
        void finished();

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif
