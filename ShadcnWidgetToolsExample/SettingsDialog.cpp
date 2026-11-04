#include "SettingsDialog.h"

#include <QCoreApplication>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QList>
#include <QLocale>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cstddef>
#include <iterator>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadField.h"
#include "ShadFieldGroup.h"
#include "ShadFieldSet.h"
#include "ShadI18n.h"
#include "ShadLabel.h"
#include "ShadSidebar.h"
#include "ShadSlider.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"
#include "ShadToaster.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"
#include "ShadWindow.h"

namespace
{
    constexpr int kDialogWidth = 760;
    constexpr int kFieldSpacing = 12;
    constexpr int kColumnGap = 32;
    constexpr int kSectionGap = 24;
    const QList<int> kRadiusOptions{0, 6, 10, 14};
    // 顺序与 ShadToastType::Position 一致；在 QApplication 创建前初始化，只做翻译标记，使用时再翻译
    const char* const kToastPositionNames[] = {
        QT_TRANSLATE_NOOP("SettingsDialog", "左上"), QT_TRANSLATE_NOOP("SettingsDialog", "顶部居中"),
        QT_TRANSLATE_NOOP("SettingsDialog", "右上"), QT_TRANSLATE_NOOP("SettingsDialog", "左下"),
        QT_TRANSLATE_NOOP("SettingsDialog", "底部居中"), QT_TRANSLATE_NOOP("SettingsDialog", "右下")
    };
    // 顺序与 ShadThemeType::BaseColor 一致
    const QStringList kBaseColorNames{
        QStringLiteral("neutral"), QStringLiteral("zinc"), QStringLiteral("slate"), QStringLiteral("stone"),
        QStringLiteral("gray")
    };
    // 顺序与 ShadWindowType::Effect 一致；只做翻译标记，使用时再翻译
    const char* const kWindowEffectNames[] = {
        QT_TRANSLATE_NOOP("SettingsDialog", "无"), QT_TRANSLATE_NOOP("SettingsDialog", "壁纸模糊"),
        QT_TRANSLATE_NOOP("SettingsDialog", "背景模糊"), QT_TRANSLATE_NOOP("SettingsDialog", "亚克力"),
        QT_TRANSLATE_NOOP("SettingsDialog", "云母"), QT_TRANSLATE_NOOP("SettingsDialog", "云母 Alt")
    };
    constexpr int kMaxTransitionDuration = 1000;
    constexpr int kTransitionDurationStep = 50;
    constexpr int kSliderWidth = 160;
    constexpr int kSliderGap = 8;
    constexpr int kValueWidth = 56;
    constexpr int kMaxPercent = 100;
    constexpr int kOpacityStep = 5;
    constexpr int kMaxBlurRadius = 200;
    constexpr int kBlurRadiusStep = 10;

    // 按当前语言翻译 QT_TRANSLATE_NOOP 标记的名称列表
    template <std::size_t Count>
    QStringList translatedNames(const char* const (&names)[Count])
    {
        QStringList result;
        for (const char* name : names)
        {
            result.append(QCoreApplication::translate("SettingsDialog", name));
        }
        return result;
    }

    QString durationText(int msecs)
    {
        return QStringLiteral("%1 ms").arg(msecs);
    }

    QString percentText(int percent)
    {
        return QStringLiteral("%1%").arg(percent);
    }

    QString pixelText(int pixels)
    {
        return QStringLiteral("%1 px").arg(pixels);
    }

    // 单选按钮组：不可取消选中；选项文字随语言刷新
    template <typename OnSelected>
    ShadToggleGroup* createChoiceGroup(const QList<ExampleI18n::TextFunction>& texts, int currentIndex, QWidget* parent, OnSelected onSelected)
    {
        ShadToggleGroup* group = new ShadToggleGroup(ShadToggleGroupType::Single, parent);
        group->setVariant(ShadToggleType::Outline);
        for (int i = 0; i < texts.size(); ++i)
        {
            group->addItem(QString());
        }
        ExampleI18n::bind(group, [group, texts]()
        {
            for (int i = 0; i < texts.size(); ++i)
            {
                group->itemAt(i)->setText(texts.at(i)());
            }
        });
        group->setCurrentIndex(currentIndex);
        QObject::connect(group, &ShadToggleGroup::currentIndexChanged, group, [group, onSelected, selectedIndex = currentIndex](int index) mutable
        {
            if (index < 0)
            {
                if (selectedIndex >= 0)
                {
                    group->setCurrentIndex(selectedIndex);
                }
                return;
            }
            selectedIndex = index;
            onSelected(index);
        });
        return group;
    }

    ShadComboBox* createComboBox(const QStringList& texts, int currentIndex, QWidget* parent)
    {
        ShadComboBox* comboBox = new ShadComboBox(parent);
        comboBox->addItems(texts);
        comboBox->setCurrentIndex(currentIndex);
        return comboBox;
    }

    ShadField* createField(ExampleI18n::TextFunction label, QWidget* control, QWidget* parent)
    {
        ShadField* field = new ShadField(QString(), control, parent);
        field->setOrientation(ShadFieldType::Horizontal);
        ExampleI18n::bind(field, [field, label]() { field->setLabel(label()); });
        return field;
    }

    template <typename Format, typename OnChanged>
    QWidget* createValueSlider(int value, int maximum, int step, QWidget* parent, Format format, OnChanged onChanged)
    {
        QWidget* control = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(control);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kSliderGap);
        ShadSlider* slider = new ShadSlider(Qt::Horizontal, control);
        slider->setRange(0, maximum);
        slider->setSingleStep(step);
        slider->setPageStep(step);
        slider->setValue(value);
        slider->setFixedWidth(kSliderWidth);
        ShadLabel* valueLabel = new ShadLabel(format(value), control);
        valueLabel->setColorRole(ShadThemeType::MutedForeground);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        valueLabel->setFixedWidth(kValueWidth);
        layout->addWidget(slider);
        layout->addWidget(valueLabel);
        QObject::connect(slider, &ShadSlider::valueChanged, valueLabel, [valueLabel, format, onChanged](int newValue)
        {
            valueLabel->setText(format(newValue));
            onChanged(newValue);
        });
        return control;
    }

    ShadFieldSet* createSection(ExampleI18n::TextFunction legend, const QList<ShadField*>& fields, QWidget* parent)
    {
        ShadFieldSet* section = new ShadFieldSet(QString(), QString(), parent);
        ExampleI18n::bind(section, [section, legend]() { section->setLegend(legend()); });
        ShadFieldGroup* group = new ShadFieldGroup(section);
        group->setSpacing(kFieldSpacing);
        for (ShadField* field : fields)
        {
            group->addWidget(field);
        }
        section->addWidget(group);
        return section;
    }

    int themeIndex()
    {
        if (shadTheme->isFollowSystemTheme())
        {
            return 2;
        }
        return shadTheme->isDark() ? 1 : 0;
    }
} // namespace

SettingsDialog::SettingsDialog(ShadSidebar* sidebar, ShadToaster* toaster, QWidget* parent)
    : ShadDialog(parent)
{
    resize(kDialogWidth, height());
    if (parent != nullptr)
    {
        setMinimumHeight(parent->window()->height());
    }
    ExampleLayout::addStayOnTopButton(this);
    ExampleI18n::bind(this, [this]()
    {
        setTitle(tr("设置"));
        setDescription(tr("修改后立即生效。"));
    });

    // 语言名称使用各自的语言显示，不翻译；切换后所有文字原地刷新，本对话框保持打开
    const QList<QLocale> languages = {QLocale(QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China), QLocale(QLocale::Chinese, QLocale::TraditionalHanScript, QLocale::Taiwan), QLocale(QLocale::English, QLocale::UnitedStates), QLocale(QLocale::Japanese, QLocale::Japan), QLocale(QLocale::Korean, QLocale::SouthKorea)};
    const QLocale currentLanguage = shadI18n->getLanguage();
    int languageIndex = 0;
    for (int index = 0; index < languages.size(); ++index)
    {
        if (languages.at(index).language() == currentLanguage.language() && (currentLanguage.language() != QLocale::Chinese || languages.at(index).script() == currentLanguage.script()))
        {
            languageIndex = index;
            break;
        }
    }
    ShadComboBox* languageComboBox = createComboBox({QStringLiteral("简体中文"), QStringLiteral("繁體中文"), QStringLiteral("English"), QStringLiteral("日本語"), QStringLiteral("한국어")}, languageIndex, this);
    connect(languageComboBox, &ShadComboBox::currentIndexChanged, this, [languages](int index)
    {
        if (index < 0 || index >= languages.size())
        {
            return;
        }
        shadI18n->setLanguage(languages.at(index));
    });

    ShadToggleGroup* themeGroup = createChoiceGroup({[] { return tr("浅色"); }, [] { return tr("深色"); }, [] { return tr("跟随系统"); }}, themeIndex(), this, [](int index)
    {
        if (index == 2)
        {
            shadTheme->setFollowSystemTheme(true);
            return;
        }
        shadTheme->setThemeMode(index == 1 ? ShadThemeType::Dark : ShadThemeType::Light);
    });
    ShadComboBox* baseColorComboBox = createComboBox(kBaseColorNames, static_cast<int>(shadTheme->getBaseColor()), this);
    connect(baseColorComboBox, &ShadComboBox::currentIndexChanged, this, [](int index)
    {
        if (index >= 0)
        {
            shadTheme->setBaseColor(static_cast<ShadThemeType::BaseColor>(index));
        }
    });
    QList<ExampleI18n::TextFunction> radiusNames;
    for (int radius : kRadiusOptions)
    {
        radiusNames.append([radius]() { return QString::number(radius); });
    }
    ShadToggleGroup* radiusGroup = createChoiceGroup(radiusNames, kRadiusOptions.indexOf(shadTheme->getBaseRadius()), this, [](int index)
    {
        shadTheme->setBaseRadius(kRadiusOptions.at(index));
    });
    // 只列出当前平台支持的特效
    QList<ShadWindowType::Effect> effects;
    for (int i = 0; i < static_cast<int>(std::size(kWindowEffectNames)); ++i)
    {
        const ShadWindowType::Effect effect = static_cast<ShadWindowType::Effect>(i);
        if (ShadTheme::isWindowEffectSupported(effect))
        {
            effects.append(effect);
        }
    }
    const QList<ShadField*> appearanceFields{
        createField([] { return tr("语言"); }, languageComboBox, this),
        createField([] { return tr("主题"); }, themeGroup, this),
        createField([] { return tr("基色"); }, baseColorComboBox, this),
        createField([] { return tr("圆角"); }, radiusGroup, this),
    };

    const auto effectPercent = []()
    {
        return qRound(shadTheme->getWindowEffectOpacity(shadTheme->getWindowEffect()) * kMaxPercent);
    };
    QWidget* opacityControl = createValueSlider(effectPercent(), kMaxPercent, kOpacityStep, this, percentText, [](int percent)
    {
        shadTheme->setWindowEffectOpacity(shadTheme->getWindowEffect(), static_cast<qreal>(percent) / kMaxPercent);
    });
    QWidget* blurControl = createValueSlider(shadTheme->getWindowEffectBlurRadius(), kMaxBlurRadius, kBlurRadiusStep, this, pixelText, [](int radius)
    {
        shadTheme->setWindowEffectBlurRadius(radius);
    });
    QWidget* surfaceOpacityControl = createValueSlider(qRound(shadTheme->getWindowEffectSurfaceOpacity() * kMaxPercent), kMaxPercent, kOpacityStep, this, percentText, [](int percent)
    {
        shadTheme->setWindowEffectSurfaceOpacity(static_cast<qreal>(percent) / kMaxPercent);
    });
#ifdef Q_OS_MACOS
    ShadSwitch* desktopOnlySwitch = new ShadSwitch(this);
    desktopOnlySwitch->setChecked(shadTheme->isWindowEffectDesktopOnly());
    connect(desktopOnlySwitch, &ShadSwitch::toggled, this, [](bool checked)
    {
        shadTheme->setWindowEffectDesktopOnly(checked);
    });
#else
    ShadSwitch* desktopOnlySwitch = nullptr;
#endif
    const auto updateEffectControls = [opacityControl, blurControl, surfaceOpacityControl, desktopOnlySwitch]()
    {
        const ShadWindowType::Effect effect = shadTheme->getWindowEffect();
        opacityControl->setEnabled(effect != ShadWindowType::NoEffect);
#ifdef Q_OS_MACOS
        blurControl->setEnabled(effect == ShadWindowType::Wallpaper || effect == ShadWindowType::Blur);
#else
        blurControl->setEnabled(effect == ShadWindowType::Wallpaper);
#endif
        surfaceOpacityControl->setEnabled(effect != ShadWindowType::NoEffect);
        if (desktopOnlySwitch != nullptr)
        {
            desktopOnlySwitch->setEnabled(effect == ShadWindowType::Blur);
        }
    };
    updateEffectControls();
    ShadComboBox* effectComboBox = createComboBox(QStringList(effects.size(), QString()), effects.indexOf(shadTheme->getWindowEffect()), this);
    // 按序号刷新选项文字，当前选中项不变
    ExampleI18n::bind(effectComboBox, [effectComboBox, effects]()
    {
        const QStringList allEffectNames = translatedNames(kWindowEffectNames);
        for (int i = 0; i < effects.size(); ++i)
        {
            effectComboBox->setItemText(i, allEffectNames.at(static_cast<int>(effects.at(i))));
        }
    });
    connect(effectComboBox, &ShadComboBox::currentIndexChanged, this, [effects, opacityControl, effectPercent, updateEffectControls](int index)
    {
        if (index < 0)
        {
            return;
        }
        shadTheme->setWindowEffect(effects.at(index));
        opacityControl->findChild<ShadSlider*>()->setValue(effectPercent());
        updateEffectControls();
    });
    QList<ShadField*> effectFields{
        createField([] { return tr("特效"); }, effectComboBox, this),
        createField([] { return tr("底色不透明度"); }, opacityControl, this),
        createField([] { return tr("模糊程度"); }, blurControl, this),
        createField([] { return tr("组件不透明度"); }, surfaceOpacityControl, this),
    };
#ifdef Q_OS_MACOS
    ShadField* desktopOnlyField = createField([] { return tr("仅模糊桌面"); }, desktopOnlySwitch, this);
    ExampleI18n::bind(desktopOnlyField, [desktopOnlyField]() { desktopOnlyField->setDescription(tr("只模糊壁纸，不含其他窗口；需要录屏权限，未授权时使用系统模糊")); });
    effectFields.insert(3, desktopOnlyField);
#endif

    ShadSwitch* rtlSwitch = new ShadSwitch(this);
    rtlSwitch->setChecked(QGuiApplication::layoutDirection() == Qt::RightToLeft);
    connect(rtlSwitch, &ShadSwitch::toggled, this, [](bool checked)
    {
        QGuiApplication::setLayoutDirection(checked ? Qt::RightToLeft : Qt::LeftToRight);
    });
    const int collapsibleIndex = sidebar->getCollapsible() == ShadSidebarType::Icon ? 0 : (sidebar->getCollapsible() == ShadSidebarType::Offcanvas ? 1 : -1);
    ShadToggleGroup* collapsibleGroup = createChoiceGroup({[] { return tr("图标"); }, [] { return tr("完全隐藏"); }}, collapsibleIndex, this, [sidebar](int index)
    {
        sidebar->setCollapsible(index == 0 ? ShadSidebarType::Icon : ShadSidebarType::Offcanvas);
    });
    ShadSwitch* exclusiveSwitch = new ShadSwitch(this);
    exclusiveSwitch->setChecked(sidebar->isSubMenuExclusive());
    connect(exclusiveSwitch, &ShadSwitch::toggled, this, [sidebar](bool checked)
    {
        sidebar->setSubMenuExclusive(checked);
    });
    QList<ShadField*> layoutFields{
        createField([] { return tr("从右到左布局"); }, rtlSwitch, this),
        createField([] { return tr("侧栏收起方式"); }, collapsibleGroup, this),
        createField([] { return tr("只展开一个分类"); }, exclusiveSwitch, this),
    };
#ifdef Q_OS_MACOS
    if (ShadWindow* window = qobject_cast<ShadWindow*>(parent))
    {
        const int buttonPositionIndex = window->getButtonPosition() == ShadWindowType::ButtonsOnLeft ? 1 : 0;
        ShadToggleGroup* buttonPositionGroup = createChoiceGroup({[] { return tr("右侧"); }, [] { return tr("左侧"); }}, buttonPositionIndex, this, [window](int index)
        {
            window->setButtonPosition(index == 1 ? ShadWindowType::ButtonsOnLeft : ShadWindowType::ButtonsOnRight);
        });
        layoutFields.append(createField([] { return tr("窗口按钮位置"); }, buttonPositionGroup, this));
    }
#endif

    ShadComboBox* toastPositionComboBox = createComboBox(translatedNames(kToastPositionNames), static_cast<int>(toaster->getPosition()), this);
    ExampleI18n::bind(toastPositionComboBox, [toastPositionComboBox]()
    {
        const QStringList names = translatedNames(kToastPositionNames);
        for (int i = 0; i < names.size(); ++i)
        {
            toastPositionComboBox->setItemText(i, names.at(i));
        }
    });
    connect(toastPositionComboBox, &ShadComboBox::currentIndexChanged, this, [toaster](int index)
    {
        if (index < 0)
        {
            return;
        }
        toaster->setPosition(static_cast<ShadToastType::Position>(index));
        toaster->add(tr("提示位置已更改"), tr("之后的提示显示在这里"));
    });
    const QList<ShadField*> notificationFields{
        createField([] { return tr("提示位置"); }, toastPositionComboBox, this),
    };

    // 关闭与风格合为一组
    const int transitionIndex = shadTheme->isPageTransitionEnabled() ? static_cast<int>(shadTheme->getPageTransitionStyle()) + 1 : 0;
    ShadToggleGroup* transitionGroup = createChoiceGroup({[] { return tr("关闭"); }, [] { return tr("淡入淡出"); }, [] { return tr("滑入"); }}, transitionIndex, this, [](int index)
    {
        shadTheme->setPageTransitionEnabled(index > 0);
        if (index > 0)
        {
            shadTheme->setPageTransitionStyle(static_cast<ShadThemeType::PageTransitionStyle>(index - 1));
        }
    });
    QWidget* durationSlider = createValueSlider(shadTheme->getPageTransitionDuration(), kMaxTransitionDuration, kTransitionDurationStep, this, durationText, [](int msecs)
    {
        shadTheme->setPageTransitionDuration(msecs);
    });
    QWidget* indicatorSlider = createValueSlider(sidebar->getIndicatorDuration(), kMaxTransitionDuration, kTransitionDurationStep, this, durationText, [sidebar](int msecs)
    {
        sidebar->setIndicatorDuration(msecs);
    });
    const QList<ShadField*> animationFields{
        createField([] { return tr("页面切换"); }, transitionGroup, this),
        createField([] { return tr("切换时长"); }, durationSlider, this),
        createField([] { return tr("侧栏指示条"); }, indicatorSlider, this),
    };

    QGridLayout* sectionGrid = new QGridLayout();
    sectionGrid->setContentsMargins(0, 0, 0, 0);
    sectionGrid->setHorizontalSpacing(kColumnGap);
    sectionGrid->setVerticalSpacing(kSectionGap);
    sectionGrid->setColumnStretch(0, 1);
    sectionGrid->setColumnStretch(1, 1);
    sectionGrid->addWidget(createSection([] { return tr("外观"); }, appearanceFields, this), 0, 0, Qt::AlignTop);
    sectionGrid->addWidget(createSection([] { return tr("动画"); }, animationFields, this), 0, 1, Qt::AlignTop);
    sectionGrid->addWidget(createSection([] { return tr("布局"); }, layoutFields, this), 1, 0, Qt::AlignTop);
    sectionGrid->addWidget(createSection([] { return tr("窗口特效"); }, effectFields, this), 1, 1, Qt::AlignTop);
    sectionGrid->addWidget(createSection([] { return tr("通知"); }, notificationFields, this), 2, 0, Qt::AlignTop);
    contentLayout()->addLayout(sectionGrid);

    ShadButton* doneButton = new ShadButton(this);
    ExampleI18n::bind(doneButton, [doneButton]() { doneButton->setText(tr("完成")); });
    doneButton->setDefault(true);
    footerLayout()->addWidget(doneButton);
    connect(doneButton, &ShadButton::clicked, this, &ShadDialog::accept);
}

SettingsDialog::~SettingsDialog()
{
}
