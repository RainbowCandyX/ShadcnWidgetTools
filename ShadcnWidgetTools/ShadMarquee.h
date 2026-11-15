#ifndef SHADMARQUEE_H
#define SHADMARQUEE_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QHideEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;

// 跑马灯：一段文字沿水平方向循环滚动
class ShadMarqueePrivate;

class SHADCN_EXPORT ShadMarquee : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadMarquee)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(int speed READ getSpeed WRITE setSpeed)
    Q_PROPERTY(int gap READ getGap WRITE setGap)
    Q_PROPERTY(bool reversed READ isReversed WRITE setReversed)
    Q_PROPERTY(bool pauseOnHover READ isPauseOnHover WRITE setPauseOnHover)
    Q_PROPERTY(bool overflowOnly READ isOverflowOnly WRITE setOverflowOnly)
    Q_PROPERTY(int fadeWidth READ getFadeWidth WRITE setFadeWidth)
    Q_PROPERTY(ShadThemeType::ThemeColor colorRole READ getColorRole WRITE setColorRole)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning)

    public:
        explicit ShadMarquee(QWidget* parent = nullptr);
        explicit ShadMarquee(const QString& text, QWidget* parent = nullptr);
        ~ShadMarquee() override;

        void setText(const QString& text);
        QString getText() const;

        // 滚动速度，0 表示不滚动
        void setSpeed(int speed);
        int getSpeed() const;

        void setGap(int gap);
        int getGap() const;

        void setReversed(bool reversed);
        bool isReversed() const;

        void setPauseOnHover(bool pause);
        bool isPauseOnHover() const;

        // 只在文字超出宽度时滚动
        void setOverflowOnly(bool overflowOnly);
        bool isOverflowOnly() const;

        void setFadeWidth(int width);
        int getFadeWidth() const;

        void setColorRole(ShadThemeType::ThemeColor colorRole);
        ShadThemeType::ThemeColor getColorRole() const;

        void setRunning(bool running);
        bool isRunning() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADMARQUEE_H
