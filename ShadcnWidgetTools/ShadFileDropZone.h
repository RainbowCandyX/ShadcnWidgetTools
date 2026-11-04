#ifndef SHADFILEDROPZONE_H
#define SHADFILEDROPZONE_H

#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtGlobal>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 文件上传区：虚线边框卡片，中间为圆形图标、标题与描述；可把本地文件拖进来，或点击 / 按回车、空格打开系统文件对话框
// 只负责选择文件并按类型与大小检查，不做网络上传；四周预留焦点环边距，与本库其他可聚焦组件一致
class ShadFileDropZonePrivate;

class SHADCN_EXPORT ShadFileDropZone : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadFileDropZone)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(QStringList nameFilters READ getNameFilters WRITE setNameFilters)
    Q_PROPERTY(bool multipleSelection READ isMultipleSelection WRITE setMultipleSelection)
    Q_PROPERTY(qint64 maximumFileSize READ getMaximumFileSize WRITE setMaximumFileSize)

    public:
        explicit ShadFileDropZone(QWidget* parent = nullptr);
        ~ShadFileDropZone() override;

        // 默认 "拖拽文件到这里" / "或点击选择文件"，为空时不显示对应行
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        // 圆形图标区的 Lucide 图标，默认 Upload
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 与 QFileDialog 相同的过滤器格式，例如 "图片 (*.png *.jpg)"；拖入的文件按同一组通配符检查（不区分大小写）
        // 为空表示接受所有文件
        void setNameFilters(const QStringList& filters);
        QStringList getNameFilters() const;

        // 是否允许一次选择多个文件，默认 true；关闭后拖入多个文件时只取第一个
        void setMultipleSelection(bool multiple);
        bool isMultipleSelection() const;

        // 单个文件的最大字节数，默认 0 表示不限制；只读取文件大小，不读取内容
        void setMaximumFileSize(qint64 bytes);
        qint64 getMaximumFileSize() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        // 打开系统文件对话框，选择的文件同样经过类型与大小检查
        void openFileDialog();

    Q_SIGNALS:
        // 通过检查的文件绝对路径
        void filesSelected(const QStringList& paths);
        // 未通过检查的文件与原因（中文），同一原因的文件合并为一次
        void filesRejected(const QStringList& paths, const QString& reason);

    protected:
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADFILEDROPZONE_H
