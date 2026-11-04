#ifndef SHADWINDOW_H
#define SHADWINDOW_H

#include <QByteArray>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QPaintEvent;

// 不使用系统标题栏的顶层窗口：标题栏、最小化 / 最大化 / 关闭按钮均为自绘，背景与边框跟随主题
// Windows 11 保留系统边框以获得阴影、圆角与 Aero Snap；Windows 10 与 macOS / Linux 使用 FramelessWindowHint 与透明背景，自绘圆角与四周 1px 边框，最大化或全屏时为直角且去掉边框
// Windows 10 的贴靠、系统菜单与缩放仍由系统处理；系统不给透明窗口画阴影，阴影由一个跟随窗口的独立窗口绘制
// 边框逻辑与 ShadDialog 共用，两者外观一致
// ShadWindow 自身已有布局（标题栏 + 内容区），内容通过 setCentralWidget 设置，不要再对 ShadWindow 调用 setLayout
class ShadWindowPrivate;

class SHADCN_EXPORT ShadWindow : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadWindow)
    Q_PROPERTY(int titleBarHeight READ getTitleBarHeight WRITE setTitleBarHeight)
    Q_PROPERTY(ShadWindowType::ButtonFlags buttonFlags READ getButtonFlags WRITE setButtonFlags)
    Q_PROPERTY(ShadWindowType::ButtonPosition buttonPosition READ getButtonPosition WRITE setButtonPosition)
    Q_PROPERTY(bool stayOnTop READ isStayOnTop WRITE setStayOnTop NOTIFY stayOnTopChanged)

    public:
        // 始终作为独立窗口显示，parent 只决定所属关系与释放时机
        // 注意：在 Windows 上传入 parent 时窗口始终位于父组件所在窗口之上（从属窗口），需要平级时不要传 parent，详见 ShadWidget
        explicit ShadWindow(QWidget* parent = nullptr);
        ~ShadWindow() override;

        // 替换标题栏下方的内容区组件，旧组件会被删除，传 nullptr 只移除
        void setCentralWidget(QWidget* widget);
        QWidget* getCentralWidget() const;

        // 替换标题栏自定义区域的组件，旧组件会被删除，传 nullptr 只移除；该组件占据的区域不能拖动窗口
        void setTitleBarWidget(ShadWindowType::TitleBarArea area, QWidget* widget);
        QWidget* getTitleBarWidget(ShadWindowType::TitleBarArea area) const;

        // 标题栏高度，默认 40px
        void setTitleBarHeight(int height);
        int getTitleBarHeight() const;

        // 显示哪些窗口按钮，默认显示最小化、最大化与关闭；隐藏最大化按钮后双击标题栏也不再最大化
        // 加入 StayOnTopButton 显示置顶按钮（默认隐藏），点击切换 stayOnTop
        void setButtonFlags(ShadWindowType::ButtonFlags buttonFlags);
        ShadWindowType::ButtonFlags getButtonFlags() const;

        void setButtonPosition(ShadWindowType::ButtonPosition position);
        ShadWindowType::ButtonPosition getButtonPosition() const;

        // 窗口保持在其他窗口之上（Qt::WindowStaysOnTopHint）；显示期间修改直接作用于现有原生窗口，不会隐藏或重建窗口
        void setStayOnTop(bool stayOnTop);
        bool isStayOnTop() const;

    Q_SIGNALS:
        // 通过 setStayOnTop 或置顶按钮改变置顶状态时发出
        void stayOnTopChanged(bool stayOnTop);

    protected:
        bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADWINDOW_H
