#ifndef SHADI18NPRIVATE_H
#define SHADI18NPRIVATE_H

#include <QList>
#include <QLocale>
#include <QString>

#include "ShadProperty.h"

class QTranslator;
class ShadI18n;

class ShadI18nPrivate
{
    SHAD_D_CREATE(ShadI18n)

    public:
        ShadI18nPrivate();
        ~ShadI18nPrivate();

        // 一组翻译文件：directory 下的 prefix_<语言>.qm
        struct TranslationSource
        {
            QString directory;
            QString prefix;
            // 当前语言下安装的翻译，没有时为空
            QTranslator* translator{nullptr};
        };

        QLocale _language{QLocale::Chinese, QLocale::SimplifiedHanScript, QLocale::China};
        QList<TranslationSource> _sources;

        // 原文为简体中文，繁体中文需要单独的翻译
        static bool isSourceLanguage(const QLocale& language);
        // 卸载 source 当前的翻译，再按当前语言加载并安装
        void reloadSource(TranslationSource& source);
};

#endif // SHADI18NPRIVATE_H
