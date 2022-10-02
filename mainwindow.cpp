#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "blurmode.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    BlurMode blurMode;
    blurMode.HandleMethodCall(this, 4, kInitialize, 0);
    blurMode.setColorBackground(QColor(255, 73, 242, 64));
    blurMode.HandleMethodCall(this, 4, kSetEffect, 0);
}

MainWindow::~MainWindow()
{
    delete ui;
}
