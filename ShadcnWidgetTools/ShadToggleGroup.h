#ifndef SHADTOGGLEGROUP_H
#define SHADTOGGLEGROUP_H

#include <QList>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;
class ShadToggle;

// 切换按钮组：按钮横向拼接，只有两端圆角；Single 模式最多选中一项且可再次点击取消，Multiple 模式可多选
class ShadToggleGroupPrivate;

class SHADCN_EXPORT ShadToggleGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadToggleGroup)
    Q_PROPERTY(ShadToggleGroupType::SelectionMode selectionMode READ getSelectionMode WRITE setSelectionMode)
    Q_PROPERTY(ShadToggleType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(ShadToggleType::Size toggleSize READ getToggleSize WRITE setToggleSize)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

    public:
        explicit ShadToggleGroup(QWidget* parent = nullptr);
        explicit ShadToggleGroup(ShadToggleGroupType::SelectionMode selectionMode, QWidget* parent = nullptr);
        ~ShadToggleGroup() override;

        ShadToggle* addItem(const QString& text);
        ShadToggle* addItem(ShadIconType::IconName icon, const QString& text = QString());
        ShadToggle* itemAt(int index) const;
        int count() const;

        // 切换到 Single 时如有多项选中，只保留第一项
        void setSelectionMode(ShadToggleGroupType::SelectionMode selectionMode);
        ShadToggleGroupType::SelectionMode getSelectionMode() const;

        // 统一设置所有按钮的变体与尺寸
        void setVariant(ShadToggleType::Variant variant);
        ShadToggleType::Variant getVariant() const;
        void setToggleSize(ShadToggleType::Size size);
        ShadToggleType::Size getToggleSize() const;

        // 只选中 index 一项，传入 -1 清空选中；读取时返回第一个选中项，没有时返回 -1
        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        // Single 模式下选中某项会取消其他项
        void setItemChecked(int index, bool checked);
        bool isItemChecked(int index) const;
        QList<int> getCheckedIndexes() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // Single 模式下选中项变化时发出，-1 表示未选中
        void currentIndexChanged(int index);
        // 任一按钮选中状态变化时发出
        void checkedIndexesChanged();

    protected:
        bool event(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 按顺序排列可见按钮，并刷新每个按钮的组内位置
        void layoutItems();
};

#endif // SHADTOGGLEGROUP_H
