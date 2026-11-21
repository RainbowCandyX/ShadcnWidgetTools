#ifndef SHADTEXTCONTEXTMENU_H
#define SHADTEXTCONTEXTMENU_H

#include <QPoint>
#include <QString>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTextEdit;

namespace ShadTextContextMenu
{
    void show(QLineEdit* edit, const QPoint& globalPos);
    void show(QPlainTextEdit* edit, const QPoint& globalPos);
    void show(QTextEdit* edit, const QPoint& globalPos, const QString& link = QString());
    void show(QLabel* label, const QPoint& globalPos);

    void install(QLineEdit* edit);
    void install(QLabel* label);
} // namespace ShadTextContextMenu

#endif // SHADTEXTCONTEXTMENU_H
