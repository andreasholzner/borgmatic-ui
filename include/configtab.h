#ifndef BORGMATIC_UI_CONFIGTAB_H
#define BORGMATIC_UI_CONFIGTAB_H

#include <QWidget>

namespace Ui {
    class TabContent;
}

class ConfigTab : public QWidget {
Q_OBJECT

public:
    explicit ConfigTab(QWidget *parent = nullptr);

    ~ConfigTab() override;

private:
    Ui::TabContent *ui;
};

#endif //BORGMATIC_UI_CONFIGTAB_H
