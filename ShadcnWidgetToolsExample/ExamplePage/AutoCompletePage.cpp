#include "AutoCompletePage.h"

#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadAutoComplete.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"

namespace
{
    constexpr int kRemoteDelayMs = 400;

    ShadAutoComplete* createAutoComplete(ExampleI18n::TextFunction placeholder, QWidget* parent)
    {
        ShadAutoComplete* autoComplete = new ShadAutoComplete(parent);
        autoComplete->setFixedWidth(ExampleLayout::kFieldWidth);
        autoComplete->setTrailingAction(ShadInputType::ClearAction);
        ExampleI18n::bind(autoComplete, [autoComplete, placeholder]() { autoComplete->setPlaceholderText(placeholder()); });
        return autoComplete;
    }
} // namespace

AutoCompletePage::AutoCompletePage(QWidget* parent)
    : ExampleBasePage([] { return tr("自动补全"); }, [] { return tr("ShadAutoComplete：输入时在下方弹出建议，方向键切换，回车或点击选中。"); }, parent)
{
    const QStringList frameworks = {QStringLiteral("Next.js"), QStringLiteral("SvelteKit"), QStringLiteral("Nuxt.js"), QStringLiteral("Remix"), QStringLiteral("Astro"), QStringLiteral("Angular"), QStringLiteral("Vue.js"), QStringLiteral("React Router"), QStringLiteral("Solid Start"), QStringLiteral("Qwik City"), QStringLiteral("Gatsby"), QStringLiteral("Ember.js")};

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("setOptions 设置选项，默认按模糊匹配过滤并按相关度排序"); });
    ShadAutoComplete* frameworkInput = createAutoComplete([] { return tr("输入框架名称"); }, this);
    frameworkInput->setLeadingIcon(ShadIconType::Search);
    frameworkInput->setOptions(frameworks);
    ShadLabel* selectedLabel = new ShadLabel(this);
    selectedLabel->setColorRole(ShadThemeType::MutedForeground);
    ExampleI18n::bind(selectedLabel, [selectedLabel]() { selectedLabel->setText(tr("选中的建议: 无")); });
    connect(frameworkInput, &ShadAutoComplete::optionSelected, selectedLabel, [selectedLabel](const QString& text) { selectedLabel->setText(tr("选中的建议: %1").arg(text)); });
    addItem(basicCard, frameworkInput);
    addItem(basicCard, selectedLabel);

    ShadCard* prefixCard = addSection([] { return tr("前缀匹配"); }, [] { return tr("setFilterMode(Prefix)：只显示以输入文字开头的选项，忽略大小写"); });
    ShadAutoComplete* commandInput = createAutoComplete([] { return tr("输入 git 命令"); }, this);
    commandInput->setFilterMode(ShadAutoCompleteType::Prefix);
    commandInput->setOptions({QStringLiteral("git add"), QStringLiteral("git branch"), QStringLiteral("git checkout"), QStringLiteral("git clone"), QStringLiteral("git commit"), QStringLiteral("git diff"), QStringLiteral("git fetch"), QStringLiteral("git log"), QStringLiteral("git merge"), QStringLiteral("git pull"), QStringLiteral("git push"), QStringLiteral("git rebase"), QStringLiteral("git status")});
    addItem(prefixCard, commandInput);

    ShadCard* emailCard = addSection([] { return tr("动态选项"); }, [] { return tr("setFilterMode(NoFilter)：在 searchTextChanged 中按输入生成选项，例如补全邮箱域名"); });
    ShadAutoComplete* emailInput = createAutoComplete([] { return tr("输入邮箱"); }, this);
    emailInput->setFilterMode(ShadAutoCompleteType::NoFilter);
    emailInput->setLeadingIcon(ShadIconType::Mail);
    connect(emailInput, &ShadAutoComplete::searchTextChanged, emailInput, [emailInput](const QString& text)
    {
        const QStringList domains = {QStringLiteral("gmail.com"), QStringLiteral("outlook.com"), QStringLiteral("qq.com"), QStringLiteral("163.com")};
        const qsizetype atIndex = text.indexOf(u'@');
        const QString name = atIndex >= 0 ? text.left(atIndex) : text;
        const QString domainPrefix = atIndex >= 0 ? text.mid(atIndex + 1) : QString();
        QStringList options;
        if (!name.isEmpty())
        {
            for (const QString& domain : domains)
            {
                if (domain.startsWith(domainPrefix, Qt::CaseInsensitive))
                {
                    options.append(name + u'@' + domain);
                }
            }
        }
        emailInput->setOptions(options);
    });
    addItem(emailCard, emailInput);

    ShadCard* remoteCard = addSection([] { return tr("远程搜索"); }, [] { return tr("输入后延迟 400ms 返回结果，模拟网络请求；结果返回时建议列表自动刷新"); });
    ShadAutoComplete* cityInput = createAutoComplete([] { return tr("搜索城市"); }, this);
    cityInput->setFilterMode(ShadAutoCompleteType::NoFilter);
    cityInput->setLeadingIcon(ShadIconType::MapPin);
    QTimer* remoteTimer = new QTimer(cityInput);
    remoteTimer->setSingleShot(true);
    remoteTimer->setInterval(kRemoteDelayMs);
    connect(remoteTimer, &QTimer::timeout, cityInput, [cityInput]()
    {
        const QStringList cities = {QStringLiteral("Beijing"), QStringLiteral("Shanghai"), QStringLiteral("Shenzhen"), QStringLiteral("Guangzhou"), QStringLiteral("Hangzhou"), QStringLiteral("Chengdu"), QStringLiteral("Hong Kong"), QStringLiteral("Taipei"), QStringLiteral("Tokyo"), QStringLiteral("Osaka"), QStringLiteral("Seoul"), QStringLiteral("Singapore"), QStringLiteral("Sydney"), QStringLiteral("San Francisco"), QStringLiteral("Seattle")};
        const QString query = cityInput->text().trimmed();
        QStringList options;
        for (const QString& city : cities)
        {
            if (!query.isEmpty() && city.contains(query, Qt::CaseInsensitive))
            {
                options.append(city);
            }
        }
        cityInput->setOptions(options);
    });
    connect(cityInput, &ShadAutoComplete::searchTextChanged, remoteTimer, [remoteTimer]() { remoteTimer->start(); });
    addItem(remoteCard, cityInput);

    ShadCard* invalidCard = addSection([] { return tr("错误态"); }, [] { return tr("继承 ShadInput，setInvalid(true) 等接口同样可用"); });
    ShadAutoComplete* invalidInput = createAutoComplete([] { return tr("输入框架名称"); }, this);
    invalidInput->setOptions(frameworks);
    invalidInput->setInvalid(true);
    addItem(invalidCard, invalidInput);
}

AutoCompletePage::~AutoCompletePage()
{
}
