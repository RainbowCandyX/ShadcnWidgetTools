#ifndef MAPPAGE_H
#define MAPPAGE_H

#include "ExampleBasePage.h"

class MapPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit MapPage(QWidget* parent = nullptr);
        ~MapPage() override;
};

#endif // MAPPAGE_H
