#ifndef SHADALERT_H
#define SHADALERT_H

#include <QColor>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;

// 提示，对应 shadcn Alert：左侧 Lucide 图标（可选），右侧标题与描述，右上角可放一个操作组件（AlertAction）
// 标题与描述自动换行，高度随宽度计算（heightForWidth）
class ShadAlertPrivate;

class SHADCN_EXPORT ShadAlert : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAlert)
    Q_PROPERTY(ShadAlertType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)

    public:
        explicit ShadAlert(QWidget* parent = nullptr);
        explicit ShadAlert(ShadIconType::IconName icon, const QString& title, const QString& description = QString(), ShadAlertType::Variant variant = ShadAlertType::Default, QWidget* parent = nullptr);
        ~ShadAlert() override;

        void setVariant(ShadAlertType::Variant variant);
        ShadAlertType::Variant getVariant() const;

        // 左侧图标，传入 None 移除
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 标题或描述为空时不显示对应行
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        // 右上角的操作组件（例如按钮），旧组件会被删除，传 nullptr 只移除；设置后文字右侧预留 72px（pr-18）
        void setAction(QWidget* action);
        QWidget* getAction() const;

        // 自定义颜色，覆盖变体颜色：分别设置亮色与暗色主题下的背景、边框与前景（标题与图标）颜色，描述保持 muted-foreground
        // 对应 shadcn 示例中的 border-amber-200 bg-amber-50 text-amber-900 dark:border-amber-900 dark:bg-amber-950 dark:text-amber-50
        void setCustomColors(ShadThemeType::ThemeMode themeMode, const QColor& background, const QColor& border, const QColor& foreground);
        void clearCustomColors();

        // 不使用内部布局，按宽度手动计算高度：计算前把宽度夹到 maximumWidth 以内
        // （QWidgetItem 计算带布局组件的高度时不会考虑最大宽度，设置 max-w 后高度会算小）
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADALERT_H
