#ifndef SHADTRANSFERPRIVATE_H
#define SHADTRANSFERPRIVATE_H

#include <QList>
#include <QStringList>

#include "ShadDef.h"
#include "ShadTransfer.h"

class ShadButton;
class ShadTransferPanel;

class ShadTransferPrivate
{
    SHAD_D_CREATE(ShadTransfer)

    public:
        ShadTransferPrivate();
        ~ShadTransferPrivate();

        void refreshPanels();
        void updateButtons();
        void updateButtonIcons();
        void retranslate();
        void move(ShadTransferType::Direction direction);

        ShadTransferPanel* _sourcePanel{nullptr};
        ShadTransferPanel* _targetPanel{nullptr};
        ShadButton* _toTargetButton{nullptr};
        ShadButton* _toSourceButton{nullptr};
        QList<ShadTransferItem> _items;
        QStringList _targetKeys;
        bool _isSearchEnabled{false};
};

#endif // SHADTRANSFERPRIVATE_H
