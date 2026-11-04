#ifndef SHADTOASTER_H
#define SHADTOASTER_H

#include <QString>
#include <QWidget>

#include <functional>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QEnterEvent;
class QObject;
class QPaintEvent;

// 单条 toast 的内容
struct ShadToastOptions
{
    QString title;
    QString description;
    ShadToastType::Type type{ShadToastType::Default};
    // 操作按钮文字，为空时不显示操作按钮；点击后不会自动关闭 toast
    QString actionText;
    std::function<void()> onAction;
    // 自动关闭时长（毫秒）：-1 使用 ShadToaster 的默认时长（Loading 类型默认不自动关闭），0 不自动关闭
    int duration{-1};
    bool closable{true};
};

// toast 容器，对应 shadcn Toaster：挂在宿主窗口内，按 position 在窗口的角落或边缘中间堆叠显示
// 默认收起堆叠，鼠标悬停时展开，自动关闭计时照常进行
class ShadToasterPrivate;

class SHADCN_EXPORT ShadToaster : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadToaster)
    Q_PROPERTY(ShadToastType::Position position READ getPosition WRITE setPosition)
    Q_PROPERTY(int limit READ getLimit WRITE setLimit)
    Q_PROPERTY(int duration READ getDuration WRITE setDuration)

    public:
        // host 为显示 toast 的宿主窗口（通常是主窗口），ShadToaster 会作为它的子组件并始终保持在最上层
        // 宿主有 titleBarHeight 属性时，toast 显示在自绘标题栏下方
        explicit ShadToaster(QWidget* host);
        ~ShadToaster() override;

        // widget 所在顶层窗口的 toaster，没有则新建，随窗口销毁；widget 为空时返回 nullptr
        static ShadToaster* forWindow(QWidget* widget);

        // 随时可以切换，已显示的 toast 会移动到新位置，默认 TopRight
        void setPosition(ShadToastType::Position position);
        ShadToastType::Position getPosition() const;

        // 最多同时保留的数量，超出时关闭最旧的一条；默认 0 表示不限制；收起时只显示最新的 3 层
        void setLimit(int limit);
        int getLimit() const;

        // 默认自动关闭时长（毫秒），默认 5000，0 表示不自动关闭
        void setDuration(int duration);
        int getDuration() const;

        // 返回 toast 的 id，用于 updateToast / dismiss
        int add(const QString& title, const QString& description = QString(), ShadToastType::Type type = ShadToastType::Default);
        int add(const ShadToastOptions& options);
        // 替换已有 toast 的内容并重新计时，对应 toast.promise 的 loading 转 success / error；id 不存在时返回 false
        bool updateToast(int id, const ShadToastOptions& options);
        void dismiss(int id);
        void dismissAll();
        bool contains(int id) const;
        int count() const;

    Q_SIGNALS:
        // toast 开始关闭（自动到时、点击关闭按钮、超出数量限制或调用 dismiss）时发出
        void toastClosed(int id);
        void actionTriggered(int id);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTOASTER_H
