#include "configtab.h"
#include "ui_tabContent.h"

ConfigTab::ConfigTab(QWidget *parent) : QWidget(parent), ui(new Ui::TabContent) {
    ui->setupUi(this);
}

ConfigTab::~ConfigTab() {
    delete ui;
}