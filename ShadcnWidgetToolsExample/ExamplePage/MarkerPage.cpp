#include "MarkerPage.h"

#include <QString>
#include <QUrl>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadMarker.h"
#include "ShadSpinner.h"

namespace
{
    constexpr int kStatusIconSize = 16;

    ShadMarker* createMarker(QWidget* parent, ShadIconType::IconName icon, const ExampleI18n::TextFunction& text, ShadMarkerType::Variant variant = ShadMarkerType::Default)
    {
        ShadMarker* marker = new ShadMarker(parent);
        marker->setIcon(icon);
        marker->setVariant(variant);
        ExampleI18n::bind(marker, [marker, text]() { marker->setText(text()); });
        return marker;
    }

    ShadMarker* createStatusMarker(QWidget* parent, const ExampleI18n::TextFunction& text)
    {
        ShadMarker* marker = createMarker(parent, ShadIconType::None, text);
        ShadSpinner* spinner = new ShadSpinner();
        spinner->setSpinnerSize(kStatusIconSize);
        spinner->setColorRole(ShadThemeType::MutedForeground);
        marker->setIconWidget(spinner);
        return marker;
    }
}

MarkerPage::MarkerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("标记"); }, [] { return tr("ShadMarker：对话中的行内标记，用于状态、提示、带边框的行和带文字的分隔线。"); }, parent)
{
    ShadCard* variantCard = addSection([] { return tr("变体"); }, [] { return tr("Default 为行内提示，Separator 为居中文字加两侧分隔线，Border 在行下方加边框"); });
    addStretchItem(variantCard, createMarker(this, ShadIconType::None, [] { return tr("对话已开始"); }));
    addStretchItem(variantCard, createMarker(this, ShadIconType::None, [] { return tr("今天"); }, ShadMarkerType::Separator));
    addStretchItem(variantCard, createMarker(this, ShadIconType::None, [] { return tr("已切换到 release-candidate 分支"); }, ShadMarkerType::Border));

    ShadCard* statusCard = addSection([] { return tr("状态"); }, [] { return tr("图标位置放入 ShadSpinner；文字变化时会通知辅助功能"); });
    addStretchItem(statusCard, createStatusMarker(this, [] { return tr("正在压缩对话"); }));
    addStretchItem(statusCard, createStatusMarker(this, [] { return tr("正在运行测试"); }));

    ShadCard* shimmerCard = addSection([] { return tr("扫光"); }, [] { return tr("setShimmer(true)：文字带扫光动画，表示正在进行"); });
    ShadMarker* thinkingMarker = createMarker(this, ShadIconType::Sparkles, [] { return tr("思考中..."); });
    thinkingMarker->setShimmer(true);
    ShadMarker* readingMarker = createMarker(this, ShadIconType::FileSearch, [] { return tr("正在读取 4 个文件"); });
    readingMarker->setShimmer(true);
    addStretchItem(shimmerCard, thinkingMarker);
    addStretchItem(shimmerCard, readingMarker);

    ShadCard* separatorCard = addSection([] { return tr("分隔线"); }, [] { return tr("Separator 变体：文字居中，两侧为分隔线"); });
    addStretchItem(separatorCard, createMarker(this, ShadIconType::None, [] { return tr("今天"); }, ShadMarkerType::Separator));
    addStretchItem(separatorCard, createMarker(this, ShadIconType::None, [] { return tr("用时 42 秒"); }, ShadMarkerType::Separator));
    addStretchItem(separatorCard, createMarker(this, ShadIconType::None, [] { return tr("对话已压缩"); }, ShadMarkerType::Separator));

    ShadCard* borderCard = addSection([] { return tr("带边框"); }, [] { return tr("Border 变体：在行下方加一条边框"); });
    addStretchItem(borderCard, createMarker(this, ShadIconType::None, [] { return tr("已切换到 release-candidate 分支"); }, ShadMarkerType::Border));
    addStretchItem(borderCard, createMarker(this, ShadIconType::None, [] { return tr("已查看 8 个相关文件"); }, ShadMarkerType::Border));
    addStretchItem(borderCard, createMarker(this, ShadIconType::None, [] { return tr("已打开实现说明"); }, ShadMarkerType::Border));

    ShadCard* iconCard = addSection([] { return tr("带图标"); }, [] { return tr("图标位于文字起始侧，文字较长时自动换行"); });
    addStretchItem(iconCard, createMarker(this, ShadIconType::GitBranch, [] { return tr("已切换到新分支"); }));
    addStretchItem(iconCard, createMarker(this, ShadIconType::FolderSearch, [] { return tr("已查看 4 个文件"); }));
    addStretchItem(iconCard, createMarker(this, ShadIconType::Check, [] { return tr("同步完成"); }));
    addStretchItem(iconCard, createMarker(this, ShadIconType::Info, [] { return tr("已将 12 个文件中的旧接口替换为新接口，并更新了相关的单元测试与文档说明，所有检查均已通过，可以合并到主分支。"); }));

    ShadCard* actionCard = addSection([] { return tr("链接与按钮"); }, [] { return tr("setUrl 显示为链接，点击后用浏览器打开；setClickable 后整行可点击并发出 clicked 信号"); });
    ShadMarker* linkMarker = createMarker(this, ShadIconType::ExternalLink, [] { return tr("查看拉取请求"); });
    linkMarker->setUrl(QUrl(QStringLiteral("https://github.com/RainbowCandyX/ShadcnWidgetTools/pulls")));
    ShadMarker* revertMarker = createMarker(this, ShadIconType::RotateCcw, [] { return tr("撤销这次修改"); });
    revertMarker->setClickable(true);
    ShadLabel* clickLabel = new ShadLabel(this);
    clickLabel->setColorRole(ShadThemeType::MutedForeground);
    const std::shared_ptr<int> clickCount = std::make_shared<int>(0);
    const auto updateClickLabel = [clickLabel, clickCount]() { clickLabel->setText(tr("点击次数: %1").arg(*clickCount)); };
    ExampleI18n::bind(clickLabel, updateClickLabel);
    connect(revertMarker, &ShadMarker::clicked, clickLabel, [clickCount, updateClickLabel]()
    {
        ++*clickCount;
        updateClickLabel();
    });
    addStretchItem(actionCard, linkMarker);
    addStretchItem(actionCard, revertMarker);
    addItem(actionCard, clickLabel);
}

MarkerPage::~MarkerPage()
{
}
