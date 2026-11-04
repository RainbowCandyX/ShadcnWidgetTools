#ifndef SHADBIOMETRIC_H
#define SHADBIOMETRIC_H

#include <QObject>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;
class ShadBiometricPrivate;

class SHADCN_EXPORT ShadBiometric : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadBiometric)
    Q_PROPERTY(ShadBiometricType::Policy policy READ getPolicy WRITE setPolicy)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

    public:
        explicit ShadBiometric(QObject* parent = nullptr);
        ~ShadBiometric() override;

        static bool isSupported();

        void setPolicy(ShadBiometricType::Policy policy);
        ShadBiometricType::Policy getPolicy() const;

        ShadBiometricType::Method availableMethod() const;

        void setWindow(QWidget* window);
        QWidget* getWindow() const;

        bool isBusy() const;

        bool authenticate(const QString& reason = QString());
        void cancel();

    Q_SIGNALS:
        void succeeded();
        void failed(ShadBiometricType::Error error, const QString& message);
        void busyChanged(bool busy);
};

#endif
