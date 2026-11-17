#include "ShadStepsPrivate.h"

#include <QFontMetricsF>
#include <QPointF>
#include <QSizeF>
#include <QtGlobal>
#include <QtMath>

#include <utility>

#include "ShadSteps.h"
#include "ShadStyleHelper.h"

ShadStepsPrivate::ShadStepsPrivate()
{
}

ShadStepsPrivate::~ShadStepsPrivate()
{
}

QFont ShadStepsPrivate::titleFont() const
{
    Q_Q(const ShadSteps);
    QFont font = q->font();
    font.setPixelSize(kTextFontSize);
    font.setWeight(QFont::Medium);
    return font;
}

QFont ShadStepsPrivate::descriptionFont() const
{
    Q_Q(const ShadSteps);
    QFont font = q->font();
    font.setPixelSize(kTextFontSize);
    font.setWeight(QFont::Normal);
    return font;
}

QFont ShadStepsPrivate::numberFont() const
{
    Q_Q(const ShadSteps);
    QFont font = q->font();
    font.setPixelSize(kNumberFontSize);
    font.setWeight(QFont::Medium);
    return font;
}

int ShadStepsPrivate::naturalTextWidth(int index) const
{
    const Step& step = _steps.at(index);
    return qMax(ShadStyleHelper::textWidth(titleFont(), step.title), ShadStyleHelper::textWidth(descriptionFont(), step.description));
}

int ShadStepsPrivate::wrappedHeight(const QFont& font, const QString& text, int width) const
{
    if (text.isEmpty())
    {
        return 0;
    }
    const QFontMetricsF metrics(font);
    const QRectF bounds = metrics.boundingRect(QRectF(0.0, 0.0, qMax(1, width), 1.0e6), Qt::TextWordWrap, text);
    return qMax(kLineHeight, qCeil(bounds.height()));
}

bool ShadStepsPrivate::hasDescription() const
{
    for (const Step& step : _steps)
    {
        if (!step.description.isEmpty())
        {
            return true;
        }
    }
    return false;
}

ShadStepsPrivate::LayoutResult ShadStepsPrivate::computeLayout(int width) const
{
    return _orientation == Qt::Horizontal ? computeHorizontalLayout(width) : computeVerticalLayout(width);
}

ShadStepsPrivate::LayoutResult ShadStepsPrivate::computeHorizontalLayout(int width) const
{
    LayoutResult result;
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int stepCount = int(_steps.size());
    const int contentHeight = qMax(kIndicatorSize, (kIndicatorSize - kLineHeight) / 2 + kLineHeight + (hasDescription() ? kTextGap + kLineHeight : 0));
    result.height = 2 * margin + contentHeight;
    if (stepCount == 0)
    {
        return result;
    }
    const int lineCount = stepCount - 1;
    int fixedWidth = lineCount * (kMinLineLength + 2 * kLineGap);
    QList<int> naturalWidths;
    naturalWidths.reserve(stepCount);
    int naturalSum = 0;
    for (int index = 0; index < stepCount; ++index)
    {
        const int natural = naturalTextWidth(index);
        naturalWidths.append(natural);
        naturalSum += natural;
        fixedWidth += kIndicatorSize + (natural > 0 ? kHorizontalTextGap : 0);
    }
    const int availableText = qMax(0, width - 2 * margin - fixedWidth);
    QList<int> textWidths = naturalWidths;
    if (naturalSum > availableText)
    {
        const int cap = availableText / stepCount;
        for (int& textWidth : textWidths)
        {
            textWidth = qMin(textWidth, cap);
        }
    }
    int usedText = 0;
    for (const int textWidth : std::as_const(textWidths))
    {
        usedText += textWidth;
    }
    const qreal lineLength = lineCount > 0 ? kMinLineLength + qMax(0, availableText - usedText) / qreal(lineCount) : 0.0;

    const qreal top = margin;
    const qreal centerY = top + kIndicatorSize / 2.0;
    const qreal titleTop = top + (kIndicatorSize - kLineHeight) / 2.0;
    const QFont titleTextFont = titleFont();
    const QFont descriptionTextFont = descriptionFont();
    qreal x = margin;
    for (int index = 0; index < stepCount; ++index)
    {
        const Step& step = _steps.at(index);
        StepLayout layout;
        const qreal stepLeft = x;
        layout.indicator = QRectF(x, top, kIndicatorSize, kIndicatorSize);
        x += kIndicatorSize;
        const int textWidth = textWidths.at(index);
        if (naturalWidths.at(index) > 0)
        {
            x += kHorizontalTextGap;
            layout.title = QRectF(x, titleTop, textWidth, kLineHeight);
            layout.titleText = ShadStyleHelper::elideText(titleTextFont, step.title, textWidth);
            if (!step.description.isEmpty())
            {
                layout.description = QRectF(x, titleTop + kLineHeight + kTextGap, textWidth, kLineHeight);
                layout.descriptionText = ShadStyleHelper::elideText(descriptionTextFont, step.description, textWidth);
            }
            x += textWidth;
        }
        layout.hit = QRectF(stepLeft, top, x - stepLeft, contentHeight);
        if (index < lineCount)
        {
            layout.line = QLineF(x + kLineGap, centerY, x + kLineGap + lineLength, centerY);
            x += 2 * kLineGap + lineLength;
        }
        result.steps.append(layout);
    }
    return result;
}

ShadStepsPrivate::LayoutResult ShadStepsPrivate::computeVerticalLayout(int width) const
{
    LayoutResult result;
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int stepCount = int(_steps.size());
    const qreal textLeft = margin + kIndicatorSize + kVerticalTextGap;
    const int textWidth = qMax(1, qFloor(width - margin - textLeft));
    const QFont titleTextFont = titleFont();
    const QFont descriptionTextFont = descriptionFont();
    qreal y = margin;
    for (int index = 0; index < stepCount; ++index)
    {
        const Step& step = _steps.at(index);
        StepLayout layout;
        const qreal stepTop = y;
        layout.indicator = QRectF(margin, stepTop, kIndicatorSize, kIndicatorSize);
        const qreal titleTop = stepTop + (kIndicatorSize - kLineHeight) / 2.0;
        const int titleHeight = wrappedHeight(titleTextFont, step.title, textWidth);
        layout.title = QRectF(textLeft, titleTop, textWidth, titleHeight);
        layout.titleText = step.title;
        qreal textBottom = titleTop + titleHeight;
        if (!step.description.isEmpty())
        {
            const qreal descriptionTop = titleHeight > 0 ? textBottom + kTextGap : titleTop;
            const int descriptionHeight = wrappedHeight(descriptionTextFont, step.description, textWidth);
            layout.description = QRectF(textLeft, descriptionTop, textWidth, descriptionHeight);
            layout.descriptionText = step.description;
            textBottom = descriptionTop + descriptionHeight;
        }
        const qreal stepBottom = qMax(stepTop + kIndicatorSize, textBottom);
        layout.hit = QRectF(margin, stepTop, width - 2 * margin, stepBottom - stepTop);
        if (index < stepCount - 1)
        {
            const qreal centerX = margin + kIndicatorSize / 2.0;
            const qreal nextTop = stepBottom + kVerticalStepGap;
            layout.line = QLineF(centerX, stepTop + kIndicatorSize + kVerticalLineGap, centerX, nextTop - kVerticalLineGap);
            y = nextTop;
        }
        else
        {
            y = stepBottom;
        }
        result.steps.append(layout);
    }
    result.height = qCeil(y) + margin;
    return result;
}

int ShadStepsPrivate::indexAt(const QPoint& pos) const
{
    Q_Q(const ShadSteps);
    const LayoutResult layout = computeLayout(q->width());
    const QRectF bounds(q->rect());
    for (int index = 0; index < layout.steps.size(); ++index)
    {
        if (ShadStyleHelper::visualRect(q->layoutDirection(), bounds, layout.steps.at(index).hit).contains(QPointF(pos)))
        {
            return index;
        }
    }
    return -1;
}

bool ShadStepsPrivate::isSelectable(int index) const
{
    return index >= 0 && index < _steps.size() && !_steps.at(index).isDisabled;
}

void ShadStepsPrivate::activate(int index)
{
    Q_Q(ShadSteps);
    if (!_isClickable || !isSelectable(index))
    {
        return;
    }
    _focusIndex = index;
    q->setCurrentIndex(index);
    q->update();
    Q_EMIT q->stepClicked(index);
}

void ShadStepsPrivate::moveFocus(int step)
{
    Q_Q(ShadSteps);
    int index = _focusIndex + step;
    while (index >= 0 && index < _steps.size())
    {
        if (isSelectable(index))
        {
            _focusIndex = index;
            q->update();
            return;
        }
        index += step;
    }
}

void ShadStepsPrivate::updateFocusPolicy()
{
    Q_Q(ShadSteps);
    q->setFocusPolicy(_isClickable ? Qt::StrongFocus : Qt::NoFocus);
    if (!_isClickable)
    {
        _hoverIndex = -1;
        q->unsetCursor();
    }
}
