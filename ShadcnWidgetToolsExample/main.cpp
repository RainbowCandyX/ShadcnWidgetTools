#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "MainWindow.h"
#include "ShadI18n.h"
#include "ShadTooltip.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
#ifdef Q_OS_WIN
    // 西文使用 Segoe UI，中文回退到微软雅黑
    QFont appFont = app.font();
    appFont.setFamilies(QStringList{QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI")});
    app.setFont(appFont);
#endif
#ifdef Q_OS_WASM
    const int fontId = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/NotoSansSC-Regular.otf"));
    if (fontId >= 0)
    {
        QFont appFont = app.font();
        appFont.setFamilies(QFontDatabase::applicationFontFamilies(fontId));
        app.setFont(appFont);
    }
    else
    {
        qWarning("加载中文字体失败");
    }
#endif
    // 示例界面为中文：日历的月名、星期名与日期格式使用简体中文，不随系统语言变化；需在创建组件之前设置
    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
    // 示例自己的翻译由 qt_add_translations 嵌入资源 :/i18n，原文为简体中文；切换语言时各页面通过 ExampleI18n::bind 原地刷新文字
    shadI18n->addTranslations(QStringLiteral(":/i18n"), QStringLiteral("ShadcnWidgetToolsExample"));
    // 所有组件的 toolTip 以 shadcn 样式显示
    shadTooltip->install();
    MainWindow window;
    window.show();
    return app.exec();
}
