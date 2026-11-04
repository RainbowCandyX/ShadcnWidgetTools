#include "SeparatorPage.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadResizable.h"
#include "ShadSeparator.h"
#include "ShadTheme.h"

namespace
{
    // 演示面板组外框，对应 rounded-lg border
    class BorderedFrame : public QWidget
    {
        public:
            explicit BorderedFrame(QWidget* parent = nullptr)
                : QWidget(parent)
            {
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius - 0.5, radius - 0.5);
            }
    };

    // 演示面板：内容居中显示一段文字，文字随语言刷新
    QWidget* createPanel(const ExampleI18n::TextFunction& text, QWidget* parent)
    {
        QWidget* panel = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(24, 24, 24, 24);
        ShadLabel* label = new ShadLabel(panel);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        QFont labelFont = label->font();
        labelFont.setWeight(QFont::DemiBold);
        label->setFont(labelFont);
        layout->addWidget(label, 0, Qt::AlignCenter);
        return panel;
    }

    // 外框内放置面板组，外框 1px 边框不被面板覆盖
    QWidget* createFrame(ShadResizable* resizable, QWidget* parent)
    {
        BorderedFrame* frame = new BorderedFrame(parent);
        frame->setFixedSize(448, 200);
        QVBoxLayout* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(1, 1, 1, 1);
        layout->addWidget(resizable);
        return frame;
    }
} // namespace

SeparatorPage::SeparatorPage(QWidget* parent)
    : ExampleBasePage([] { return tr("分隔线"); }, [] { return tr("ShadSeparator / ShadResizable：静态分隔线，以及可拖动调整相邻面板大小的分隔线。"); }, parent)
{
    ShadCard* demoCard = addSection([] { return tr("静态分隔线"); }, [] { return tr("水平与垂直两种分隔线"); });

    QWidget* demoWidget = new QWidget(this);
    demoWidget->setFixedWidth(360);
    QVBoxLayout* demoLayout = new QVBoxLayout(demoWidget);
    demoLayout->setContentsMargins(0, 0, 0, 0);
    demoLayout->setSpacing(4);
    ShadLabel* titleLabel = new ShadLabel(demoWidget);
    ShadLabel* descriptionLabel = new ShadLabel(demoWidget);
    ExampleI18n::bind(demoWidget, [titleLabel, descriptionLabel]()
    {
        titleLabel->setText(tr("基础组件"));
        descriptionLabel->setText(tr("一个开源的 UI 组件库。"));
    });
    descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    QFont descriptionFont = descriptionLabel->font();
    descriptionFont.setWeight(QFont::Normal);
    descriptionLabel->setFont(descriptionFont);
    demoLayout->addWidget(titleLabel);
    demoLayout->addWidget(descriptionLabel);
    demoLayout->addSpacing(12);
    demoLayout->addWidget(new ShadSeparator(Qt::Horizontal, demoWidget));
    demoLayout->addSpacing(12);

    QHBoxLayout* linkLayout = new QHBoxLayout();
    linkLayout->setContentsMargins(0, 0, 0, 0);
    linkLayout->setSpacing(16);
    const QList<ExampleI18n::TextFunction> linkTexts{[] { return tr("博客"); }, [] { return tr("文档"); }, [] { return tr("源码"); }};
    for (int i = 0; i < linkTexts.size(); ++i)
    {
        if (i > 0)
        {
            ShadSeparator* verticalSeparator = new ShadSeparator(Qt::Vertical, demoWidget);
            verticalSeparator->setFixedHeight(20);
            linkLayout->addWidget(verticalSeparator);
        }
        ShadLabel* linkLabel = new ShadLabel(demoWidget);
        const ExampleI18n::TextFunction linkText = linkTexts.at(i);
        ExampleI18n::bind(linkLabel, [linkLabel, linkText]() { linkLabel->setText(linkText()); });
        QFont linkFont = linkLabel->font();
        linkFont.setWeight(QFont::Normal);
        linkLabel->setFont(linkFont);
        linkLayout->addWidget(linkLabel);
    }
    linkLayout->addStretch(1);
    demoLayout->addLayout(linkLayout);

    addItem(demoCard, demoWidget);

    ShadCard* resizableCard = addSection([] { return tr("可拖动分隔线"); }, [] { return tr("拖动分隔线调整面板大小，支持嵌套面板组"); });
    ShadResizable* rightGroup = new ShadResizable(Qt::Vertical);
    rightGroup->addWidget(createPanel([] { return tr("二"); }, rightGroup));
    rightGroup->addWidget(createPanel([] { return tr("三"); }, rightGroup));
    rightGroup->setSizes(QList<int>{60, 140});
    ShadResizable* nestedGroup = new ShadResizable(Qt::Horizontal);
    nestedGroup->addWidget(createPanel([] { return tr("一"); }, nestedGroup));
    nestedGroup->addWidget(rightGroup);
    nestedGroup->setSizes(QList<int>{223, 223});
    addItem(resizableCard, createFrame(nestedGroup, this));

    ShadCard* gripCard = addSection([] { return tr("带抓手"); }, [] { return tr("setGripVisible(true)：在分隔线中间显示抓手"); });
    ShadResizable* gripGroup = new ShadResizable(Qt::Horizontal);
    gripGroup->setGripVisible(true);
    gripGroup->addWidget(createPanel([] { return tr("侧边栏"); }, gripGroup));
    gripGroup->addWidget(createPanel([] { return tr("内容"); }, gripGroup));
    gripGroup->setSizes(QList<int>{150, 286});
    addItem(gripCard, createFrame(gripGroup, this));
}

SeparatorPage::~SeparatorPage()
{
}
