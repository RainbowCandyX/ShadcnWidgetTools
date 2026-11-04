#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QStringList>

#include <functional>

#include "ExampleBasePage.h"
#include "ExampleI18n.h"
#include "ShadIconType.h"

class QGridLayout;
class QString;
class QWidget;
class ShadItem;

// 首页：仓库链接、特性与组件分类入口
class HomePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit HomePage(QWidget* parent = nullptr);
        ~HomePage() override;

        // 添加分类入口，点击行为由调用方连接 clicked；name 与 pageTitles 返回当前语言的文字，切换语言时重新调用
        ShadItem* addCategoryItem(ShadIconType::IconName icon, ExampleI18n::TextFunction name, std::function<QStringList()> pageTitles);

    private:
        QWidget* _categoryGrid{nullptr};
        QGridLayout* _categoryLayout{nullptr};
};

#endif // HOMEPAGE_H
