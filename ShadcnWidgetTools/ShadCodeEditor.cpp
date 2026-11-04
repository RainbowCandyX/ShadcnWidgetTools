#include "ShadCodeEditor.h"

#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QRect>
#include <QResizeEvent>
#include <QString>
#include <QTextDocument>
#include <QtGlobal>

#include "ShadCodeEditorDocumentLayout.h"
#include "ShadCodeEditorGutter.h"
#include "ShadCodeEditorHighlighter.h"
#include "ShadCodeEditorPrivate.h"
#include "ShadCodeHighlighter.h"
#include "ShadScrollBar.h"
#include "ShadTheme.h"

ShadCodeEditor::ShadCodeEditor(QWidget* parent)
    : QPlainTextEdit(parent), d_ptr(new ShadCodeEditorPrivate())
{
    Q_D(ShadCodeEditor);
    d->q_ptr = this;
    // 行号栏必须最先创建，后续设置线框与字体会触发依赖它的回调
    d->_gutter = new ShadCodeEditorGutter(this);
    // 基类缓存了旧排版对象指针，不能对现有文档调用 setDocumentLayout，必须通过 setDocument 替换
    QTextDocument* codeDocument = new QTextDocument(this);
    d->_documentLayout = new ShadCodeEditorDocumentLayout(codeDocument);
    codeDocument->setDocumentLayout(d->_documentLayout);
    setDocument(codeDocument);
    d->_documentLayout->setPaintDevice(viewport());
    codeDocument->setDefaultFont(font());
    // 显式设置后不再继承父组件的布局方向
    setLayoutDirection(Qt::LeftToRight);
    // Plain Box 仅用于获得固定的 frameWidth，外框由 paintFrame 自绘
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(ShadCodeEditorPrivate::kFrameInset);
    viewport()->setAutoFillBackground(false);
    // 轨道两端留白以避开圆角
    ShadScrollBar* verticalBar = new ShadScrollBar(Qt::Vertical, this);
    verticalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadCodeEditorPrivate::kFrameInset);
    setVerticalScrollBar(verticalBar);
    ShadScrollBar* horizontalBar = new ShadScrollBar(Qt::Horizontal, this);
    horizontalBar->setTrackInset(shadTheme->getRadius(ShadThemeType::Medium) - ShadCodeEditorPrivate::kFrameInset);
    setHorizontalScrollBar(horizontalBar);
    setFocusPolicy(Qt::StrongFocus);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    d->_highlighter = new ShadCodeEditorHighlighter(document());
    d->applyFont();
    d->updateLineHeight();
    d->applyTabStop();
    d->applyThemeColors();
    d->updateViewportMargins();
    d->updateCurrentLine();

    connect(this, &QPlainTextEdit::blockCountChanged, this, [d]() { d->updateViewportMargins(); });
    connect(this, &QPlainTextEdit::updateRequest, this, [d](const QRect& rect, int dy)
    {
        if (dy != 0)
        {
            d->_gutter->scroll(0, dy);
            return;
        }
        d->_gutter->update(0, rect.y(), d->_gutter->width(), rect.height());
    });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [d]() { d->updateCurrentLine(); });
    connect(this, &QPlainTextEdit::selectionChanged, this, [d]() { d->updateCurrentLine(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]()
    {
        d->applyThemeColors();
        d->_highlighter->rehighlight();
        d->updateCurrentLine();
    });
}

ShadCodeEditor::ShadCodeEditor(const QString& code, QWidget* parent)
    : ShadCodeEditor(parent)
{
    setPlainText(code);
}

ShadCodeEditor::~ShadCodeEditor()
{
}

void ShadCodeEditor::setLanguage(const QString& language)
{
    Q_D(ShadCodeEditor);
    d->_language = language;
    d->_highlighter->setLanguage(ShadCodeHighlighter::languageFromName(language));
}

QString ShadCodeEditor::getLanguage() const
{
    Q_D(const ShadCodeEditor);
    return d->_language;
}

void ShadCodeEditor::setTabSize(int size)
{
    Q_D(ShadCodeEditor);
    const int tabSize = qMax(1, size);
    if (d->_tabSize == tabSize)
    {
        return;
    }
    d->_tabSize = tabSize;
    d->applyTabStop();
}

int ShadCodeEditor::getTabSize() const
{
    Q_D(const ShadCodeEditor);
    return d->_tabSize;
}

void ShadCodeEditor::setLineNumbersVisible(bool visible)
{
    Q_D(ShadCodeEditor);
    if (d->_isLineNumbersVisible == visible)
    {
        return;
    }
    d->_isLineNumbersVisible = visible;
    d->updateViewportMargins();
}

bool ShadCodeEditor::isLineNumbersVisible() const
{
    Q_D(const ShadCodeEditor);
    return d->_isLineNumbersVisible;
}

void ShadCodeEditor::setInvalid(bool invalid)
{
    Q_D(ShadCodeEditor);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadCodeEditor::isInvalid() const
{
    Q_D(const ShadCodeEditor);
    return d->_isInvalid;
}

bool ShadCodeEditor::event(QEvent* event)
{
    Q_D(ShadCodeEditor);
    if (event->type() == QEvent::Paint)
    {
        d->paintFrame();
        return true;
    }
    if (event->type() == QEvent::FontChange)
    {
        // 需先让基类把新字体应用到文档，再据此重算行高等
        const bool isHandled = QPlainTextEdit::event(event);
        d->updateLineHeight();
        d->applyTabStop();
        d->updateViewportMargins();
        return isHandled;
    }
    if (event->type() == QEvent::ReadOnlyChange)
    {
        const bool isHandled = QPlainTextEdit::event(event);
        d->updateCurrentLine();
        return isHandled;
    }
    return QPlainTextEdit::event(event);
}

void ShadCodeEditor::focusInEvent(QFocusEvent* event)
{
    QPlainTextEdit::focusInEvent(event);
    update();
}

void ShadCodeEditor::focusOutEvent(QFocusEvent* event)
{
    QPlainTextEdit::focusOutEvent(event);
    update();
}

void ShadCodeEditor::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCodeEditor);
    QPlainTextEdit::resizeEvent(event);
    d->updateGutterGeometry();
}

void ShadCodeEditor::paintEvent(QPaintEvent* event)
{
    Q_D(ShadCodeEditor);
    d->paintLineBackgrounds(event->rect());
    QPlainTextEdit::paintEvent(event);
}

void ShadCodeEditor::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadCodeEditor);
    QMouseEvent mapped(event->type(), d->mapToTextLine(event->position()), event->scenePosition(), event->globalPosition(), event->button(), event->buttons(), event->modifiers(), event->pointingDevice());
    mapped.setTimestamp(event->timestamp());
    QPlainTextEdit::mousePressEvent(&mapped);
    event->setAccepted(mapped.isAccepted());
}

void ShadCodeEditor::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadCodeEditor);
    QMouseEvent mapped(event->type(), d->mapToTextLine(event->position()), event->scenePosition(), event->globalPosition(), event->button(), event->buttons(), event->modifiers(), event->pointingDevice());
    mapped.setTimestamp(event->timestamp());
    QPlainTextEdit::mouseMoveEvent(&mapped);
    event->setAccepted(mapped.isAccepted());
}

void ShadCodeEditor::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadCodeEditor);
    QMouseEvent mapped(event->type(), d->mapToTextLine(event->position()), event->scenePosition(), event->globalPosition(), event->button(), event->buttons(), event->modifiers(), event->pointingDevice());
    mapped.setTimestamp(event->timestamp());
    QPlainTextEdit::mouseReleaseEvent(&mapped);
    event->setAccepted(mapped.isAccepted());
}

void ShadCodeEditor::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_D(ShadCodeEditor);
    QMouseEvent mapped(event->type(), d->mapToTextLine(event->position()), event->scenePosition(), event->globalPosition(), event->button(), event->buttons(), event->modifiers(), event->pointingDevice());
    mapped.setTimestamp(event->timestamp());
    QPlainTextEdit::mouseDoubleClickEvent(&mapped);
    event->setAccepted(mapped.isAccepted());
}
