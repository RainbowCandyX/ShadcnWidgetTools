#ifndef SHADRESIZABLE_H
#define SHADRESIZABLE_H

#include <QSplitter>

#include "ShadProperty.h"

class QSplitterHandle;

// 可拖动调整大小的面板组，对应 shadcn Resizable（ResizablePanelGroup + ResizableHandle）
// 面板直接用 addWidget / insertWidget 添加；手柄为 1px 分隔线，拖动即可调整相邻面板大小
// 默认不允许把面板拖到折叠（childrenCollapsible 为 false）
class ShadResizablePrivate;

class SHADCN_EXPORT ShadResizable : public QSplitter
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadResizable)
    Q_PROPERTY(bool gripVisible READ isGripVisible WRITE setGripVisible)

    public:
        explicit ShadResizable(QWidget* parent = nullptr);
        explicit ShadResizable(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadResizable() override;

        // 在分隔线中间显示抓手（对应 ResizableHandle 的 withHandle），默认不显示
        void setGripVisible(bool visible);
        bool isGripVisible() const;

    protected:
        QSplitterHandle* createHandle() override;
};

#endif // SHADRESIZABLE_H
