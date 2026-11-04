#include "AvatarPage.h"

#include <QAction>
#include <QChar>
#include <QColor>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>

#include "ExampleI18n.h"
#include "ShadAvatar.h"
#include "ShadAvatarGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadTheme.h"

namespace
{
    // 生成图片的边长，头像按屏幕缩放比例从中缩小
    constexpr int kPhotoSize = 160;
    // 下拉菜单示例中按钮主体与头像之间的焦点环边距
    constexpr int kButtonFocusMargin = 3;

    // 示例用的人物头像：没有使用网络图片，按两种颜色生成渐变背景与简化的人像剪影
    struct PhotoStyle
    {
        QString fallback;
        QColor top;
        QColor bottom;
    };

    const QList<PhotoStyle>& photoStyles()
    {
        static const QList<PhotoStyle> styles{
            {QStringLiteral("CN"), QColor(0x60, 0xa5, 0xfa), QColor(0x1e, 0x3a, 0x8a)},
            {QStringLiteral("LR"), QColor(0xf9, 0xa8, 0xd4), QColor(0x9d, 0x17, 0x4d)},
            {QStringLiteral("ER"), QColor(0x86, 0xef, 0xac), QColor(0x16, 0x65, 0x34)},
        };
        return styles;
    }

    QPixmap createPhoto(const PhotoStyle& style)
    {
        QPixmap photo(kPhotoSize, kPhotoSize);
        photo.fill(Qt::transparent);
        QPainter painter(&photo);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(QPointF(0, 0), QPointF(kPhotoSize, kPhotoSize));
        gradient.setColorAt(0.0, style.top);
        gradient.setColorAt(1.0, style.bottom);
        painter.fillRect(photo.rect(), gradient);
        // 头部与肩膀
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawEllipse(QRectF(kPhotoSize * 0.32, kPhotoSize * 0.2, kPhotoSize * 0.36, kPhotoSize * 0.36));
        painter.drawEllipse(QRectF(kPhotoSize * 0.14, kPhotoSize * 0.62, kPhotoSize * 0.72, kPhotoSize * 0.6));
        return photo;
    }

    ShadAvatar* createAvatar(int styleIndex, bool withImage, ShadAvatarType::Size size, QWidget* parent)
    {
        const PhotoStyle& style = photoStyles().at(styleIndex);
        ShadAvatar* avatar = new ShadAvatar(style.fallback, parent);
        avatar->setAvatarSize(size);
        if (withImage)
        {
            avatar->setImage(createPhoto(style));
        }
        return avatar;
    }

    // 三种尺寸各一个头像，按 sm / default / lg 顺序
    QList<QWidget*> createSizeRow(int styleIndex, bool withImage, QWidget* parent)
    {
        QList<QWidget*> row;
        for (ShadAvatarType::Size size : {ShadAvatarType::Small, ShadAvatarType::Medium, ShadAvatarType::Large})
        {
            row.append(createAvatar(styleIndex, withImage, size, parent));
        }
        return row;
    }

    // 包含三个头像的头像组
    ShadAvatarGroup* createGroup(ShadAvatarType::Size size, bool isGrayscale, QWidget* parent)
    {
        ShadAvatarGroup* group = new ShadAvatarGroup(parent);
        group->setAvatarSize(size);
        for (int index = 0; index < photoStyles().size(); ++index)
        {
            ShadAvatar* avatar = createAvatar(index, true, size, group);
            avatar->setGrayscale(isGrayscale);
            group->addAvatar(avatar);
        }
        return group;
    }
} // namespace

AvatarPage::AvatarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("头像"); }, [] { return tr("ShadAvatar / ShadAvatarGroup：圆形头像，图片不可用时显示后备文字，支持徽章与头像组。"); }, parent)
{
    // 基础用法：图片、后备文字、加载失败与后备图标
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("图片、后备文字、加载失败与后备图标"); });
    ShadAvatar* missingAvatar = new ShadAvatar(QStringLiteral("ER"), this);
    missingAvatar->setImagePath(QStringLiteral("not-exist/avatar.png"));
    ShadAvatar* iconAvatar = new ShadAvatar(this);
    iconAvatar->setFallbackIcon(ShadIconType::User);
    addRow(basicCard, {createAvatar(0, true, ShadAvatarType::Medium, this), createAvatar(0, false, ShadAvatarType::Medium, this), missingAvatar, iconAvatar});

    // 尺寸
    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("setAvatarSize：小、中、大三种尺寸"); });
    addRow(sizeCard, createSizeRow(0, true, this));
    addRow(sizeCard, createSizeRow(0, false, this));

    // 徽章
    ShadCard* badgeCard = addSection([] { return tr("徽章"); }, [] { return tr("setBadgeVisible：显示徽章圆点，可改颜色表示在线"); });
    for (bool withImage : {true, false})
    {
        QList<QWidget*> row = createSizeRow(1, withImage, this);
        for (QWidget* widget : row)
        {
            static_cast<ShadAvatar*>(widget)->setBadgeVisible(true);
        }
        if (withImage)
        {
            // 对应 bg-green-600 dark:bg-green-800
            ShadAvatar* onlineAvatar = createAvatar(2, true, ShadAvatarType::Medium, this);
            onlineAvatar->setBadgeVisible(true);
            const auto updateOnlineColor = [onlineAvatar]()
            {
                onlineAvatar->setBadgeColor(shadTheme->isDark() ? QColor(0x16, 0x65, 0x34) : QColor(0x16, 0xa3, 0x4a));
            };
            connect(shadTheme, &ShadTheme::themeChanged, onlineAvatar, updateOnlineColor);
            updateOnlineColor();
            row.append(onlineAvatar);
        }
        addRow(badgeCard, row);
    }

    // 带图标的徽章
    ShadCard* badgeIconCard = addSection([] { return tr("带图标的徽章"); }, []
    {
        return tr("setBadgeIcon：在徽章中显示图标，小尺寸不显示");
    });
    for (bool withImage : {true, false})
    {
        const QList<QWidget*> row = createSizeRow(2, withImage, this);
        for (QWidget* widget : row)
        {
            ShadAvatar* avatar = static_cast<ShadAvatar*>(widget);
            avatar->setBadgeVisible(true);
            avatar->setBadgeIcon(withImage ? ShadIconType::Plus : ShadIconType::Check);
        }
        addRow(badgeIconCard, row);
    }

    // 头像组
    ShadCard* groupCard = addSection([] { return tr("头像组"); }, [] { return tr("多个头像重叠排列，组内尺寸统一"); });
    QList<QWidget*> groups;
    QList<QWidget*> countGroups;
    QList<QWidget*> iconGroups;
    for (ShadAvatarType::Size size : {ShadAvatarType::Small, ShadAvatarType::Medium, ShadAvatarType::Large})
    {
        groups.append(createGroup(size, false, this));
        ShadAvatarGroup* countGroup = createGroup(size, false, this);
        countGroup->setCountText(QStringLiteral("+3"));
        countGroups.append(countGroup);
        // 第三组的图片为灰度，对应 className="grayscale"
        ShadAvatarGroup* iconGroup = createGroup(size, size == ShadAvatarType::Large, this);
        iconGroup->setCountIcon(ShadIconType::Plus);
        iconGroups.append(iconGroup);
    }
    addRow(groupCard, groups);

    ShadCard* countCard = addSection([] { return tr("带数量的头像组"); }, [] { return tr("setCountText：在头像组末尾显示计数"); });
    addRow(countCard, countGroups);

    ShadCard* iconCountCard = addSection([] { return tr("带图标的头像组"); }, [] { return tr("setCountIcon：在头像组末尾显示图标"); });
    addRow(iconCountCard, iconGroups);

    // 下拉菜单：头像作为触发按钮的内容
    ShadCard* dropdownCard = addSection([] { return tr("下拉菜单"); }, [] { return tr("把头像放进圆形按钮，点击弹出下拉菜单"); });
    ShadButton* triggerButton = new ShadButton(this);
    triggerButton->setVariant(ShadButtonType::Ghost);
    triggerButton->setButtonSize(ShadButtonType::Icon);
    triggerButton->setRoundedFull(true);
    ShadAvatar* triggerAvatar = createAvatar(0, true, ShadAvatarType::Medium, triggerButton);
    // 头像不接收鼠标事件，点击交给按钮；按钮主体 36px，头像组件 36px，与按钮主体重合
    triggerAvatar->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    triggerAvatar->move(kButtonFocusMargin, kButtonFocusMargin);
    ShadDropdownMenu* accountMenu = new ShadDropdownMenu(triggerButton);
    QAction* profileAction = accountMenu->addItem(QString(), ShadIconType::User);
    QAction* billingAction = accountMenu->addItem(QString(), ShadIconType::CreditCard);
    QAction* settingsAction = accountMenu->addItem(QString(), ShadIconType::Settings);
    accountMenu->addSeparator();
    QAction* logoutAction = accountMenu->addItem(QString(), ShadIconType::LogOut);
    accountMenu->setItemDestructive(logoutAction, true);
    ExampleI18n::bind(triggerButton, [triggerButton, profileAction, billingAction, settingsAction, logoutAction]()
    {
        triggerButton->setAccessibleName(tr("账户菜单"));
        profileAction->setText(tr("个人资料"));
        billingAction->setText(tr("账单"));
        settingsAction->setText(tr("设置"));
        logoutAction->setText(tr("退出登录"));
    });
    connect(triggerButton, &ShadButton::clicked, accountMenu, [triggerButton, accountMenu]()
    {
        accountMenu->popup(triggerButton, Qt::AlignRight);
    });
    addRow(dropdownCard, {triggerButton});

    // RTL：徽章位于左下角，头像组从右向左排列
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左布局下徽章与头像组镜像"); });
    QWidget* rtlContainer = new QWidget(this);
    rtlContainer->setLayoutDirection(Qt::RightToLeft);
    QHBoxLayout* rtlLayout = new QHBoxLayout(rtlContainer);
    rtlLayout->setContentsMargins(0, 0, 0, 0);
    rtlLayout->setSpacing(16);
    ShadAvatar* rtlAvatar = createAvatar(1, true, ShadAvatarType::Large, rtlContainer);
    rtlAvatar->setBadgeVisible(true);
    ShadAvatarGroup* rtlGroup = createGroup(ShadAvatarType::Medium, false, rtlContainer);
    // 阿拉伯-印度数字 ٣
    rtlGroup->setCountText(QStringLiteral("+") + QString(QChar(0x0663)));
    rtlLayout->addWidget(rtlAvatar);
    rtlLayout->addWidget(rtlGroup);
    rtlLayout->addStretch(1);
    addItem(rtlCard, rtlContainer);
}

AvatarPage::~AvatarPage()
{
}
