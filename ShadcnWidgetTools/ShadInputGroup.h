#ifndef SHADINPUTGROUP_H
#define SHADINPUTGROUP_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class ShadButton;
class ShadInput;
class ShadLabel;
class ShadTextarea;

// 输入组，对应 shadcn InputGroup：外框（border-input h-9 rounded-md shadow-xs，暗色 bg-input/30）包住内部输入框与附加区
// 内部输入框获得焦点时外框显示 border-ring 与焦点环；内部输入框以嵌入模式显示，不绘制自身的外框
// 附加区（InputGroupAddon）可放文字、图标、按钮、Kbd 等；点击附加区空白处时聚焦输入框
// 四周预留焦点环边距；RTL 下起始侧为右侧；可以放进 ShadButtonGroup 与相邻组件拼接
class ShadInputGroupPrivate;

class SHADCN_EXPORT ShadInputGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadInputGroup)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid)

    public:
        // 默认内部为单行 ShadInput
        explicit ShadInputGroup(QWidget* parent = nullptr);
        explicit ShadInputGroup(ShadInputGroupType::Control control, QWidget* parent = nullptr);
        ~ShadInputGroup() override;

        // 内部输入组件，类型不符时返回 nullptr
        ShadInput* getInput() const;
        ShadTextarea* getTextarea() const;

        // 错误态：border-destructive 与 destructive 焦点环
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 以下函数在指定附加区末尾追加一项，附加区内各项间距 gap-2
        // 文字（InputGroupText）：text-sm font-medium text-muted-foreground
        ShadLabel* addText(ShadInputGroupType::AddonPosition position, const QString& text);
        // 图标 size-4，muted-foreground 色；LoaderCircle 自动旋转，可作为加载指示
        QWidget* addIcon(ShadInputGroupType::AddonPosition position, ShadIconType::IconName icon);
        // 按钮（InputGroupButton）：ghost 变体，默认 ExtraSmall（xs），也可用 Small、IconExtraSmall、IconSmall
        ShadButton* addButton(ShadInputGroupType::AddonPosition position, const QString& text, ShadIconType::IconName icon = ShadIconType::None, ShadButtonType::Size size = ShadButtonType::ExtraSmall);
        // 任意组件，例如 ShadKbd、ShadKbdGroup；输入组接管组件的父子关系
        void addWidget(ShadInputGroupType::AddonPosition position, QWidget* widget);
        // 在 BlockStart / BlockEnd 附加区插入弹性空白，之后的项靠结束侧排列（对应 ml-auto）
        void addStretch(ShadInputGroupType::AddonPosition position);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADINPUTGROUP_H
