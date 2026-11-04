#include "ShadColorPickerPrivate.h"

#include <QChar>
#include <QList>
#include <QSignalBlocker>
#include <QString>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadColorArea.h"
#include "ShadColorPicker.h"
#include "ShadColorPreview.h"
#include "ShadColorSwatches.h"
#include "ShadHueSlider.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadSpinBox.h"
#include "ShadStyleHelper.h"

namespace
{
    constexpr int kRgbMaximum = 255;
    constexpr int kHueMaximum = 359;
    constexpr int kPercentMaximum = 100;
    constexpr qreal kHueDegrees = 360.0;
    constexpr qreal kPercent = 100.0;
    // #RGB 与 #RRGGBB 的长度
    constexpr int kShortHexLength = 4;
    constexpr int kHexLength = 7;

    // 补上开头的 #
    QString normalizedHex(const QString& text)
    {
        const QString trimmed = text.trimmed();
        return trimmed.startsWith(QLatin1Char('#')) ? trimmed : QStringLiteral("#") + trimmed;
    }
} // namespace

ShadColorPickerPrivate::ShadColorPickerPrivate()
{
}

ShadColorPickerPrivate::~ShadColorPickerPrivate()
{
}

QColor ShadColorPickerPrivate::currentColor() const
{
    return QColor::fromHsvF(_hue, _saturation, _value).toRgb();
}

void ShadColorPickerPrivate::applyHsv(qreal hue, qreal saturation, qreal value, Source source)
{
    Q_Q(ShadColorPicker);
    const QColor oldColor = currentColor();
    _hue = qBound(0.0, hue, 1.0);
    _saturation = qBound(0.0, saturation, 1.0);
    _value = qBound(0.0, value, 1.0);
    _area->setHue(_hue);
    if (source != Source::Area)
    {
        _area->setSaturationValue(_saturation, _value);
    }
    if (source != Source::Hue)
    {
        _hueSlider->setHue(_hue);
    }
    const QColor newColor = currentColor();
    _preview->setCurrentColor(newColor);
    if (source != Source::Hex)
    {
        updateHexInput();
    }
    if (source != Source::SpinBox)
    {
        updateSpinBoxes();
    }
    if (newColor.rgb() != oldColor.rgb())
    {
        Q_EMIT q->colorChanged(newColor);
    }
}

void ShadColorPickerPrivate::applyColor(const QColor& color, Source source)
{
    if (!color.isValid())
    {
        return;
    }
    const QColor hsvColor = color.toHsv();
    const qreal value = hsvColor.valueF();
    // 明度为 0（黑色）时饱和度与色相都不确定，饱和度为 0（灰色）时色相不确定，保留原来的值
    const qreal saturation = value > 0.0 ? hsvColor.hsvSaturationF() : _saturation;
    const qreal hue = (hsvColor.hsvHueF() >= 0.0 && saturation > 0.0 && value > 0.0) ? hsvColor.hsvHueF() : _hue;
    applyHsv(hue, saturation, value, source);
}

void ShadColorPickerPrivate::updateHexInput()
{
    _hexInput->setText(currentColor().name(QColor::HexRgb).toUpper());
}

void ShadColorPickerPrivate::updateSpinBoxes()
{
    const QColor color = currentColor();
    int values[kSpinBoxCount]{};
    if (_mode == ShadColorPickerType::Rgb)
    {
        values[0] = color.red();
        values[1] = color.green();
        values[2] = color.blue();
    }
    else
    {
        values[0] = qRound(_hue * kHueDegrees) % static_cast<int>(kHueDegrees);
        values[1] = qRound(_saturation * kPercent);
        values[2] = qRound(_value * kPercent);
    }
    for (int index = 0; index < kSpinBoxCount; ++index)
    {
        // 程序设置的数值不再触发 onSpinBoxValueChanged
        const QSignalBlocker blocker(_spinBoxes[index]);
        _spinBoxes[index]->setValue(values[index]);
    }
}

void ShadColorPickerPrivate::updateSpinBoxRanges()
{
    const bool isRgb = _mode == ShadColorPickerType::Rgb;
    const QString rgbLabels[kSpinBoxCount]{QStringLiteral("R"), QStringLiteral("G"), QStringLiteral("B")};
    const QString hsvLabels[kSpinBoxCount]{QStringLiteral("H"), QStringLiteral("S"), QStringLiteral("V")};
    for (int index = 0; index < kSpinBoxCount; ++index)
    {
        const QSignalBlocker blocker(_spinBoxes[index]);
        _spinBoxLabels[index]->setText(isRgb ? rgbLabels[index] : hsvLabels[index]);
        const int maximum = isRgb ? kRgbMaximum : (index == 0 ? kHueMaximum : kPercentMaximum);
        _spinBoxes[index]->setRange(0, maximum);
        _spinBoxes[index]->setSuffix((!isRgb && index > 0) ? QStringLiteral("%") : QString());
    }
    updateSpinBoxes();
}

void ShadColorPickerPrivate::onHexEdited(const QString& text)
{
    // 输入完整的 #RRGGBB 后立即生效；不完整时等编辑结束再处理
    const QString name = normalizedHex(text);
    if (name.size() == kHexLength && QColor::isValidColorName(name))
    {
        applyColor(QColor::fromString(name), Source::Hex);
    }
}

void ShadColorPickerPrivate::onHexEditingFinished()
{
    // #RGB 简写展开为 #RRGGBB；无法识别时恢复为当前颜色。来源为 None，输入框按当前颜色重新显示
    const QString name = normalizedHex(_hexInput->text());
    if ((name.size() == kShortHexLength || name.size() == kHexLength) && QColor::isValidColorName(name))
    {
        applyColor(QColor::fromString(name), Source::None);
        return;
    }
    updateHexInput();
}

void ShadColorPickerPrivate::onSpinBoxValueChanged()
{
    const int first = _spinBoxes[0]->value();
    const int second = _spinBoxes[1]->value();
    const int third = _spinBoxes[2]->value();
    if (_mode == ShadColorPickerType::Rgb)
    {
        applyColor(QColor(first, second, third), Source::SpinBox);
        return;
    }
    applyHsv(first / kHueDegrees, second / kPercent, third / kPercent, Source::SpinBox);
}

void ShadColorPickerPrivate::addCustomColor()
{
    Q_Q(ShadColorPicker);
    // 写入选中的槽；没有选中时写入第一个空槽
    int index = _customSwatches->getSelectedIndex();
    for (int slot = 0; index < 0 && slot < _customSwatches->count(); ++slot)
    {
        if (!_customSwatches->colorAt(slot).isValid())
        {
            index = slot;
        }
    }
    if (index >= 0)
    {
        _customSwatches->setColorAt(index, currentColor());
    }
    else
    {
        // 槽已满且没有选中：去掉最早的颜色，其余前移，当前颜色放在最后
        QList<QColor> colors = _customSwatches->getColors();
        if (!colors.isEmpty())
        {
            colors.removeFirst();
        }
        colors.append(currentColor());
        _customSwatches->setColors(colors);
    }
    // 写入后取消选中，再次点击 + 时写入下一个空槽
    _customSwatches->setSelectedIndex(-1);
    updateCustomButtons();
    Q_EMIT q->customColorsChanged();
}

void ShadColorPickerPrivate::removeCustomColor()
{
    Q_Q(ShadColorPicker);
    const int index = _customSwatches->getSelectedIndex();
    if (index < 0 || !_customSwatches->colorAt(index).isValid())
    {
        return;
    }
    _customSwatches->setColorAt(index, QColor());
    updateCustomButtons();
    Q_EMIT q->customColorsChanged();
}

void ShadColorPickerPrivate::updateCustomButtons()
{
    const int index = _customSwatches->getSelectedIndex();
    const bool canRemove = index >= 0 && _customSwatches->colorAt(index).isValid();
    // 禁用有焦点的按钮前先把焦点交给另一个按钮
    ShadStyleHelper::setNavigationButtonsEnabled(_addCustomButton, true, _removeCustomButton, canRemove);
}
