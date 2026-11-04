#ifndef SHADWIDGET_H
#define SHADWIDGET_H

#include <QByteArray>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QCloseEvent;
class QPaintEvent;

// 独立窗口：标题栏与边框和 ShadWindow 相同（共用同一实现），但不占用布局，可以直接 setLayout 或在子类中建立布局
// 标题栏固定在窗口顶部，窗口的上边距让出标题栏高度，布局内容排在标题栏下方；默认尺寸 300 × 300
// 固定尺寸直接使用 setFixedSize，之后不能拖动边缘缩放，双击标题栏也不会最大化
class ShadWidgetPrivate;

class SHADCN_EXPORT ShadWidget : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadWidget)
    Q_PROPERTY(int titleBarHeight READ getTitleBarHeight WRITE setTitleBarHeight)
    Q_PROPERTY(ShadWindowType::ButtonFlags buttonFlags READ getButtonFlags WRITE setButtonFlags)
    Q_PROPERTY(ShadWindowType::ButtonPosition buttonPosition READ getButtonPosition WRITE setButtonPosition)
    Q_PROPERTY(bool stayOnTop READ isStayOnTop WRITE setStayOnTop NOTIFY stayOnTopChanged)
    Q_PROPERTY(bool defaultCloseEnabled READ isDefaultCloseEnabled WRITE setDefaultCloseEnabled)

    public:
        // 始终作为独立窗口显示，parent 只决定所属关系与释放时机
        // 注意：在 Windows 上传入 parent 时，窗口成为父组件所在窗口的从属窗口（owned window），系统规定它始终位于该窗口之上，
        // 点击父窗口也无法把父窗口提到它前面（适合工具面板）；macOS 与 Linux 没有这个限制
        // 需要与主窗口平级、可以被主窗口遮住时不要传 parent，并自行负责释放（例如在所属对象析构时 delete），
        // 同时可以设置 Qt::WA_QuitOnClose 为 false，避免关闭主窗口后它仍阻止程序退出
        explicit ShadWidget(QWidget* parent = nullptr);
        ~ShadWidget() override;

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

        // 默认关闭行为，默认 true：点击标题栏关闭按钮或系统请求关闭时直接关闭窗口
        // 为 false 时不自动关闭，而是发出 closeRequested，由调用方决定是否调用 close()（例如先弹出确认框）；
        // 拦截的系统关闭包括 Alt+F4、任务栏与系统菜单的关闭、窗口管理器的关闭；代码调用 close() 与 QApplication::closeAllWindows 不受影响
        void setDefaultCloseEnabled(bool enabled);
        bool isDefaultCloseEnabled() const;

        // 移到所在屏幕可用区域的中央；最大化或全屏时不移动
        void moveToCenter();

    Q_SIGNALS:
        // 点击标题栏关闭按钮时总会发出，与 defaultCloseEnabled 无关
        void closeButtonClicked();
        // defaultCloseEnabled 为 false 时，点击关闭按钮或系统请求关闭都会发出，窗口保持打开
        void closeRequested();
        // 通过 setStayOnTop 或置顶按钮改变置顶状态时发出
        void stayOnTopChanged(bool stayOnTop);

    protected:
        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADWIDGET_H
