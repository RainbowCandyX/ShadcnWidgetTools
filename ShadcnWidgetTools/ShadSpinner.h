#ifndef SHADSPINNER_H
#define SHADSPINNER_H

#include <QColor>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;

// 加载指示，对应 shadcn Spinner：Lucide LoaderCircle（Loader2Icon）size-4 animate-spin，1s 匀速旋转一周
// 控件尺寸固定为图标尺寸；隐藏时暂停动画，显示后继续；禁用时整体 opacity-50；RTL 下旋转方向不变
class ShadSpinnerPrivate;

class SHADCN_EXPORT ShadSpinner : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSpinner)
    Q_PROPERTY(int spinnerSize READ getSpinnerSize WRITE setSpinnerSize)
    Q_PROPERTY(ShadThemeType::ThemeColor colorRole READ getColorRole WRITE setColorRole)
    Q_PROPERTY(QColor color READ getColor WRITE setColor)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning)

    public:
        explicit ShadSpinner(QWidget* parent = nullptr);
        ~ShadSpinner() override;

        // 旋转的 Lucide 图标，默认 LoaderCircle；传 ShadIconType::None 时不绘制
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 图标尺寸（像素），默认 16（size-4），size-3 / size-6 / size-8 对应 12 / 24 / 32
        void setSpinnerSize(int size);
        int getSpinnerSize() const;

        // 图标颜色取主题色，默认 Foreground，跟随主题切换
        void setColorRole(ShadThemeType::ThemeColor colorRole);
        ShadThemeType::ThemeColor getColorRole() const;
        // 固定颜色，设置后优先于 colorRole；传无效的 QColor() 恢复使用 colorRole
        void setColor(const QColor& color);
        QColor getColor() const;

        // 是否旋转，默认 true；停止时显示静止的图标
        void setRunning(bool running);
        bool isRunning() const;

        QSize sizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSPINNER_H
