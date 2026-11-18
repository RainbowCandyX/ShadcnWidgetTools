#include "ResultPage.h"

#include <QColor>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadResult.h"
#include "ShadSegmented.h"
#include "ShadSpinner.h"

namespace
{
    constexpr int kStatusCount = 7;
    constexpr int kSpinnerSize = 40;

    ShadButton* addResultButton(ShadResult* result, ShadButtonType::Variant variant, ExampleI18n::TextFunction text)
    {
        ShadButton* button = result->addButton(QString(), variant);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }

    ShadLabel* addDetailLabel(ShadResult* result, ExampleI18n::TextFunction text)
    {
        ShadLabel* label = new ShadLabel(result);
        label->setAlignment(Qt::AlignHCenter);
        label->setColorRole(ShadThemeType::Destructive);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        result->addContentWidget(label);
        return label;
    }

    QString statusTitle(int status)
    {
        switch (status)
        {
            case ShadResultType::Success:
                return ResultPage::tr("操作成功");
            case ShadResultType::Error:
                return ResultPage::tr("操作失败");
            case ShadResultType::Warning:
                return ResultPage::tr("导入完成，部分记录被跳过");
            case ShadResultType::Info:
                return ResultPage::tr("申请已提交");
            case ShadResultType::Forbidden:
                return ResultPage::tr("没有访问权限");
            case ShadResultType::NotFound:
                return ResultPage::tr("页面不存在");
            default:
                return ResultPage::tr("服务器出错");
        }
    }

    QString statusDescription(int status)
    {
        switch (status)
        {
            case ShadResultType::Success:
                return ResultPage::tr("更改已保存并立即生效");
            case ShadResultType::Error:
                return ResultPage::tr("请检查网络连接后重试");
            case ShadResultType::Warning:
                return ResultPage::tr("共 120 条，其中 3 条因格式不正确没有导入");
            case ShadResultType::Info:
                return ResultPage::tr("审核结果会通过邮件通知你");
            case ShadResultType::Forbidden:
                return ResultPage::tr("你的账户无权查看此页面，请联系管理员");
            case ShadResultType::NotFound:
                return ResultPage::tr("你访问的页面已被移动或删除");
            default:
                return ResultPage::tr("服务暂时不可用，请稍后再试");
        }
    }
} // namespace

ResultPage::ResultPage(QWidget* parent)
    : ExampleBasePage([] { return tr("结果"); }, [] { return tr("ShadResult：操作结束后用一整块区域说明结果，由状态图标、标题、说明与操作按钮组成。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("ShadResult(status, title, description) 创建，addButton 在底部添加操作按钮"); });
    ShadResult* basicResult = new ShadResult(this);
    basicResult->setStatus(ShadResultType::Success);
    ExampleI18n::bind(basicResult, [basicResult]()
    {
        basicResult->setTitle(tr("提交成功"));
        basicResult->setDescription(tr("订单号 2026111800123，预计 1 到 2 个工作日内完成审核"));
    });
    addResultButton(basicResult, ShadButtonType::Default, [] { return tr("返回列表"); });
    addResultButton(basicResult, ShadButtonType::Outline, [] { return tr("查看详情"); });
    addStretchItem(basicCard, basicResult);

    ShadCard* statusCard = addSection([] { return tr("状态"); }, [] { return tr("setStatus 切换状态，图标与颜色随之变化；Forbidden、NotFound、ServerError 对应 403、404、500"); });
    ShadSegmented* statusSegmented = new ShadSegmented(this);
    for (int index = 0; index < kStatusCount; ++index)
    {
        statusSegmented->addItem(QString());
    }
    ExampleI18n::bind(statusSegmented, [statusSegmented]()
    {
        statusSegmented->setItemText(ShadResultType::Success, tr("成功"));
        statusSegmented->setItemText(ShadResultType::Error, tr("失败"));
        statusSegmented->setItemText(ShadResultType::Warning, tr("警告"));
        statusSegmented->setItemText(ShadResultType::Info, tr("信息"));
        statusSegmented->setItemText(ShadResultType::Forbidden, QStringLiteral("403"));
        statusSegmented->setItemText(ShadResultType::NotFound, QStringLiteral("404"));
        statusSegmented->setItemText(ShadResultType::ServerError, QStringLiteral("500"));
    });
    ShadResult* statusResult = new ShadResult(this);
    const auto updateStatusResult = [statusSegmented, statusResult]()
    {
        const int status = statusSegmented->getCurrentIndex();
        statusResult->setStatus(static_cast<ShadResultType::Status>(status));
        statusResult->setTitle(statusTitle(status));
        statusResult->setDescription(statusDescription(status));
    };
    ExampleI18n::bind(statusResult, updateStatusResult);
    connect(statusSegmented, &ShadSegmented::currentIndexChanged, statusResult, updateStatusResult);
    addItem(statusCard, statusSegmented);
    addStretchItem(statusCard, statusResult);

    ShadCard* contentCard = addSection([] { return tr("自定义内容"); }, [] { return tr("addContentWidget 在说明与按钮之间加入任意组件，例如错误明细"); });
    ShadResult* errorResult = new ShadResult(this);
    errorResult->setStatus(ShadResultType::Error);
    ExampleI18n::bind(errorResult, [errorResult]()
    {
        errorResult->setTitle(tr("提交失败"));
        errorResult->setDescription(tr("请修正下面的问题后重新提交"));
    });
    addDetailLabel(errorResult, [] { return tr("账户已被冻结，暂时不能发起付款"); });
    addDetailLabel(errorResult, [] { return tr("收款银行卡信息不完整"); });
    addResultButton(errorResult, ShadButtonType::Default, [] { return tr("重新提交"); });
    addStretchItem(contentCard, errorResult);

    ShadCard* iconCard = addSection([] { return tr("自定义图标"); }, [] { return tr("setShadIcon 用任意 Lucide 图标替换状态图标，颜色仍然跟随状态"); });
    ShadResult* iconResult = new ShadResult(this);
    iconResult->setStatus(ShadResultType::Success);
    iconResult->setShadIcon(ShadIconType::Rocket);
    ExampleI18n::bind(iconResult, [iconResult]()
    {
        iconResult->setTitle(tr("版本已发布"));
        iconResult->setDescription(tr("用户会在下次启动时收到更新"));
    });
    addStretchItem(iconCard, iconResult);

    ShadCard* customCard = addSection([] { return tr("自定义状态"); }, [] { return tr("setColor 设置状态色，与 setShadIcon 组合可以表达任意状态；setMediaWidget 用任意组件取代图标"); });
    ShadResult* pendingResult = new ShadResult(this);
    pendingResult->setShadIcon(ShadIconType::Hourglass);
    pendingResult->setColor(QColor(0x8b, 0x5c, 0xf6));
    ExampleI18n::bind(pendingResult, [pendingResult]()
    {
        pendingResult->setTitle(tr("等待审核"));
        pendingResult->setDescription(tr("前面还有 3 个申请，预计等待 10 分钟"));
    });
    ShadResult* processingResult = new ShadResult(this);
    ShadSpinner* spinner = new ShadSpinner(processingResult);
    spinner->setSpinnerSize(kSpinnerSize);
    processingResult->setMediaWidget(spinner);
    ExampleI18n::bind(processingResult, [processingResult]()
    {
        processingResult->setTitle(tr("正在处理"));
        processingResult->setDescription(tr("处理完成前请不要关闭窗口"));
    });
    addStretchItem(customCard, pendingResult);
    addStretchItem(customCard, processingResult);
}

ResultPage::~ResultPage()
{
}
