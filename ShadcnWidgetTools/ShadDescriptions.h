#ifndef SHADDESCRIPTIONS_H
#define SHADDESCRIPTIONS_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;

// 描述列表：键值对按列网格排列，内容可以是文字或任意组件，结构参考 Ant Design Descriptions
// 支持跨列、带边框与纵向排列，高度随宽度变化
class ShadDescriptionsPrivate;

class SHADCN_EXPORT ShadDescriptions : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDescriptions)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(int columnCount READ getColumnCount WRITE setColumnCount)
    Q_PROPERTY(ShadDescriptionsType::Layout itemLayout READ getItemLayout WRITE setItemLayout)
    Q_PROPERTY(bool bordered READ isBordered WRITE setBordered)
    Q_PROPERTY(ShadDescriptionsType::Size descriptionsSize READ getDescriptionsSize WRITE setDescriptionsSize)
    Q_PROPERTY(bool colonVisible READ isColonVisible WRITE setColonVisible)

    public:
        explicit ShadDescriptions(QWidget* parent = nullptr);
        explicit ShadDescriptions(const QString& title, QWidget* parent = nullptr);
        ~ShadDescriptions() override;

        // 标题，为空且没有操作区时隐藏标题行
        void setTitle(const QString& title);
        QString getTitle() const;

        // 标题行结束侧的操作区，替换时删除旧组件
        void setExtra(QWidget* widget);
        QWidget* getExtra() const;

        // 列数，默认 3
        void setColumnCount(int count);
        int getColumnCount() const;

        // 标签与内容的排列方向，默认 Horizontal
        void setItemLayout(ShadDescriptionsType::Layout layout);
        ShadDescriptionsType::Layout getItemLayout() const;

        // 带边框样式，默认 false
        void setBordered(bool bordered);
        bool isBordered() const;

        // 尺寸，默认 Medium
        void setDescriptionsSize(ShadDescriptionsType::Size size);
        ShadDescriptionsType::Size getDescriptionsSize() const;

        // 标签后显示冒号，默认 false
        void setColonVisible(bool visible);
        bool isColonVisible() const;

        // 添加文字项，返回序号
        int addItem(const QString& label, const QString& text, int span = 1);
        // 添加组件项，组件归描述列表所有，返回序号
        int addItem(const QString& label, QWidget* content, int span = 1);

        void setItemLabel(int index, const QString& label);
        QString getItemLabel(int index) const;
        // 只对文字项有效
        void setItemText(int index, const QString& text);
        QString getItemText(int index) const;
        // 组件项的内容组件，文字项返回 nullptr
        QWidget* getItemWidget(int index) const;
        void setItemSpan(int index, int span);
        int getItemSpan(int index) const;

        // 删除第 index 项及其内容组件
        void removeItem(int index);
        // 删除全部项，标题与操作区保留
        void clear();
        int count() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDESCRIPTIONS_H
