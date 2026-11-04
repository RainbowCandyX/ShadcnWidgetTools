#ifndef SHADFIELD_H
#define SHADFIELD_H

#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class ShadLabel;

// 表单项，对应 shadcn Field / FieldLabel / FieldContent / FieldTitle / FieldDescription / FieldError：标签、控件、说明与错误信息
// 标签为 ShadLabel 并关联控件：点击标签聚焦或切换控件，控件禁用时标签半透明
// 竖排（默认）：标签、控件、说明、错误依次排列，间距 gap-3；文字与控件的可视边缘对齐（扣除控件四周 3px 焦点环边距）
// 横排：标签、说明与错误组成一列（gap-1），与控件左右并排；没有说明时垂直居中，有说明时顶部对齐
// 错误态（setInvalid）：标签为 destructive 色，并同步设置控件的 invalid 属性（ShadInput、ShadCheckbox、ShadComboBox 等）
// 选择卡片（setChoiceCard）：整个表单项为带边框的圆角卡片（p-3），悬停 bg-muted/50，控件选中时 bg-primary/5 border-primary/30，
// 控件通过键盘获得焦点时卡片显示焦点环，点击卡片任意位置切换控件；用于复选框、单选框与开关
class ShadFieldPrivate;

class SHADCN_EXPORT ShadField : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadField)
    Q_PROPERTY(QString label READ getLabel WRITE setLabel)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadFieldType::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(ShadFieldType::ControlPosition controlPosition READ getControlPosition WRITE setControlPosition)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)
    Q_PROPERTY(bool choiceCard READ isChoiceCard WRITE setChoiceCard)

    public:
        explicit ShadField(QWidget* parent = nullptr);
        ShadField(const QString& label, QWidget* control, QWidget* parent = nullptr);
        ~ShadField() override;

        // 标签文字，为空时不显示
        void setLabel(const QString& label);
        QString getLabel() const;
        ShadLabel* labelWidget() const;

        // 表单控件，表单项接管父子关系；替换时旧控件被删除，传入 nullptr 移除
        void setControl(QWidget* control);
        QWidget* getControl() const;

        // 说明文字，text-sm text-muted-foreground，自动换行；为空时不显示
        void setDescription(const QString& description);
        QString getDescription() const;

        // 错误信息，对应 FieldError 的 errors：重复的消息只显示一次，多条时显示为列表；传入空列表清除
        void setErrors(const QStringList& errors);
        QStringList getErrors() const;
        void setError(const QString& error);
        void clearErrors();

        // 错误态，对应 data-invalid
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 默认 Vertical
        void setOrientation(ShadFieldType::Orientation orientation);
        ShadFieldType::Orientation getOrientation() const;
        // 横排时控件所在的一侧，默认 End
        void setControlPosition(ShadFieldType::ControlPosition position);
        ShadFieldType::ControlPosition getControlPosition() const;

        void setChoiceCard(bool choiceCard);
        bool isChoiceCard() const;

        bool eventFilter(QObject* watched, QEvent* event) override;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADFIELD_H
