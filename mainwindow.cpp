#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectButton_clicked()
{
    std::string username = ui->myUsernameLabel->text().toStdString();
    std::string address = ui->addressLabel->text().toStdString();
    std::string port = ui->portLabel->text().toStdString();

}

