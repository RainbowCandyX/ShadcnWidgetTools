#include "ShadPagination.h"

#include <QEvent>
#include <QFont>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QString>
#include <QtGlobal>

#include <initializer_list>
#include <utility>

#include "ShadButton.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPaginationNavButton.h"
#include "ShadPaginationPrivate.h"
#include "ShadStyleHelper.h"

namespace
{
    // 上一页、下一页使用默认文字（使用者改过的保持不变）；跳转区的文字与无障碍名称直接刷新
    void applyDefaultTexts(ShadPagination* pagination, ShadPaginationPrivate* d)
    {
        ShadStyleHelper::applyDefaultText(pagination, "previousText", pagination->getPreviousText(), ShadPagination::tr("上一页"), [pagination](const QString& text) { pagination->setPreviousText(text); });
        ShadStyleHelper::applyDefaultText(pagination, "nextText", pagination->getNextText(), ShadPagination::tr("下一页"), [pagination](const QString& text) { pagination->setNextText(text); });
        d->_jumpPrefix->setText(ShadPagination::tr("跳至"));
        d->_jumpInput->setAccessibleName(ShadPagination::tr("跳转页码"));
        d->_jumpSuffix->setText(ShadPagination::tr("页"));
    }
} // namespace

ShadPagination::ShadPagination(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadPaginationPrivate())
{
    Q_D(ShadPagination);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    d->_previousButton = new ShadPaginationNavButton(false, this);
    d->_nextButton = new ShadPaginationNavButton(true, this);
    connect(d->_previousButton, &ShadButton::clicked, this, [d]() { d->changePage(d->_currentPage - 1, false); });
    connect(d->_nextButton, &ShadButton::clicked, this, [d]() { d->changePage(d->_currentPage + 1, false); });

    // 页码跳转：跳至 [输入框] 页，只能输入数字
    d->_jumpPrefix = new ShadLabel(QString(), this);
    d->_jumpInput = new ShadInput(this);
    d->_jumpInput->setFixedWidth(ShadPaginationPrivate::kJumpInputWidth + 2 * ShadStyleHelper::kFocusRingWidth);
    d->_jumpInput->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("\\d{0,6}")), d->_jumpInput));
    d->_jumpSuffix = new ShadLabel(QString(), this);
    connect(d->_jumpInput, &ShadInput::returnPressed, this, [d]() { d->jumpToInput(); });
    connect(d->_jumpInput, &ShadInput::textEdited, this, [d]() { d->updateJumpInvalid(); });
    // setPreviousText / setNextText 会调用 updateItems，必须在子组件创建之后
    applyDefaultTexts(this, d);

    // 字体变化会触发 changeEvent 重新布局，必须在子组件创建之后设置
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->updateItems();
}

ShadPagination::ShadPagination(int pageCount, QWidget* parent)
    : ShadPagination(parent)
{
    setPageCount(pageCount);
}

ShadPagination::~ShadPagination()
{
    Q_D(ShadPagination);
    // 私有数据先于子组件释放，断开捕获私有数据的连接
    for (QObject* sender : {static_cast<QObject*>(d->_previousButton), static_cast<QObject*>(d->_nextButton), static_cast<QObject*>(d->_jumpInput)})
    {
        QObject::disconnect(sender, nullptr, this, nullptr);
    }
    for (ShadButton* button : std::as_const(d->_pageButtons))
    {
        QObject::disconnect(button, nullptr, this, nullptr);
    }
}

void ShadPagination::setPageCount(int pageCount)
{
    Q_D(ShadPagination);
    const int count = qMax(0, pageCount);
    if (d->_pageCount == count)
    {
        return;
    }
    d->_pageCount = count;
    const int boundedPage = qBound(1, d->_currentPage, qMax(1, count));
    const bool isPageChanged = boundedPage != d->_currentPage;
    d->_currentPage = boundedPage;
    d->updateJumpInvalid();
    d->updateItems();
    if (isPageChanged)
    {
        Q_EMIT currentPageChanged(d->_currentPage);
    }
}

int ShadPagination::getPageCount() const
{
    Q_D(const ShadPagination);
    return d->_pageCount;
}

void ShadPagination::setCurrentPage(int page)
{
    Q_D(ShadPagination);
    const int boundedPage = qBound(1, page, qMax(1, d->_pageCount));
    if (d->_currentPage == boundedPage)
    {
        return;
    }
    d->_currentPage = boundedPage;
    d->updateItems();
    Q_EMIT currentPageChanged(d->_currentPage);
}

int ShadPagination::getCurrentPage() const
{
    Q_D(const ShadPagination);
    return d->_currentPage;
}

void ShadPagination::setSiblingCount(int count)
{
    Q_D(ShadPagination);
    d->_siblingCount = qMax(0, count);
    d->updateItems();
}

int ShadPagination::getSiblingCount() const
{
    Q_D(const ShadPagination);
    return d->_siblingCount;
}

void ShadPagination::setBoundaryCount(int count)
{
    Q_D(ShadPagination);
    d->_boundaryCount = qMax(0, count);
    d->updateItems();
}

int ShadPagination::getBoundaryCount() const
{
    Q_D(const ShadPagination);
    return d->_boundaryCount;
}

void ShadPagination::setNavigationVisible(bool visible)
{
    Q_D(ShadPagination);
    d->_isNavigationVisible = visible;
    d->updateItems();
}

bool ShadPagination::isNavigationVisible() const
{
    Q_D(const ShadPagination);
    return d->_isNavigationVisible;
}

void ShadPagination::setNavigationTextVisible(bool visible)
{
    Q_D(ShadPagination);
    d->_previousButton->setLabelVisible(visible);
    d->_nextButton->setLabelVisible(visible);
    d->updateItems();
}

bool ShadPagination::isNavigationTextVisible() const
{
    Q_D(const ShadPagination);
    return d->_previousButton->isLabelVisible();
}

void ShadPagination::setPreviousText(const QString& text)
{
    Q_D(ShadPagination);
    d->_previousButton->setLabel(text);
    d->updateItems();
}

QString ShadPagination::getPreviousText() const
{
    Q_D(const ShadPagination);
    return d->_previousButton->getLabel();
}

void ShadPagination::setNextText(const QString& text)
{
    Q_D(ShadPagination);
    d->_nextButton->setLabel(text);
    d->updateItems();
}

QString ShadPagination::getNextText() const
{
    Q_D(const ShadPagination);
    return d->_nextButton->getLabel();
}

void ShadPagination::setPageButtonsVisible(bool visible)
{
    Q_D(ShadPagination);
    d->_isPageButtonsVisible = visible;
    d->updateItems();
}

bool ShadPagination::isPageButtonsVisible() const
{
    Q_D(const ShadPagination);
    return d->_isPageButtonsVisible;
}

void ShadPagination::setJumpVisible(bool visible)
{
    Q_D(ShadPagination);
    d->_isJumpVisible = visible;
    d->updateItems();
}

bool ShadPagination::isJumpVisible() const
{
    Q_D(const ShadPagination);
    return d->_isJumpVisible;
}

void ShadPagination::setJumpTexts(const QString& prefix, const QString& suffix)
{
    Q_D(ShadPagination);
    d->_jumpPrefix->setText(prefix);
    d->_jumpSuffix->setText(suffix);
    d->updateItems();
}

void ShadPagination::setAlignment(Qt::Alignment alignment)
{
    Q_D(ShadPagination);
    d->_alignment = alignment & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter);
    d->layoutItems();
}

Qt::Alignment ShadPagination::getAlignment() const
{
    Q_D(const ShadPagination);
    return d->_alignment;
}

QSize ShadPagination::sizeHint() const
{
    Q_D(const ShadPagination);
    const int ring = ShadStyleHelper::kFocusRingWidth;
    return QSize(d->contentWidth() + 2 * ring, ShadPaginationPrivate::kButtonSize + 2 * ring);
}

QSize ShadPagination::minimumSizeHint() const
{
    return sizeHint();
}

void ShadPagination::changeEvent(QEvent* event)
{
    Q_D(ShadPagination);
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::FontChange)
    {
        d->layoutItems();
        updateGeometry();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        applyDefaultTexts(this, d);
        // 刷新页码按钮的无障碍名称，并按新文字宽度重新布局
        d->updateItems();
    }
}

void ShadPagination::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadPagination);
    QWidget::resizeEvent(event);
    d->layoutItems();
}
