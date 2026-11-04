#ifndef SHADPOPOVER_H
#define SHADPOPOVER_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QHideEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QShowEvent;
class QVBoxLayout;

// 浮层，对应 shadcn Popover：w-72 rounded-md border bg-popover p-4 shadow-md，打开时淡入并从锚点方向滑入
// 内容放入 contentLayout()；setTitle / setDescription 对应 PopoverHeader（PopoverTitle / PopoverDescription）
// 点击浮层外部或按 Esc 关闭，打开时焦点移到第一个可聚焦的子组件
class ShadPopoverPrivate;

class SHADCN_EXPORT ShadPopover : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadPopover)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(int contentWidth READ getContentWidth WRITE setContentWidth)
    Q_PROPERTY(int contentPadding READ getContentPadding WRITE setContentPadding)

    public:
        explicit ShadPopover(QWidget* parent = nullptr);
        ~ShadPopover() override;

        // 标题 font-medium 与描述 text-muted-foreground，间距 gap-1；为空时不显示对应行
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        // 浮层宽度（含边框与内边距），默认 288（w-72）
        void setContentWidth(int width);
        int getContentWidth() const;

        // 边框内的内边距，默认 16（p-4）；放入自带内边距的组件（例如 ShadCalendar，对应 p-0）时设为 0
        void setContentPadding(int padding);
        int getContentPadding() const;

        // 标题区下方的内容布局，间距默认 16（gap-4）
        QVBoxLayout* contentLayout() const;

        // 在 anchor 旁弹出，与锚点间距 4（sideOffset=4）；align 为 Start / End 时与锚点起始侧 / 结束侧对齐（RTL 下互换）
        // 浮层打开时再次点击 anchor 会关闭浮层
        void popup(QWidget* anchor, ShadPlacementType::Side side = ShadPlacementType::Bottom, ShadPlacementType::Align align = ShadPlacementType::Center);
        // 关闭时与 hide() 相同，打开时关闭、关闭时打开
        void toggle(QWidget* anchor, ShadPlacementType::Side side = ShadPlacementType::Bottom, ShadPlacementType::Align align = ShadPlacementType::Center);

        QSize sizeHint() const override;

    Q_SIGNALS:
        void aboutToShow();
        void aboutToHide();

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADPOPOVER_H
