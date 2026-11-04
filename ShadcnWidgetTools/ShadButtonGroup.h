#ifndef SHADBUTTONGROUP_H
#define SHADBUTTONGROUP_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;

// 按钮组中的文字块，对应 ButtonGroupText：bg-muted border rounded-md px-4 text-sm font-medium shadow-xs，可带 size-4 图标
// 四周预留焦点环边距，与按钮对齐
class ShadButtonGroupTextPrivate;

class SHADCN_EXPORT ShadButtonGroupText : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadButtonGroupText)
    Q_PROPERTY(QString text READ getText WRITE setText)

    public:
        explicit ShadButtonGroupText(QWidget* parent = nullptr);
        explicit ShadButtonGroupText(const QString& text, QWidget* parent = nullptr);
        ~ShadButtonGroupText() override;

        void setText(const QString& text);
        QString getText() const;

        // 文字前的 Lucide 图标，None 表示不显示
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

// 按钮组，对应 shadcn ButtonGroup：子组件紧贴排列，相邻处为直角且只保留一条边框（ShadButton、ShadInput、ShadComboBox、ShadButtonGroupText 支持拼接外观）
// 子组件等高（纵向时等宽），输入框（QLineEdit）平分多余的宽度；获得焦点的子组件提到最上层，焦点环不被相邻组件遮挡
// 直接子组件中有 ShadButtonGroup 时为嵌套模式：各项之间留 8px（gap-2），不做拼接
// 子组件按本库约定四周预留焦点环边距；RTL 下横向排列从右向左
class ShadButtonGroupPrivate;

class SHADCN_EXPORT ShadButtonGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadButtonGroup)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)

    public:
        explicit ShadButtonGroup(QWidget* parent = nullptr);
        explicit ShadButtonGroup(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadButtonGroup() override;

        // 默认 Qt::Horizontal；纵向时上下拼接
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        // 添加到末尾，分组接管组件的父子关系
        void addWidget(QWidget* widget);
        // index 超出范围时添加到末尾（分隔线也计入序号）
        void insertWidget(int index, QWidget* widget);
        // 从分组中移除但不删除，恢复独立外观，父组件置空，由调用方负责释放
        void removeWidget(QWidget* widget);
        // 对应 ButtonGroupSeparator：1px 的 bg-input 分隔线，用于没有边框的按钮（例如 secondary）之间
        void addSeparator();
        // 对应 ButtonGroupText，返回的文字块由分组持有
        ShadButtonGroupText* addText(const QString& text, ShadIconType::IconName icon = ShadIconType::None);
        // 项数，分隔线也计入
        int count() const;
        // 第 index 项的组件，分隔线返回 nullptr
        QWidget* widgetAt(int index) const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADBUTTONGROUP_H
