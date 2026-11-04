#ifndef SHADTEXTAREA_H
#define SHADTEXTAREA_H

#include <QPlainTextEdit>
#include <QSize>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QResizeEvent;

class ShadTextareaPrivate;

class SHADCN_EXPORT ShadTextarea : public QPlainTextEdit
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTextarea)
    Q_PROPERTY(bool invalid READ isInvalid WRITE setInvalid NOTIFY invalidChanged)
    Q_PROPERTY(bool autoResize READ isAutoResize WRITE setAutoResize)
    Q_PROPERTY(int maximumAutoHeight READ getMaximumAutoHeight WRITE setMaximumAutoHeight)

    public:
        explicit ShadTextarea(QWidget* parent = nullptr);
        ~ShadTextarea() override;

        // 错误态，对应 aria-invalid：边框与焦点环变为 destructive 色
        void setInvalid(bool invalid);
        bool isInvalid() const;

        // 高度随内容自动增长（对应 field-sizing-content），默认开启；开启时垂直尺寸策略为 Fixed
        // 外部调用 setFixedHeight 等固定高度时以外部设置为准
        void setAutoResize(bool autoResize);
        bool isAutoResize() const;

        // 自动增长的最大可视高度（不含焦点环边距），超出后显示滚动条；0 表示不限制
        void setMaximumAutoHeight(int height);
        int getMaximumAutoHeight() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void invalidChanged();

    protected:
        // 外框绘制事件不经过 paintEvent（paintEvent 只负责视口），因此在 event 中拦截
        bool event(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADTEXTAREA_H
