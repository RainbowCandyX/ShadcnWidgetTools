#ifndef SHADPASSKEYPRIVATE_H
#define SHADPASSKEYPRIVATE_H

#include <QByteArray>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include <functional>
#include <memory>

#include "ShadDef.h"
#include "ShadProperty.h"

class QTimer;
class ShadPasskey;
class ShadPasskeyBackend;

class ShadPasskeyPrivate
{
    SHAD_D_CREATE(ShadPasskey)

    public:
        ShadPasskeyPrivate();
        ~ShadPasskeyPrivate();

        static constexpr int kSimulationDelay = 800;
        static constexpr int kSimulatedIdSize = 16;
        static constexpr int kSimulatedSignatureSize = 64;

        QWidget* resolveWindow() const;
        void setBusy(bool busy);
        void start(int timeout);
        void finish();
        void fail(ShadPasskeyType::Error error, const QString& detail);
        void failLater(ShadPasskeyType::Error error);
        void simulate(const std::function<void()>& deliver);
        static QString errorMessage(ShadPasskeyType::Error error, const QString& detail);
        static QByteArray randomBytes(int size);
        static QByteArray clientDataJson(const char* type, const QByteArray& challenge, const QString& rpId);
        static QByteArray authenticatorData(const QString& rpId, quint32 signCount);

        std::unique_ptr<ShadPasskeyBackend> _backend;
        QTimer* _timeoutTimer{nullptr};
        QTimer* _simulationTimer{nullptr};
        std::function<void()> _simulationResult;
        QPointer<QWidget> _window;
        QByteArray _simulatedCredentialId;
        QByteArray _simulatedUserId;
        quint32 _simulatedSignCount{0};
        bool _isBusy{false};
        bool _isTimedOut{false};
        bool _isSimulated{false};
};

#endif
