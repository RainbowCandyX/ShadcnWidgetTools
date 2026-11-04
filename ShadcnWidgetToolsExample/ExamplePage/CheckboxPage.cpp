#include "CheckboxPage.h"

#include <QString>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"

CheckboxPage::CheckboxPage(QWidget* parent)
    : ExampleBasePage([] { return tr("复选框"); }, [] { return tr("ShadCheckbox：点击文字也可切换的复选框，支持三态与错误态。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("未选中与选中"); });
    ShadCheckbox* termsCheckbox = new ShadCheckbox(this);
    ShadCheckbox* checkedCheckbox = new ShadCheckbox(this);
    checkedCheckbox->setChecked(true);
    addItem(basicCard, termsCheckbox);
    addItem(basicCard, checkedCheckbox);

    ShadCard* stateCard = addSection([] { return tr("状态"); }, [] { return tr("禁用、半选与错误态"); });
    ShadCheckbox* disabledCheckbox = new ShadCheckbox(this);
    disabledCheckbox->setEnabled(false);
    ShadCheckbox* disabledCheckedCheckbox = new ShadCheckbox(this);
    disabledCheckedCheckbox->setChecked(true);
    disabledCheckedCheckbox->setEnabled(false);
    ShadCheckbox* partialCheckbox = new ShadCheckbox(this);
    partialCheckbox->setTristate(true);
    partialCheckbox->setCheckState(Qt::PartiallyChecked);
    ShadCheckbox* invalidCheckbox = new ShadCheckbox(this);
    invalidCheckbox->setInvalid(true);
    addItem(stateCard, disabledCheckbox);
    addItem(stateCard, disabledCheckedCheckbox);
    addItem(stateCard, partialCheckbox);
    addItem(stateCard, invalidCheckbox);

    ExampleI18n::bind(this, [termsCheckbox, checkedCheckbox, disabledCheckbox, disabledCheckedCheckbox, partialCheckbox, invalidCheckbox]()
    {
        termsCheckbox->setText(tr("接受服务条款"));
        checkedCheckbox->setText(tr("开启通知"));
        disabledCheckbox->setText(tr("禁用"));
        disabledCheckedCheckbox->setText(tr("禁用且选中"));
        partialCheckbox->setText(tr("半选"));
        invalidCheckbox->setText(tr("错误"));
    });
}

CheckboxPage::~CheckboxPage()
{
}
