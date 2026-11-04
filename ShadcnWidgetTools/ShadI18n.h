#ifndef SHADI18N_H
#define SHADI18N_H

#include <QLocale>
#include <QObject>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

#define shadI18n ShadI18n::getInstance()

// 界面语言管理，第一次访问 shadI18n 时创建，应在 QApplication 创建之后使用
// 组件库内置文字的原文为简体中文，另带英文、繁体中文、日语与韩语翻译；切换语言时安装对应的 QTranslator，Qt 随后向所有组件发送 QEvent::LanguageChange，
// 组件内置文字随之刷新（通过 setPlaceholderText 等接口设置过的文字保持不变）
// 应用自己的翻译通过 addTranslations 注册，切换语言时一并加载；应用自己的界面文字可在 languageChanged 或 QEvent::LanguageChange 中重新设置
class ShadI18nPrivate;

class SHADCN_EXPORT ShadI18n : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadI18n)
    Q_PROPERTY(QLocale language READ getLanguage WRITE setLanguage NOTIFY languageChanged)

    public:
        static ShadI18n* getInstance();

        // 切换界面语言，默认为简体中文（原文，不安装翻译）；与当前语言相同时不做任何事
        // 同时调用 QLocale::setDefault，并让没有显式设置过 locale 的窗口及其子组件改用新 locale，月份名、日期格式等随之刷新；
        // 通过 setLocale 显式设置过 locale 的组件保持不变
        // 某组翻译没有该语言的文件时，这组文字保持原文并输出警告
        void setLanguage(const QLocale& language);
        QLocale getLanguage() const;

        // 注册一组翻译文件：在 directory 中查找 prefix_<语言>.qm（例如 MyApp_en.qm 或 MyApp_en_US.qm），按 QTranslator::load 的规则依次匹配
        // directory 可以是资源路径（如 :/i18n）；注册后立即按当前语言加载，同一 directory 与 prefix 只注册一次
        void addTranslations(const QString& directory, const QString& prefix);

    Q_SIGNALS:
        // 翻译已安装、组件已收到 LanguageChange 之后发出
        void languageChanged(const QLocale& language);

    private:
        explicit ShadI18n(QObject* parent = nullptr);
        ~ShadI18n() override;
};

#endif // SHADI18N_H
