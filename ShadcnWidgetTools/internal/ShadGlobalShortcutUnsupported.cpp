#include "ShadGlobalShortcut.h"

#include <QKeySequence>
#include <QObject>

struct ShadGlobalShortcut::Impl
{
};

ShadGlobalShortcut::ShadGlobalShortcut(QObject* parent)
    : QObject(parent)
{
}

ShadGlobalShortcut::~ShadGlobalShortcut()
{
}

bool ShadGlobalShortcut::isSupported()
{
    return false;
}

void ShadGlobalShortcut::activateApplication()
{
}

bool ShadGlobalShortcut::setShortcut(const QKeySequence& sequence)
{
    return sequence.isEmpty();
}

QKeySequence ShadGlobalShortcut::getShortcut() const
{
    return _sequence;
}
