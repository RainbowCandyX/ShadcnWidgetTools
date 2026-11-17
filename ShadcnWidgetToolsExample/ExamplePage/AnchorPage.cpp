#include "AnchorPage.h"

#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadAnchor.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadScrollArea.h"

namespace
{
    constexpr int kAnchorWidth = 180;
    constexpr int kDocumentHeight = 320;
    constexpr int kHeadingFontSize = 16;
    constexpr int kSubheadingFontSize = 14;
    constexpr int kParagraphFontSize = 14;
    constexpr int kParagraphCount = 3;
    constexpr int kSectionSpacing = 12;
    constexpr int kPageOffset = 16;

    struct Heading
    {
        ExampleI18n::TextFunction title;
        int level{0};
    };

    ShadLabel* createLabel(int pixelSize, QFont::Weight weight, ShadThemeType::ThemeColor colorRole, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setWordWrap(true);
        label->setColorRole(colorRole);
        QFont font = label->font();
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        label->setFont(font);
        return label;
    }
} // namespace

AnchorPage::AnchorPage(QWidget* parent)
    : ExampleBasePage([] { return tr("锚点"); }, [] { return tr("ShadAnchor：页面内的目录导航，点击平滑滚动到对应位置，滚动时自动高亮当前链接。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("addLink 添加链接，level 设置缩进；在右侧滚动区域中滚动，左侧自动切换当前链接"); });
    const QList<Heading> headings = {
        {[] { return tr("安装"); }, 0},
        {[] { return tr("使用 FetchContent"); }, 1},
        {[] { return tr("使用 find_package"); }, 1},
        {[] { return tr("主题"); }, 0},
        {[] { return tr("国际化"); }, 0},
        {[] { return tr("常见问题"); }, 0},
    };
    QWidget* basicRow = new QWidget(this);
    QHBoxLayout* basicLayout = new QHBoxLayout(basicRow);
    basicLayout->setContentsMargins(0, 0, 0, 0);
    basicLayout->setSpacing(16);
    ShadAnchor* documentAnchor = new ShadAnchor(basicRow);
    documentAnchor->setFixedWidth(kAnchorWidth);
    ShadScrollArea* documentArea = new ShadScrollArea(basicRow);
    documentArea->setFixedHeight(kDocumentHeight);
    QWidget* document = new QWidget();
    QVBoxLayout* documentLayout = new QVBoxLayout(document);
    documentLayout->setContentsMargins(0, 0, 16, 0);
    documentLayout->setSpacing(kSectionSpacing);
    for (const Heading& heading : headings)
    {
        ShadLabel* headingLabel = createLabel(heading.level == 0 ? kHeadingFontSize : kSubheadingFontSize, QFont::DemiBold, ShadThemeType::Foreground, document);
        documentLayout->addWidget(headingLabel);
        const int linkIndex = documentAnchor->addLink(QString(), headingLabel, heading.level);
        const ExampleI18n::TextFunction title = heading.title;
        ExampleI18n::bind(headingLabel, [headingLabel, documentAnchor, linkIndex, title]()
        {
            headingLabel->setText(title());
            documentAnchor->setLinkTitle(linkIndex, title());
        });
        for (int paragraph = 0; paragraph < kParagraphCount; ++paragraph)
        {
            ShadLabel* paragraphLabel = createLabel(kParagraphFontSize, QFont::Normal, ShadThemeType::MutedForeground, document);
            ExampleI18n::bind(paragraphLabel, [paragraphLabel]() { paragraphLabel->setText(tr("这是一段占位文字，用于撑开内容的高度，便于观察滚动时当前链接的变化。")); });
            documentLayout->addWidget(paragraphLabel);
        }
    }
    documentLayout->addStretch(1);
    documentArea->setWidget(document);
    documentAnchor->setScrollArea(documentArea);
    basicLayout->addWidget(documentAnchor, 0, Qt::AlignTop);
    basicLayout->addWidget(documentArea, 1);
    addStretchItem(basicCard, basicRow);

    ShadLabel* currentLabel = new ShadLabel(this);
    currentLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateCurrentLabel = [currentLabel, documentAnchor]()
    {
        const int index = documentAnchor->getCurrentIndex();
        currentLabel->setText(index < 0 ? tr("当前链接: 无") : tr("当前链接: %1").arg(documentAnchor->getLinkTitle(index)));
    };
    connect(documentAnchor, &ShadAnchor::currentIndexChanged, currentLabel, updateCurrentLabel);
    ExampleI18n::bind(currentLabel, updateCurrentLabel);
    addItem(basicCard, currentLabel);

    ShadCard* pageCard = addSection([] { return tr("页面导航"); }, [] { return tr("setScrollArea 绑定整个示例页，setOffset(16) 让区块顶部与页面顶部保留 16px"); });
    ShadAnchor* pageAnchor = new ShadAnchor(this);
    pageAnchor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pageAnchor->setOffset(kPageOffset);
    addItem(pageCard, pageAnchor);

    ShadCard* offsetCard = addSection([] { return tr("滚动偏移"); }, [] { return tr("offset 同时决定判断当前链接的位置：区块顶部越过可视区顶部加 offset 的位置即视为进入该区块"); });
    ShadLabel* offsetLabel = createLabel(kParagraphFontSize, QFont::Normal, ShadThemeType::MutedForeground, this);
    ExampleI18n::bind(offsetLabel, [offsetLabel]() { offsetLabel->setText(tr("滚动到底部后，最后几个区块无法再到达顶部，此时切换到最后一个可见的链接。")); });
    addStretchItem(offsetCard, offsetLabel);

    const int basicLink = pageAnchor->addLink(QString(), basicCard);
    const int pageLink = pageAnchor->addLink(QString(), pageCard);
    const int offsetLink = pageAnchor->addLink(QString(), offsetCard);
    ExampleI18n::bind(pageAnchor, [pageAnchor, basicLink, pageLink, offsetLink]()
    {
        pageAnchor->setLinkTitle(basicLink, tr("基础用法"));
        pageAnchor->setLinkTitle(pageLink, tr("页面导航"));
        pageAnchor->setLinkTitle(offsetLink, tr("滚动偏移"));
    });
    pageAnchor->setScrollArea(this);
}

AnchorPage::~AnchorPage()
{
}
