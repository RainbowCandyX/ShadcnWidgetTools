#ifndef SHADGLOBALSHORTCUT_H
#define SHADGLOBALSHORTCUT_H

#include <QKeySequence>
#include <QObject>

#include <memory>

class ShadGlobalShortcut : public QObject
{
    Q_OBJECT

    public:
        explicit ShadGlobalShortcut(QObject* parent = nullptr);
        ~ShadGlobalShortcut() override;

        static bool isSupported();
        static void activateApplication();

        bool setShortcut(const QKeySequence& sequence);
        QKeySequence getShortcut() const;

    Q_SIGNALS:
        void activated();

    private:
        struct Impl;
        std::unique_ptr<Impl> _impl;
        QKeySequence _sequence;
};

#endif
