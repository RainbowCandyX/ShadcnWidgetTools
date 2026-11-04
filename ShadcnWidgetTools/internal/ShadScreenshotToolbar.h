#ifndef SHADSCREENSHOTTOOLBAR_H
#define SHADSCREENSHOTTOOLBAR_H

#include <QColor>
#include <QList>
#include <QMap>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadScreenshotAnnotation.h"

class QAbstractButton;
class QButtonGroup;
class QEvent;
class QPaintEvent;
class ShadButton;
class ShadLabel;
class ShadSlider;
class ShadToggleGroup;

class ShadScreenshotToolbar : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadScreenshotToolbar(QWidget* parent);
        ~ShadScreenshotToolbar() override;

        void setTool(ShadScreenshotAnnotation::Tool tool);
        ShadScreenshotAnnotation::Tool currentTool() const;
        QColor currentColor() const;
        int currentSize() const;
        int currentMosaicBlockSize() const;
        void setUndoEnabled(bool enabled);

    Q_SIGNALS:
        void toolChanged(ShadScreenshotAnnotation::Tool tool);
        void styleChanged();
        void layoutChanged();
        void undoClicked();
        void cancelClicked();
        void saveClicked();
        void copyClicked();
        void pinClicked();
        void doneClicked();

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadButton* createButton(ShadIconType::IconName icon, ShadButtonType::Variant variant);
        void updateOptions();
        void retranslate();

        ShadToggleGroup* _toolGroup{nullptr};
        ShadLabel* _sizeCaption{nullptr};
        ShadSlider* _sizeSlider{nullptr};
        ShadLabel* _sizeLabel{nullptr};
        QWidget* _blurArea{nullptr};
        ShadLabel* _blurCaption{nullptr};
        ShadSlider* _blurSlider{nullptr};
        ShadLabel* _blurLabel{nullptr};
        int _mosaicBlockSize{10};
        QMap<ShadScreenshotAnnotation::Tool, int> _sizes;
        QWidget* _optionsRow{nullptr};
        QWidget* _colorArea{nullptr};
        QButtonGroup* _colorGroup{nullptr};
        QList<QColor> _colors;
        ShadButton* _undoButton{nullptr};
        ShadButton* _cancelButton{nullptr};
        ShadButton* _saveButton{nullptr};
        ShadButton* _copyButton{nullptr};
        ShadButton* _pinButton{nullptr};
        ShadButton* _doneButton{nullptr};
};

#endif
