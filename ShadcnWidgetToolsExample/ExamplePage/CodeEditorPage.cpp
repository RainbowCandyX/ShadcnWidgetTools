#include "CodeEditorPage.h"

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadCodeEditor.h"
#include "ShadComboBox.h"

namespace
{
    // 编辑器可视高度 240px，另加上下焦点环边距
    constexpr int kEditorHeight = 240 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kSmallEditorHeight = 160 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kLanguageComboBoxWidth = 200;

    struct LanguageSample
    {
        QString name;
        QString language;
        QString code;
    };

    ShadCodeEditor* createEditor(const QString& code, const QString& language, int height, QWidget* parent)
    {
        ShadCodeEditor* editor = new ShadCodeEditor(code, parent);
        editor->setLanguage(language);
        editor->setFixedHeight(height);
        return editor;
    }

    QList<LanguageSample> languageSamples()
    {
        return {
            {
                QStringLiteral("C++"), QStringLiteral("cpp"), QStringLiteral("#include <QApplication>\n"
                    "#include \"MainWindow.h\"\n"
                    "\n"
                    "int main(int argc, char* argv[])\n"
                    "{\n"
                    "    QApplication app(argc, argv);\n"
                    "    MainWindow window;\n"
                    "    window.show();\n"
                    "    return app.exec();\n"
                    "}\n")
            },
            {
                QStringLiteral("Python"), QStringLiteral("python"),
                QCoreApplication::translate("CodeEditorPage", "from dataclasses import dataclass\n\n\n@dataclass\nclass User:\n    name: str\n    age: int = 0\n\n    def greet(self) -> str:\n        \"\"\"返回问候语\"\"\"\n        return f\"Hello, {self.name}!\"\n")
            },
            {
                QStringLiteral("TypeScript"), QStringLiteral("ts"),
                QCoreApplication::translate("CodeEditorPage", "type Status = \"idle\" | \"loading\" | \"done\"\n\n/* 读取用户列表 */\nexport async function fetchUsers(page = 1): Promise<User[]> {\n  const response = await fetch(`/api/users?page=${page}`)\n  if (!response.ok) {\n    throw new Error(\"请求失败\")\n  }\n  return response.json()\n}\n")
            },
            {
                QStringLiteral("JSON"), QStringLiteral("json"), QStringLiteral("{\n"
                    "  \"name\": \"shadcn-widget-tools\",\n"
                    "  \"version\": \"0.1.0\",\n"
                    "  \"private\": true,\n"
                    "  \"dependencies\": {\n"
                    "    \"qt\": \"6.8.0\"\n"
                    "  }\n"
                    "}\n")
            },
            {
                QStringLiteral("CSS"), QStringLiteral("css"), QStringLiteral(".card {\n"
                    "  display: flex;\n"
                    "  gap: 1rem;\n"
                    "  border-radius: 0.5rem;\n"
                    "  color: #0a0a0a;\n"
                    "}\n"
                    "\n"
                    ".card:hover {\n"
                    "  background-color: rgb(245 245 245);\n"
                    "}\n")
            },
            {
                QStringLiteral("Bash"), QStringLiteral("bash"),
                QCoreApplication::translate("CodeEditorPage", "#!/usr/bin/env bash\nset -euo pipefail\n\nBUILD_DIR=\"build\"\ncmake -S . -B \"$BUILD_DIR\" -DCMAKE_BUILD_TYPE=Release\ncmake --build \"$BUILD_DIR\" --parallel\necho \"构建完成\"\n")
            },
        };
    }
} // namespace

CodeEditorPage::CodeEditorPage(QWidget* parent)
    : ExampleBasePage([] { return tr("代码编辑器"); }, [] { return tr("ShadCodeEditor：带行号栏、语法高亮与当前行高亮的代码编辑器，继承 QPlainTextEdit。"); }, parent)
{
    // 编辑器内容可由用户修改，只在创建时设置一次，切换语言时不刷新
    const QList<LanguageSample> samples = languageSamples();

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, []
    {
        return tr("setLanguage 设置高亮语言，编辑时实时着色，光标所在行高亮");
    });
    addStretchItem(basicCard, createEditor(samples.first().code, samples.first().language, kEditorHeight, this));

    ShadCard* languageCard = addSection([] { return tr("切换语言"); }, []
    {
        return tr("支持 cpp、python、typescript、javascript、json、css、bash");
    });
    ShadComboBox* languageComboBox = new ShadComboBox(this);
    QStringList names;
    for (const LanguageSample& sample : samples)
    {
        names.append(sample.name);
    }
    languageComboBox->addItems(names);
    languageComboBox->setFixedWidth(kLanguageComboBoxWidth);
    addItem(languageCard, languageComboBox);
    ShadCodeEditor* languageEditor = createEditor(samples.first().code, samples.first().language, kEditorHeight, this);
    addStretchItem(languageCard, languageEditor);
    // 切换时按当前语言重新生成示例代码
    connect(languageComboBox, &ShadComboBox::currentIndexChanged, languageEditor, [languageEditor](int index)
    {
        const QList<LanguageSample> currentSamples = languageSamples();
        if (index < 0 || index >= currentSamples.size())
        {
            return;
        }
        languageEditor->setLanguage(currentSamples.at(index).language);
        languageEditor->setPlainText(currentSamples.at(index).code);
    });

    ShadCard* readOnlyCard = addSection([] { return tr("只读"); }, []
    {
        return tr("setReadOnly(true) 后只能选择与复制，不显示当前行高亮");
    });
    ShadCodeEditor* readOnlyEditor = createEditor(samples.at(3).code, samples.at(3).language, kSmallEditorHeight, this);
    readOnlyEditor->setReadOnly(true);
    addStretchItem(readOnlyCard, readOnlyEditor);

    ShadCard* numbersCard = addSection([] { return tr("隐藏行号"); }, []
    {
        return tr("setLineNumbersVisible(false) 隐藏行号栏");
    });
    ShadCodeEditor* noNumbersEditor = createEditor(samples.last().code, samples.last().language, kSmallEditorHeight, this);
    noNumbersEditor->setLineNumbersVisible(false);
    addStretchItem(numbersCard, noNumbersEditor);

    ShadCard* tabCard = addSection([] { return tr("Tab 宽度"); }, [] { return tr("setTabSize(2) 让 Tab 显示为 2 个空格宽"); });
    const QString tabCode = QStringLiteral("function clamp(value, min, max) {\n"
        "\tif (value < min) {\n"
        "\t\treturn min\n"
        "\t}\n"
        "\treturn Math.min(value, max)\n"
        "}\n");
    ShadCodeEditor* tabEditor = createEditor(tabCode, QStringLiteral("js"), kSmallEditorHeight, this);
    tabEditor->setTabSize(2);
    addStretchItem(tabCard, tabEditor);

    ShadCard* invalidCard = addSection([] { return tr("错误态"); }, []
    {
        return tr("setInvalid(true) 边框与焦点环变为 destructive 色");
    });
    const QString invalidJson = QStringLiteral("{\n"
        "  \"name\": \"my-app\",\n"
        "  \"version\": 0.1.0\n"
        "}\n");
    ShadCodeEditor* invalidEditor = createEditor(invalidJson, QStringLiteral("json"), kSmallEditorHeight, this);
    invalidEditor->setInvalid(true);
    addStretchItem(invalidCard, invalidEditor);
}

CodeEditorPage::~CodeEditorPage()
{
}
