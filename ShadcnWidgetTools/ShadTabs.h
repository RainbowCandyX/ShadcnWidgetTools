#ifndef SHADTABS_H
#define SHADTABS_H

#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

// 标签页，对应 shadcn Tabs（TabsList / TabsTrigger / TabsContent）：一组分层的内容，每次显示其中一个
// 标签栏与内容区之间 gap-2；横向时标签栏在上方并按内容宽度显示（w-fit），纵向时标签栏在起始侧
// 每个标签可以对应一个页面，只显示当前标签的页面，内容区高度随当前页面变化；没有页面时只作为切换控件使用
// 键盘：Tab 聚焦到当前标签，横向用左右方向键（RTL 下对调）、纵向用上下方向键移动焦点，Home / End 跳到首尾，回车或空格切换
class ShadTabsPrivate;

class SHADCN_EXPORT ShadTabs : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTabs)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(ShadTabsType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(bool activateOnFocus READ isActivateOnFocus WRITE setActivateOnFocus)

    public:
        explicit ShadTabs(QWidget* parent = nullptr);
        ~ShadTabs() override;

        // 添加标签，page 为对应的页面（可以为 nullptr），标签页接管页面的父子关系；返回标签的位置
        // 第一个添加的标签自动成为当前标签
        int addTab(const QString& text, QWidget* page = nullptr);
        int addTab(ShadIconType::IconName icon, const QString& text, QWidget* page = nullptr);
        // index 超出范围时添加到末尾
        int insertTab(int index, ShadIconType::IconName icon, const QString& text, QWidget* page = nullptr);
        // 移除标签但不删除页面，页面的父组件置空，由调用方负责释放；移除当前标签时选中同一位置的标签
        void removeTab(int index);
        int count() const;
        QWidget* widget(int index) const;
        // 页面不属于任何标签时返回 -1
        int indexOf(QWidget* page) const;

        void setTabText(int index, const QString& text);
        QString tabText(int index) const;
        // 标签文字前的图标，size-4，与文字之间 gap-1.5
        void setTabIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName tabIcon(int index) const;
        // 禁用的标签为 50% 不透明度，不能点击，键盘导航时跳过
        void setTabEnabled(int index, bool enabled);
        bool isTabEnabled(int index) const;

        // -1 表示没有当前标签（没有标签时）
        void setCurrentIndex(int index);
        int currentIndex() const;

        // Default 为 bg-muted 分段样式，Line 为透明背景加指示线
        void setVariant(ShadTabsType::Variant variant);
        ShadTabsType::Variant getVariant() const;

        // 默认 Qt::Horizontal；Qt::Vertical 时标签纵向排列在起始侧，标签占满标签栏宽度，内容靠起始侧
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        // 开启后方向键移动焦点时同时切换标签；默认关闭，方向键只移动焦点，回车或空格切换（与 Base UI 一致）
        void setActivateOnFocus(bool activateOnFocus);
        bool isActivateOnFocus() const;

    Q_SIGNALS:
        // 当前标签变化时发出，包括点击、键盘与调用 setCurrentIndex
        void currentChanged(int index);
};

#endif // SHADTABS_H
