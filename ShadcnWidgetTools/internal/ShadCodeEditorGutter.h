#ifndef SHADCODEEDITORGUTTER_H
#define SHADCODEEDITORGUTTER_H

#include <QWidget>

class QPaintEvent;
class ShadCodeEditor;

// ShadCodeEditor 的行号栏：行号右对齐，当前行号高亮
class ShadCodeEditorGutter : public QWidget
{
    public:
        explicit ShadCodeEditorGutter(ShadCodeEditor* editor);
        ~ShadCodeEditorGutter() override;

        int gutterWidth() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        // min-w-6 与 gap-3
        static constexpr int kLineNumberMinWidth = 24;
        static constexpr int kLineNumberGap = 12;

        ShadCodeEditor* _editor{nullptr};
};

#endif // SHADCODEEDITORGUTTER_H
