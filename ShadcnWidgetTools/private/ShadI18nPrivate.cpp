#include "ShadI18nPrivate.h"

#include <QCoreApplication>
#include <QTranslator>
#include <QtGlobal>

#include "ShadI18n.h"

ShadI18nPrivate::ShadI18nPrivate()
{
}

ShadI18nPrivate::~ShadI18nPrivate()
{
}

bool ShadI18nPrivate::isSourceLanguage(const QLocale& language)
{
    return language.language() == QLocale::Chinese && language.script() != QLocale::TraditionalHanScript;
}

void ShadI18nPrivate::reloadSource(TranslationSource& source)
{
    Q_Q(ShadI18n);
    if (source.translator != nullptr)
    {
        QCoreApplication::removeTranslator(source.translator);
        delete source.translator;
        source.translator = nullptr;
    }
    if (isSourceLanguage(_language))
    {
        return;
    }
    QTranslator* translator = new QTranslator(q);
    if (!translator->load(_language, source.prefix, QStringLiteral("_"), source.directory))
    {
        qWarning("未找到 %s 的翻译文件: %s/%s_*.qm，该组文字保持原文", qUtf8Printable(_language.name()), qUtf8Printable(source.directory), qUtf8Printable(source.prefix));
        delete translator;
        return;
    }
    if (!QCoreApplication::installTranslator(translator))
    {
        qWarning("安装翻译失败: %s", qUtf8Printable(translator->filePath()));
        delete translator;
        return;
    }
    source.translator = translator;
}
