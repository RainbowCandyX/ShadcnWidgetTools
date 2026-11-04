#ifndef SHADBUTTON_H
#define SHADBUTTON_H

#include <QPoint>
#include <QPushButton>
#include <QSize>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;

class ShadButtonPrivate;

class SHADCN_EXPORT ShadButton : public QPushButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadButton)
    Q_PROPERTY(ShadButtonType::Variant variant READ getVariant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(ShadButtonType::Size buttonSize READ getButtonSize WRITE setButtonSize NOTIFY buttonSizeChanged)
    Q_PROPERTY(Qt::Alignment contentAlignment READ getContentAlignment WRITE setContentAlignment)
    Q_PROPERTY(bool loading READ isLoading WRITE setLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool roundedFull READ isRoundedFull WRITE setRoundedFull)

    public:
        explicit ShadButton(QWidget* parent = nullptr);
        explicit ShadButton(const QString& text, QWidget* parent = nullptr);
        ShadButton(const QString& text, ShadButtonType::Variant variant, QWidget* parent = nullptr);
        ~ShadButton() override;

        void setVariant(ShadButtonType::Variant variant);
        ShadButtonType::Variant getVariant() const;

        void setButtonSize(ShadButtonType::Size size);
        ShadButtonType::Size getButtonSize() const;

        // Lucide 图标，绘制在文字左侧；未设置时回退使用 QAbstractButton::icon()
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 内容水平对齐方式，默认居中，对应 justify-start / justify-center / justify-end
        void setContentAlignment(Qt::Alignment alignment);
        Qt::Alignment getContentAlignment() const;

        // 加载状态：图标位置显示旋转的 LoaderCircle，期间不响应鼠标、键盘与快捷键点击
        void setLoading(bool loading);
        bool isLoading() const;

        // 全圆角，对应 className="rounded-full"：圆角半径取按钮高度的一半，图标按钮为圆形；默认关闭（rounded-md）
        void setRoundedFull(bool roundedFull);
        bool isRoundedFull() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void variantChanged();
        void buttonSizeChanged();
        void loadingChanged();

    protected:
        bool event(QEvent* event) override;
        bool hitButton(const QPoint& pos) const override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADBUTTON_H
