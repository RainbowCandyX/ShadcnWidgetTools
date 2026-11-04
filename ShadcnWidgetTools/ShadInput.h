#ifndef SHADINPUT_H
#define SHADINPUT_H

#include <QLineEdit>
#include <QSize>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QMouseEvent;
class QPaintEvent;

class ShadInputPrivate;

class SHADCN_EXPORT ShadInput : public QLineEdit
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadInput)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)
    Q_PROPERTY(ShadInputType::TrailingAction trailingAction READ getTrailingAction WRITE setTrailingAction)

    public:
        explicit ShadInput(QWidget* parent = nullptr);
        ~ShadInput() override;

        // 错误态，对应 aria-invalid：边框与焦点环变为 destructive 色
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 文字左侧的 Lucide 图标（muted-foreground 色），传入 None 移除
        void setLeadingIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getLeadingIcon() const;

        // 尾部内置按钮：ClearAction 有文字时显示清空按钮；PasswordToggleAction 切换密码显示，并把 echoMode 设为 Password
        void setTrailingAction(ShadInputType::TrailingAction action);
        ShadInputType::TrailingAction getTrailingAction() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        bool event(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADINPUT_H
