#ifndef SHADLABEL_H
#define SHADLABEL_H

#include <QLabel>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QMouseEvent;
class QObject;

class ShadLabelPrivate;

class SHADCN_EXPORT ShadLabel : public QLabel
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadLabel)
    Q_PROPERTY(ShadThemeType::ThemeColor colorRole READ getColorRole WRITE setColorRole)

    public:
        explicit ShadLabel(QWidget* parent = nullptr);
        explicit ShadLabel(const QString& text, QWidget* parent = nullptr);
        ~ShadLabel() override;

        // 文字颜色使用的主题 token，默认 Foreground
        void setColorRole(ShadThemeType::ThemeColor colorRole);
        ShadThemeType::ThemeColor getColorRole() const;

        // 隐藏 QLabel::setBuddy：额外监听 buddy 的启用状态，buddy 禁用时标签变为 50% 透明度（对应 peer-disabled:opacity-50）
        // 注意需通过 ShadLabel 指针调用，经 QLabel 指针调用时不会监听
        void setBuddy(QWidget* buddy);

        bool eventFilter(QObject* watched, QEvent* event) override;

    protected:
        // 点击标签时触发 buddy：按钮类组件切换状态，其余组件获得焦点（对应 <label for>）
        void mouseReleaseEvent(QMouseEvent* event) override;
};

#endif // SHADLABEL_H
