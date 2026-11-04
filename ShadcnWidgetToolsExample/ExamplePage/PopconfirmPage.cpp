#include "PopconfirmPage.h"

#include <QCoreApplication>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadPopconfirm.h"

namespace
{
    enum class Result
    {
        None = 0,
        Confirmed,
        Cancelled,
    };

    void updateResultText(ShadLabel* label, Result result)
    {
        switch (result)
        {
            case Result::Confirmed:
                label->setText(QCoreApplication::translate("PopconfirmPage", "结果: 已确认"));
                break;
            case Result::Cancelled:
                label->setText(QCoreApplication::translate("PopconfirmPage", "结果: 已取消"));
                break;
            case Result::None:
                label->setText(QCoreApplication::translate("PopconfirmPage", "结果: 无"));
                break;
        }
    }

    ShadLabel* createResultLabel(const QList<ShadPopconfirm*>& popconfirms, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        const std::shared_ptr<Result> result = std::make_shared<Result>(Result::None);
        for (ShadPopconfirm* popconfirm : popconfirms)
        {
            QObject::connect(popconfirm, &ShadPopconfirm::confirmed, label, [label, result]()
            {
                *result = Result::Confirmed;
                updateResultText(label, *result);
            });
            QObject::connect(popconfirm, &ShadPopconfirm::cancelled, label, [label, result]()
            {
                *result = Result::Cancelled;
                updateResultText(label, *result);
            });
        }
        ExampleI18n::bind(label, [label, result]() { updateResultText(label, *result); });
        return label;
    }
}

PopconfirmPage::PopconfirmPage(QWidget* parent)
    : ExampleBasePage([] { return tr("气泡确认"); }, [] { return tr("ShadPopconfirm：在操作旁弹出的轻量确认。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("危险操作前确认"); });
    ShadButton* deleteButton = new ShadButton(QString(), ShadButtonType::Destructive, this);
    ShadPopconfirm* deleteConfirm = new ShadPopconfirm(this);
    deleteConfirm->setDestructive(true);
    deleteConfirm->attach(deleteButton);
    ExampleI18n::bind(deleteConfirm, [deleteButton, deleteConfirm]()
    {
        deleteButton->setText(tr("删除"));
        deleteConfirm->setTitle(tr("删除这条记录？"));
        deleteConfirm->setDescription(tr("删除后无法恢复。"));
        deleteConfirm->confirmButton()->setText(tr("删除"));
    });
    addRow(basicCard, {deleteButton});
    addItem(basicCard, createResultLabel({deleteConfirm}, this));

    struct SideOption
    {
        ShadPlacementType::Side side;
        ExampleI18n::TextFunction text;
    };
    const QList<SideOption> sideOptions = {{ShadPlacementType::Top, [] { return tr("上方"); }}, {ShadPlacementType::Bottom, [] { return tr("下方"); }}, {ShadPlacementType::Left, [] { return tr("左侧"); }}, {ShadPlacementType::Right, [] { return tr("右侧"); }}};
    ShadCard* sideCard = addSection([] { return tr("方向"); }, [] { return tr("在按钮四周弹出"); });
    QList<QWidget*> sideButtons;
    QList<ShadPopconfirm*> sidePopconfirms;
    for (const SideOption& option : sideOptions)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        ShadPopconfirm* popconfirm = new ShadPopconfirm(this);
        popconfirm->attach(button, option.side);
        const ExampleI18n::TextFunction text = option.text;
        ExampleI18n::bind(popconfirm, [button, popconfirm, text]()
        {
            button->setText(text());
            popconfirm->setTitle(tr("确认执行此操作？"));
        });
        sideButtons.append(button);
        sidePopconfirms.append(popconfirm);
    }
    addRow(sideCard, sideButtons);
    addItem(sideCard, createResultLabel(sidePopconfirms, this));

    ShadCard* customCard = addSection([] { return tr("自定义"); }, [] { return tr("图标与按钮文字"); });
    ShadButton* publishButton = new ShadButton(QString(), ShadButtonType::Default, this);
    ShadPopconfirm* publishConfirm = new ShadPopconfirm(this);
    publishConfirm->setShadIcon(ShadIconType::CircleQuestionMark);
    publishConfirm->attach(publishButton, ShadPlacementType::Bottom, ShadPlacementType::Start);
    ShadButton* archiveButton = new ShadButton(QString(), ShadButtonType::Secondary, this);
    ShadPopconfirm* archiveConfirm = new ShadPopconfirm(this);
    archiveConfirm->setShadIcon(ShadIconType::None);
    archiveConfirm->attach(archiveButton, ShadPlacementType::Bottom, ShadPlacementType::Start);
    ExampleI18n::bind(publishConfirm, [publishButton, publishConfirm, archiveButton, archiveConfirm]()
    {
        publishButton->setText(tr("发布"));
        publishConfirm->setTitle(tr("发布到正式环境？"));
        publishConfirm->setDescription(tr("所有用户都会看到新版本。"));
        publishConfirm->confirmButton()->setText(tr("发布"));
        publishConfirm->cancelButton()->setText(tr("再想想"));
        archiveButton->setText(tr("归档"));
        archiveConfirm->setTitle(tr("归档这个项目？"));
    });
    addRow(customCard, {publishButton, archiveButton});
    addItem(customCard, createResultLabel({publishConfirm, archiveConfirm}, this));
}

PopconfirmPage::~PopconfirmPage()
{
}
