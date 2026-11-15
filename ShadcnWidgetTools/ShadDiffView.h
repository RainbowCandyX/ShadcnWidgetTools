#ifndef SHADDIFFVIEW_H
#define SHADDIFFVIEW_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;

// 文本差异对比：按行比较旧文本与新文本
class ShadDiffViewPrivate;

class SHADCN_EXPORT ShadDiffView : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDiffView)
    Q_PROPERTY(QString oldText READ getOldText WRITE setOldText)
    Q_PROPERTY(QString newText READ getNewText WRITE setNewText)
    Q_PROPERTY(ShadDiffViewType::Mode mode READ getMode WRITE setMode)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage)
    Q_PROPERTY(bool lineNumbersVisible READ isLineNumbersVisible WRITE setLineNumbersVisible)
    Q_PROPERTY(int contextLines READ getContextLines WRITE setContextLines)
    Q_PROPERTY(bool wordWrap READ isWordWrap WRITE setWordWrap)
    Q_PROPERTY(bool selectable READ isSelectable WRITE setSelectable)
    Q_PROPERTY(int maximumContentHeight READ getMaximumContentHeight WRITE setMaximumContentHeight)

    public:
        explicit ShadDiffView(QWidget* parent = nullptr);
        ShadDiffView(const QString& oldText, const QString& newText, QWidget* parent = nullptr);
        ~ShadDiffView() override;

        // 同时设置两段文本只比较一次
        void setTexts(const QString& oldText, const QString& newText);
        void setOldText(const QString& text);
        QString getOldText() const;
        void setNewText(const QString& text);
        QString getNewText() const;

        void setMode(ShadDiffViewType::Mode mode);
        ShadDiffViewType::Mode getMode() const;

        // 语法高亮语言，不支持时按纯文本显示
        void setLanguage(const QString& language);
        QString getLanguage() const;

        void setLineNumbersVisible(bool visible);
        bool isLineNumbersVisible() const;

        // 改动周围保留的未改动行数，其余折叠，-1 显示全部
        void setContextLines(int lines);
        int getContextLines() const;

        void setWordWrap(bool wrap);
        bool isWordWrap() const;

        void setSelectable(bool selectable);
        bool isSelectable() const;
        QString selectedText() const;

        // 内容区最大高度，0 不限制
        void setMaximumContentHeight(int height);
        int getMaximumContentHeight() const;

        int getAddedLineCount() const;
        int getRemovedLineCount() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDIFFVIEW_H
