#ifndef SHADTIMELINE_H
#define SHADTIMELINE_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QHideEvent;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QVBoxLayout;
class ShadTimeline;
class ShadTimelinePrivate;

// 时间轴中的一个节点：圆点（或图标）、时间、标题、描述与可放任意组件的内容区
// 圆点 size-2.5，默认 primary 色；设置图标后以 size-4 图标代替圆点，LoaderCircle 自动旋转
// 时间 text-xs muted-foreground，标题 text-sm font-medium，描述 text-sm muted-foreground，均自动换行；为空的部分不占空间
// 节点由 ShadTimeline 创建和排列，位置与连线由时间轴决定
class ShadTimelineItemPrivate;

class SHADCN_EXPORT ShadTimelineItem : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTimelineItem)
    // 时间轴设置节点的位置、连线与进行中样式
    friend class ShadTimeline;
    friend class ShadTimelinePrivate;
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString time READ getTime WRITE setTime)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadThemeType::ThemeColor color READ getColor WRITE setColor)

    public:
        explicit ShadTimelineItem(QWidget* parent = nullptr);
        ShadTimelineItem(const QString& title, const QString& time = QString(), const QString& description = QString(), QWidget* parent = nullptr);
        ~ShadTimelineItem() override;

        void setTitle(const QString& title);
        QString getTitle() const;

        // 时间文字，Start / End 模式显示在标题上方，Alternate 模式显示在轴的另一侧
        void setTime(const QString& time);
        QString getTime() const;

        void setDescription(const QString& description);
        QString getDescription() const;

        // 代替圆点的图标，None 表示显示圆点
        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        // 圆点或图标的颜色，默认 Primary
        void setColor(ShadThemeType::ThemeColor color);
        ShadThemeType::ThemeColor getColor() const;

        // 内容区布局，位于描述下方，没有可见内容时不占空间
        QVBoxLayout* contentLayout() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

// 时间轴，对应 Ant Design Timeline：节点纵向排列，圆点之间以 2px border 色竖线相连
// setMode 决定轴的位置，setReverse 倒序显示；setPending 在末尾追加一个旋转图标的进行中节点，与它相连的一段为虚线
class SHADCN_EXPORT ShadTimeline : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTimeline)
    Q_PROPERTY(ShadTimelineType::Mode mode READ getMode WRITE setMode)
    Q_PROPERTY(bool reverse READ isReverse WRITE setReverse)
    Q_PROPERTY(QString pending READ getPending WRITE setPending)

    public:
        explicit ShadTimeline(QWidget* parent = nullptr);
        ~ShadTimeline() override;

        // 添加节点，返回的节点由时间轴管理
        ShadTimelineItem* addItem(const QString& title, const QString& time = QString(), const QString& description = QString());
        ShadTimelineItem* itemAt(int index) const;
        int count() const;
        // 删除全部节点，不包括进行中节点
        void clear();

        // 时间轴位置，默认 Start
        void setMode(ShadTimelineType::Mode mode);
        ShadTimelineType::Mode getMode() const;

        // 倒序显示，进行中节点随之移到最前
        void setReverse(bool reverse);
        bool isReverse() const;

        // 进行中节点的文字，为空时不显示进行中节点
        void setPending(const QString& text);
        QString getPending() const;
        // 进行中节点，未设置文字时为 nullptr
        ShadTimelineItem* pendingItem() const;
};

#endif // SHADTIMELINE_H
