#ifndef SHADSTEPS_H
#define SHADSTEPS_H

#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 步骤条，对应 Ant Design Steps：用于向导与多步骤表单，显示每一步的序号、标题、描述与完成情况
// 指示器 size-6 圆形：已完成为 primary 底色加对勾，当前步骤为 primary 底色加序号，未开始为 muted 底色，出错为 destructive 底色加 X
// 标题 text-sm font-medium，描述 text-sm muted-foreground；步骤之间以 2px 连接线相连，已完成的一段为 primary 色
// 横向时标题与描述各占一行，空间不够时尾部省略；纵向时自动换行，连接线竖直连到下一步
// setClickable 后点击步骤或按回车、空格切换到该步，方向键移动键盘焦点，只有键盘导航时显示焦点环
class ShadStepsPrivate;

class SHADCN_EXPORT ShadSteps : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSteps)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(ShadStepsType::Status status READ getStatus WRITE setStatus)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable)

    public:
        explicit ShadSteps(QWidget* parent = nullptr);
        ~ShadSteps() override;

        // 添加一步，返回下标
        int addStep(const QString& title, const QString& description = QString());
        void setStepTitle(int index, const QString& title);
        QString getStepTitle(int index) const;
        void setStepDescription(int index, const QString& description);
        QString getStepDescription(int index) const;
        // 用图标代替序号，None 恢复显示序号；出错时仍显示 X
        void setStepIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName getStepIcon(int index) const;
        // 禁用的步骤半透明显示，不能点击切换
        void setStepDisabled(int index, bool disabled);
        bool isStepDisabled(int index) const;
        int count() const;
        // 删除全部步骤
        void clear();

        // 当前步骤，之前的步骤显示为已完成，之后的显示为未开始；没有步骤时为 -1
        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        // 当前步骤的状态，默认 Process；Error 表示当前步骤出错，Finish 表示最后一步也已完成
        void setStatus(ShadStepsType::Status status);
        ShadStepsType::Status getStatus() const;
        // 按当前步骤与状态计算的某一步的状态
        ShadStepsType::Status getStepStatus(int index) const;

        // 排列方向，默认 Qt::Horizontal
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        // 点击切换步骤，默认关闭
        void setClickable(bool clickable);
        bool isClickable() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        // 前进或后退一步，到达两端时不变
        void next();
        void previous();

    Q_SIGNALS:
        // 当前步骤变化时发出，包括点击与调用接口
        void currentIndexChanged(int index);
        // 用户点击步骤或按回车、空格时发出
        void stepClicked(int index);

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSTEPS_H
