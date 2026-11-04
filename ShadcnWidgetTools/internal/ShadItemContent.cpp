#include "ShadItemContent.h"

#include <QChar>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QTextLayout>
#include <QTextLine>
#include <QTextOption>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // text-sm / text-xs
    constexpr int kTextPixelSize = 14;
    constexpr int kExtraSmallTextPixelSize = 12;
    // leading-snug / leading-normal
    constexpr qreal kTitleLineHeightRatio = 1.375;
    constexpr qreal kDescriptionLineHeightRatio = 1.5;
    // gap-1
    constexpr int kGap = 4;
    // line-clamp-2
    constexpr int kMaxDescriptionLines = 2;

    // 换行符按行分隔符处理，QTextLayout 只在行分隔符处强制换行
    QString layoutText(const QString& text)
    {
        QString result = text;
        result.replace(QLatin1Char('\n'), QChar::LineSeparator);
        return result;
    }
} // namespace

ShadItemContent::ShadItemContent(QWidget* parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadItemContent::~ShadItemContent()
{
}

void ShadItemContent::setTitle(const QString& title)
{
    if (_title == title)
    {
        return;
    }
    _title = title;
    updateGeometry();
    update();
}

QString ShadItemContent::getTitle() const
{
    return _title;
}

void ShadItemContent::setDescription(const QString& description)
{
    if (_description == description)
    {
        return;
    }
    _description = description;
    updateGeometry();
    update();
}

QString ShadItemContent::getDescription() const
{
    return _description;
}

void ShadItemContent::setItemSize(ShadItemType::Size size)
{
    if (_size == size)
    {
        return;
    }
    _size = size;
    updateGeometry();
    update();
}

bool ShadItemContent::hasHeightForWidth() const
{
    return true;
}

int ShadItemContent::heightForWidth(int width) const
{
    return contentHeight(width);
}

QSize ShadItemContent::sizeHint() const
{
    QString singleLine = _description;
    singleLine.replace(QLatin1Char('\n'), QLatin1Char(' '));
    const int width = qMax(ShadStyleHelper::textWidth(titleFont(), _title), ShadStyleHelper::textWidth(descriptionFont(), singleLine));
    return QSize(width, contentHeight(width));
}

QSize ShadItemContent::minimumSizeHint() const
{
    // 宽度可以压缩，文字按宽度省略
    return QSize(0, contentHeight(sizeHint().width()));
}

void ShadItemContent::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange)
    {
        updateGeometry();
        update();
    }
}

void ShadItemContent::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const int width = this->width();
    const QStringList lines = descriptionLines(width);
    const int totalHeight = contentHeight(width);
    // 分配的高度大于内容时垂直居中
    qreal y = qMax(0, (height() - totalHeight) / 2);
    const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter);
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (!_title.isEmpty())
    {
        const QFont font = titleFont();
        painter.setFont(font);
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(QRectF(0, y, width, titleLineHeight()), alignment, ShadStyleHelper::elideText(font, _title, width));
        y += titleLineHeight();
        if (!lines.isEmpty())
        {
            y += gap();
        }
    }
    if (lines.isEmpty())
    {
        return;
    }
    painter.setFont(descriptionFont());
    painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    for (const QString& line : lines)
    {
        painter.drawText(QRectF(0, y, width, descriptionLineHeight()), alignment, line);
        y += descriptionLineHeight();
    }
}

QFont ShadItemContent::titleFont() const
{
    QFont result = font();
    result.setPixelSize(kTextPixelSize);
    result.setWeight(QFont::Medium);
    return result;
}

QFont ShadItemContent::descriptionFont() const
{
    QFont result = font();
    result.setPixelSize(_size == ShadItemType::ExtraSmall ? kExtraSmallTextPixelSize : kTextPixelSize);
    result.setWeight(QFont::Normal);
    return result;
}

int ShadItemContent::titleLineHeight() const
{
    return qRound(kTextPixelSize * kTitleLineHeightRatio);
}

int ShadItemContent::descriptionLineHeight() const
{
    return qRound(descriptionFont().pixelSize() * kDescriptionLineHeightRatio);
}

int ShadItemContent::gap() const
{
    return _size == ShadItemType::ExtraSmall ? 0 : kGap;
}

QStringList ShadItemContent::descriptionLines(int width) const
{
    QStringList lines;
    if (_description.isEmpty() || width <= 0)
    {
        return lines;
    }
    const QFont font = descriptionFont();
    const QString text = layoutText(_description);
    QTextLayout textLayout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    textLayout.setTextOption(option);
    textLayout.beginLayout();
    while (true)
    {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
        {
            break;
        }
        line.setLineWidth(width);
        if (lines.size() == kMaxDescriptionLines - 1)
        {
            // 最后一行放入剩余的全部文字，放不下时尾部省略
            QString rest = text.mid(line.textStart());
            rest.replace(QChar::LineSeparator, QLatin1Char(' '));
            lines.append(ShadStyleHelper::elideText(font, rest.trimmed(), width));
            break;
        }
        QString lineText = text.mid(line.textStart(), line.textLength());
        lineText.remove(QChar::LineSeparator);
        lines.append(lineText.trimmed());
    }
    textLayout.endLayout();
    return lines;
}

int ShadItemContent::contentHeight(int width) const
{
    int height = 0;
    if (!_title.isEmpty())
    {
        height += titleLineHeight();
    }
    const int lineCount = descriptionLines(width).size();
    if (lineCount > 0)
    {
        if (!_title.isEmpty())
        {
            height += gap();
        }
        height += lineCount * descriptionLineHeight();
    }
    return height;
}
