#ifndef SHADTOGGLE_H
#define SHADTOGGLE_H

#include <QAbstractButton>
#include <QSize>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QPaintEvent;
class QResizeEvent;

// 双态切换按钮，选中时为 accent 背景
class ShadTogglePrivate;

class SHADCN_EXPORT ShadToggle : public QAbstractButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadToggle)
    // ShadToggleGroup 需要设置组内位置（圆角与边框）
    friend class ShadToggleGroup;
    Q_PROPERTY(ShadToggleType::Variant variant READ getVariant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(ShadToggleType::Size toggleSize READ getToggleSize WRITE setToggleSize NOTIFY toggleSizeChanged)
    Q_PROPERTY(Qt::Alignment contentAlignment READ getContentAlignment WRITE setContentAlignment)

    public:
        explicit ShadToggle(QWidget* parent = nullptr);
        explicit ShadToggle(const QString& text, QWidget* parent = nullptr);
        explicit ShadToggle(ShadIconType::IconName icon, QWidget* parent = nullptr);
        ~ShadToggle() override;

        void setVariant(ShadToggleType::Variant variant);
        ShadToggleType::Variant getVariant() const;

        void setToggleSize(ShadToggleType::Size size);
        ShadToggleType::Size getToggleSize() const;

        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 内容水平对齐方式，默认居中
        void setContentAlignment(Qt::Alignment alignment);
        Qt::Alignment getContentAlignment() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void variantChanged();
        void toggleSizeChanged();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTOGGLE_H
