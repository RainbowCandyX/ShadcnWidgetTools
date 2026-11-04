#include "ShadI18n.h"

#include <QApplication>
#include <QCoreApplication>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadI18nPrivate.h"

ShadI18n::ShadI18n(QObject* parent)
    : QObject(parent), d_ptr(new ShadI18nPrivate())
{
    Q_D(ShadI18n);
    d->q_ptr = this;
    if (QCoreApplication::instance() == nullptr)
    {
        qWarning("ShadI18n 在 QCoreApplication 创建前被初始化，无法安装翻译");
    }
#ifdef SHADCN_TRANSLATIONS
    // 组件库内置翻译，由 CMake 的 qt_add_translations 嵌入资源
    addTranslations(QStringLiteral(":/ShadcnWidgetTools/i18n"), QStringLiteral("ShadcnWidgetTools"));
#endif
}

ShadI18n::~ShadI18n()
{
}

ShadI18n* ShadI18n::getInstance()
{
    static ShadI18n instance;
    return &instance;
}

void ShadI18n::setLanguage(const QLocale& language)
{
    Q_D(ShadI18n);
    if (d->_language == language)
    {
        return;
    }
    d->_language = language;
    QLocale::setDefault(language);
    if (QCoreApplication::instance() != nullptr)
    {
        for (ShadI18nPrivate::TranslationSource& source : d->_sources)
        {
            d->reloadSource(source);
        }
    }
    // 组件在创建时记下 locale，之后不随默认 locale 变化；没有显式设置过 locale 的顶层窗口重新取默认 locale，
    // Qt 随后把新 locale 传给没有显式设置的子组件并发送 LocaleChange，日历、日期与时间选择器按新语言显示
    if (qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr)
    {
        const QWidgetList windows = QApplication::topLevelWidgets();
        for (QWidget* window : windows)
        {
            if (!window->testAttribute(Qt::WA_SetLocale))
            {
                window->unsetLocale();
            }
        }
    }
    Q_EMIT languageChanged(language);
}

QLocale ShadI18n::getLanguage() const
{
    Q_D(const ShadI18n);
    return d->_language;
}

void ShadI18n::addTranslations(const QString& directory, const QString& prefix)
{
    Q_D(ShadI18n);
    for (const ShadI18nPrivate::TranslationSource& source : std::as_const(d->_sources))
    {
        if (source.directory == directory && source.prefix == prefix)
        {
            return;
        }
    }
    d->_sources.append(ShadI18nPrivate::TranslationSource{directory, prefix, nullptr});
    if (QCoreApplication::instance() != nullptr)
    {
        d->reloadSource(d->_sources.last());
    }
}
