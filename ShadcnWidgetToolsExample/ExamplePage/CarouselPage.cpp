#include "CarouselPage.h"

#include <QChar>
#include <QFont>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadCarousel.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadTheme.h"

namespace
{
    // 幻灯片内容的宽度：基础用法 max-w-xs，尺寸与循环示例 max-w-sm
    constexpr int kNarrowContentWidth = 320;
    constexpr int kWideContentWidth = 384;
    // 纵向示例的视口高度 h-[200px]，幻灯片间距 pt-1
    constexpr int kVerticalViewportHeight = 200;
    constexpr int kVerticalSpacing = 4;
    // 默认间距 pl-4
    constexpr int kDefaultSpacing = 16;
    constexpr int kSlideCount = 5;
    // 幻灯片外层的 p-1
    constexpr int kSlidePadding = 4;
    // 数字字号：text-4xl / text-3xl / text-2xl
    constexpr int kLargeTextSize = 36;
    constexpr int kMediumTextSize = 30;
    constexpr int kSmallTextSize = 24;

    // 示例用的幻灯片：p-1 内放一张卡片（bg-card rounded-xl border），卡片中间是一个数字
    // isSquare 为 true 时按宽度保持正方形（aspect-square），对应横向示例；纵向示例的高度由轮播决定
    class NumberSlide : public QWidget
    {
        public:
            NumberSlide(const QString& text, const QSize& hint, int textPixelSize, bool isSquare, QWidget* parent = nullptr)
                : QWidget(parent), _text(text), _hint(hint), _textPixelSize(textPixelSize), _isSquare(isSquare)
            {
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

            QSize sizeHint() const override
            {
                return _hint;
            }

            bool hasHeightForWidth() const override
            {
                return _isSquare;
            }

            int heightForWidth(int width) const override
            {
                return _isSquare ? width : QWidget::heightForWidth(width);
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge);
                // 边框画在卡片内侧，与 CSS border 一致
                const QRectF cardRect = QRectF(rect()).adjusted(kSlidePadding + 0.5, kSlidePadding + 0.5, -kSlidePadding - 0.5, -kSlidePadding - 0.5);
                QPainter painter(this);
                painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
                painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
                painter.setBrush(shadTheme->getColor(ShadThemeType::Card));
                painter.drawRoundedRect(cardRect, radius, radius);
                QFont textFont = font();
                textFont.setPixelSize(_textPixelSize);
                textFont.setWeight(QFont::DemiBold);
                painter.setFont(textFont);
                painter.setPen(shadTheme->getColor(ShadThemeType::CardForeground));
                painter.drawText(cardRect, Qt::AlignCenter, _text);
            }

        private:
            QString _text;
            QSize _hint;
            int _textPixelSize;
            bool _isSquare;
    };

    // 一屏 itemsPerView 张时每张幻灯片的宽度：(内容宽度 + 间距) / 张数 - 间距
    int slideExtent(int contentWidth, int itemsPerView, int spacing)
    {
        return (contentWidth + spacing) / itemsPerView - spacing;
    }

    // 横向轮播：内容宽度为 contentWidth，放入 5 张正方形数字幻灯片
    ShadCarousel* createHorizontalCarousel(int contentWidth, int itemsPerView, int spacing, int textPixelSize, QWidget* parent)
    {
        ShadCarousel* carousel = new ShadCarousel(parent);
        carousel->setItemsPerView(itemsPerView);
        carousel->setSpacing(spacing);
        const int extent = slideExtent(contentWidth, itemsPerView, spacing);
        for (int index = 0; index < kSlideCount; ++index)
        {
            carousel->addItem(new NumberSlide(QString::number(index + 1), QSize(extent, extent), textPixelSize, true, carousel));
        }
        return carousel;
    }

    ShadLabel* createMutedText(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

CarouselPage::CarouselPage(QWidget* parent)
    : ExampleBasePage([] { return tr("轮播"); }, [] { return tr("ShadCarousel：可拖动的幻灯片轮播，支持一屏多张、纵向、循环与自动播放。"); }, parent)
{
    // 基础用法：一屏一张，两侧为上一张 / 下一张按钮
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击两侧按钮或拖动切换幻灯片"); });
    // 横向轮播的高度随宽度变化（heightForWidth）；addItem 的左对齐会让布局按整行宽度预留高度，轮播被拉高，
    // 这里用 addRow（流式布局）按尺寸提示摆放，宽度与高度一致
    addRow(basicCard, {createHorizontalCarousel(kNarrowContentWidth, 1, kDefaultSpacing, kLargeTextSize, this)});

    // 尺寸：一屏 3 张
    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("setItemsPerView(3)：一屏同时显示 3 张幻灯片"); });
    ShadCarousel* sizeCarousel = createHorizontalCarousel(kWideContentWidth, 3, kDefaultSpacing, kMediumTextSize, this);
    sizeCarousel->setAlign(ShadCarouselType::Start);
    addRow(sizeCard, {sizeCarousel});

    // 间距：8 与 16 对比
    ShadCard* spacingCard = addSection([] { return tr("间距"); }, [] { return tr("setSpacing：设置幻灯片之间的距离"); });
    for (int spacing : {8, kDefaultSpacing})
    {
        ShadCarousel* carousel = createHorizontalCarousel(kWideContentWidth, 3, spacing, kSmallTextSize, this);
        carousel->setAlign(ShadCarouselType::Start);
        addRow(spacingCard, {carousel});
    }

    // 方向：纵向，视口高 200，一屏 2 张
    ShadCard* orientationCard = addSection([] { return tr("方向"); }, []
    {
        return tr("setOrientation(Qt::Vertical)：幻灯片纵向排列，按钮在上下两端");
    });
    ShadCarousel* verticalCarousel = new ShadCarousel(this);
    verticalCarousel->setOrientation(Qt::Vertical);
    verticalCarousel->setItemsPerView(2);
    verticalCarousel->setSpacing(kVerticalSpacing);
    verticalCarousel->setAlign(ShadCarouselType::Start);
    const int verticalSlideHeight = slideExtent(kVerticalViewportHeight, 2, kVerticalSpacing);
    for (int index = 0; index < kSlideCount; ++index)
    {
        verticalCarousel->addItem(new NumberSlide(QString::number(index + 1), QSize(kNarrowContentWidth, verticalSlideHeight), kMediumTextSize, false, verticalCarousel));
    }
    addItem(orientationCard, verticalCarousel);

    // 循环：首尾无缝衔接
    ShadCard* loopCard = addSection([] { return tr("循环"); }, [] { return tr("setLoop(true)：首尾无缝衔接，按钮不会禁用"); });
    ShadCarousel* loopCarousel = createHorizontalCarousel(kWideContentWidth, 3, kDefaultSpacing, kMediumTextSize, this);
    loopCarousel->setAlign(ShadCarouselType::Start);
    loopCarousel->setLoop(true);
    addRow(loopCard, {loopCarousel});

    // API 与事件：显示当前停靠点
    ShadCard* apiCard = addSection([] { return tr("API 与事件"); }, [] { return tr("读取当前停靠点与数量，监听切换信号"); });
    QWidget* apiContainer = new QWidget(this);
    QVBoxLayout* apiLayout = new QVBoxLayout(apiContainer);
    apiLayout->setContentsMargins(0, 0, 0, 0);
    apiLayout->setSpacing(8);
    ShadCarousel* apiCarousel = createHorizontalCarousel(kNarrowContentWidth, 1, kDefaultSpacing, kLargeTextSize, apiContainer);
    // 对应 py-2 text-center text-sm text-muted-foreground
    ShadLabel* slideLabel = createMutedText(QString(), apiContainer);
    slideLabel->setAlignment(Qt::AlignCenter);
    const auto updateSlideLabel = [apiCarousel, slideLabel]()
    {
        slideLabel->setText(tr("第 %1 张，共 %2 张").arg(apiCarousel->currentIndex() + 1).arg(apiCarousel->snapCount()));
    };
    connect(apiCarousel, &ShadCarousel::currentChanged, slideLabel, updateSlideLabel);
    connect(apiCarousel, &ShadCarousel::snapCountChanged, slideLabel, updateSlideLabel);
    ExampleI18n::bind(slideLabel, updateSlideLabel);
    apiLayout->addWidget(apiCarousel);
    apiLayout->addWidget(slideLabel);
    addRow(apiCard, {apiContainer});

    // 自动播放
    ShadCard* autoplayCard = addSection([] { return tr("自动播放"); }, [] { return tr("setAutoplayInterval：定时切换，悬停时暂停"); });
    ShadCarousel* autoplayCarousel = createHorizontalCarousel(kNarrowContentWidth, 1, kDefaultSpacing, kLargeTextSize, this);
    autoplayCarousel->setAutoplayInterval(2000);
    addRow(autoplayCard, {autoplayCarousel});

    // RTL：第一张在右侧，拖动与方向键随之镜像
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左布局下幻灯片顺序与拖动方向镜像"); });
    ShadCarousel* rtlCarousel = new ShadCarousel(this);
    rtlCarousel->setLayoutDirection(Qt::RightToLeft);
    rtlCarousel->setItemsPerView(2);
    const int rtlExtent = slideExtent(kNarrowContentWidth, 2, kDefaultSpacing);
    for (int index = 0; index < kSlideCount; ++index)
    {
        // 阿拉伯-印度数字 ١ 到 ٥
        const QString text(QChar(0x0661 + index));
        rtlCarousel->addItem(new NumberSlide(text, QSize(rtlExtent, rtlExtent), kMediumTextSize, true, rtlCarousel));
    }
    addRow(rtlCard, {rtlCarousel});
}

CarouselPage::~CarouselPage()
{
}
