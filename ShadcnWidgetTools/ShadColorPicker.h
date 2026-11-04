#ifndef SHADCOLORPICKER_H
#define SHADCOLORPICKER_H

#include <QColor>
#include <QList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;

// 颜色选择面板，可以直接放进页面、卡片或浮层，也由 ShadColorDialog 使用
// 左列：饱和度与明度方块（240x240，背景为当前色相），下方为色相滑条；方块与滑条的方向不随布局方向镜像
// 右列：预览条（上半当前颜色，点击下半恢复原始颜色）、十六进制输入（#RRGGBB）与吸管按钮、模式选择与三个数值输入框
// 吸管（屏幕取色）：冻结所有屏幕的画面，光标旁的放大镜显示周围像素与颜色值，单击或 Enter 取色，方向键微调 1 个物理像素，Esc 或右键取消
// macOS 与 Windows 上取色时高亮光标所在的窗口
// 屏幕取色在 Wayland 下不可用（吸管按钮隐藏）；macOS 需要屏幕录制权限，没有权限时请求授权，本次不开始取色
// 下方：基本颜色（48 个，12 列）与自定义颜色（12 个槽，选中槽后 + 写入当前颜色、− 清空）
// 内部按 HSV 保存颜色：饱和度或明度为 0 时色相不确定，保留原来的色相与饱和度，拖动明度不会让取色点跳动
class ShadColorPickerPrivate;

class SHADCN_EXPORT ShadColorPicker : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadColorPicker)
    Q_PROPERTY(QColor color READ getColor WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor originalColor READ getOriginalColor WRITE setOriginalColor)
    Q_PROPERTY(ShadColorPickerType::Mode mode READ getMode WRITE setMode)
    Q_PROPERTY(bool screenPickingEnabled READ isScreenPickingEnabled WRITE setScreenPickingEnabled)
    Q_PROPERTY(bool ownWindowsVisibleWhilePicking READ isOwnWindowsVisibleWhilePicking WRITE setOwnWindowsVisibleWhilePicking)

    public:
        explicit ShadColorPicker(QWidget* parent = nullptr);
        ~ShadColorPicker() override;

        // 当前颜色（不含透明度），默认白色；无效颜色被忽略
        void setColor(const QColor& color);
        QColor getColor() const;

        // 预览条下半显示的原始颜色，点击后恢复为该颜色；无效颜色时下半显示当前颜色且不可点击
        void setOriginalColor(const QColor& color);
        QColor getOriginalColor() const;

        // 数值输入框的模式，默认 Rgb
        void setMode(ShadColorPickerType::Mode mode);
        ShadColorPickerType::Mode getMode() const;

        // 基本颜色，默认为 defaultPresetColors()
        void setPresetColors(const QList<QColor>& colors);
        QList<QColor> getPresetColors() const;
        static QList<QColor> defaultPresetColors();

        // 自定义颜色，固定 12 个槽，无效颜色表示空槽；多出的颜色被忽略，不足时补空槽
        void setCustomColors(const QList<QColor>& colors);
        QList<QColor> getCustomColors() const;

        // 是否显示吸管按钮，默认 true；平台不支持屏幕取色时始终隐藏
        void setScreenPickingEnabled(bool enabled);
        bool isScreenPickingEnabled() const;
        // 屏幕取色时是否包含本程序的窗口，默认 true；为 false 时取色期间本程序窗口全透明，取到窗口背后的颜色（Linux X11 无合成器时不生效）
        void setOwnWindowsVisibleWhilePicking(bool visible);
        bool isOwnWindowsVisibleWhilePicking() const;
        // 当前平台能否屏幕取色（Wayland 下为 false）
        static bool isScreenPickingSupported();
        // 开始屏幕取色，与点击吸管按钮相同；无法开始时直接发出 screenPickingFinished(false)
        void pickScreenColor();

    Q_SIGNALS:
        void colorChanged(const QColor& color);
        // 通过 + / − 按钮或 setCustomColors 修改自定义颜色时发出
        void customColorsChanged();
        // 屏幕取色结束，picked 表示是否取到了颜色（取到时已设置为当前颜色）
        void screenPickingFinished(bool picked);

    protected:
        void changeEvent(QEvent* event) override;
};

#endif // SHADCOLORPICKER_H
