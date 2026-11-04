#ifndef SHADCOLORPICKERPRIVATE_H
#define SHADCOLORPICKERPRIVATE_H

#include <QColor>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadButton;
class ShadColorArea;
class ShadColorPicker;
class ShadColorPreview;
class ShadColorSwatches;
class ShadComboBox;
class ShadHueSlider;
class ShadInput;
class ShadLabel;
class ShadScreenColorPicker;
class ShadSpinBox;

class ShadColorPickerPrivate
{
    SHAD_D_CREATE(ShadColorPicker)

    public:
        ShadColorPickerPrivate();
        ~ShadColorPickerPrivate();

        // 颜色变化的来源：来源控件自身不再回写，避免打断正在进行的拖动或输入
        enum class Source
        {
            None,
            Area,
            Hue,
            Hex,
            SpinBox,
        };

        static constexpr int kCustomSlotCount = 12;
        static constexpr int kSpinBoxCount = 3;

        QColor currentColor() const;
        // 按 HSV 设置当前颜色并同步各控件，RGB 变化时发出 colorChanged
        void applyHsv(qreal hue, qreal saturation, qreal value, Source source);
        // 按 QColor 设置当前颜色；饱和度或明度为 0 时保留原来的色相（及明度为 0 时的饱和度）
        void applyColor(const QColor& color, Source source);
        void updateHexInput();
        void updateSpinBoxes();
        // 按模式设置数值输入框的标签、范围与后缀
        void updateSpinBoxRanges();
        void onHexEdited(const QString& text);
        void onHexEditingFinished();
        void onSpinBoxValueChanged();
        void addCustomColor();
        void removeCustomColor();
        // 选中了有颜色的槽时才能移除
        void updateCustomButtons();

        ShadColorArea* _area{nullptr};
        ShadHueSlider* _hueSlider{nullptr};
        ShadColorPreview* _preview{nullptr};
        ShadInput* _hexInput{nullptr};
        ShadButton* _screenPickButton{nullptr};
        // 屏幕取色控制器，第一次取色时创建
        ShadScreenColorPicker* _screenPicker{nullptr};
        bool _isScreenPickingEnabled{true};
        bool _isOwnWindowsVisibleWhilePicking{true};
        ShadComboBox* _modeComboBox{nullptr};
        ShadLabel* _spinBoxLabels[kSpinBoxCount]{};
        ShadSpinBox* _spinBoxes[kSpinBoxCount]{};
        ShadColorSwatches* _presetSwatches{nullptr};
        ShadColorSwatches* _customSwatches{nullptr};
        ShadButton* _addCustomButton{nullptr};
        ShadButton* _removeCustomButton{nullptr};
        // 基本颜色与自定义颜色的分区标题，切换语言时刷新
        ShadLabel* _presetTitle{nullptr};
        ShadLabel* _customTitle{nullptr};

        qreal _hue{0.0};
        qreal _saturation{0.0};
        qreal _value{1.0};
        QColor _originalColor;
        ShadColorPickerType::Mode _mode{ShadColorPickerType::Rgb};
};

#endif // SHADCOLORPICKERPRIVATE_H
