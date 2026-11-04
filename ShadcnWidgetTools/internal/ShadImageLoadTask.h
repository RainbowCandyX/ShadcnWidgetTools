#ifndef SHADIMAGELOADTASK_H
#define SHADIMAGELOADTASK_H

#include <QImage>
#include <QList>
#include <QRunnable>
#include <QString>

#include <functional>

// 在线程池中解码图片文件并生成多级缩小版本，组件库内部使用
// 结果在主线程回调，callback 需自行判断接收方是否存在、结果是否过期
class ShadImageLoadTask : public QRunnable
{
    public:
        // levels 为空表示解码失败，此时 errorString 为失败原因
        using Callback = std::function<void(const QList<QImage> &levels, const QString &errorString)>;

        ShadImageLoadTask(const QString& path, Callback callback);
        ~ShadImageLoadTask() override;

        void run() override;

        // 生成逐级减半的缩小版本，第 0 级为原图
        static QList<QImage> buildLevels(const QImage& image);

    private:
        QString _path;
        Callback _callback;
};

#endif // SHADIMAGELOADTASK_H
