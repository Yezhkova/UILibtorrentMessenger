#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "SessionWrapper.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, &MainWindow::signal, ui->feed, &QTextEdit::append);
    LOG("MainWindow started\n");
}

MainWindow::~MainWindow()
{
    delete ui;
}

boost::asio::ip::udp::endpoint MainWindow::uep(char const* ip, uint16_t port)
{
    libtorrent::error_code ec;
    boost::asio::ip::udp::endpoint ret( boost::asio::ip::make_address(ip, ec), port );
    assert(!ec);
    return ret;
}

std::shared_ptr<SessionWrapper> MainWindow::createLtSessionPtr( const std::string& addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate )
{
    LOG("");
    return std::make_shared<SessionWrapper>( addressAndPort, delegate );
};

void MainWindow::onMessage( const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint )
{
    emit signal(QString::fromStdString(messageText));
}

void MainWindow::on_connectButton_clicked()
{
    LOG("Setting up client...\n");
    m_username = ui->myUsernameLabel->text().toStdString();
    m_address = ui->addressLabel->text().toStdString();
    m_port = ui->portLabel->text().toUShort();

    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
    ui->sendButton->setEnabled(true);

    m_sessionWrapperPtr = createLtSessionPtr(m_address +":"+ std::to_string(m_port), std::make_shared<MainWindow> (this));
    if(m_sessionWrapperPtr != nullptr)
    {
        LOG(m_username << ": Session created\n");
        m_sessionWrapperPtr->start();
        LOG(m_username << ": Session started\n");
    }
    else
    {
        LOG(m_username << ": Session failed\n");
    }
}

void MainWindow::on_disconnectButton_clicked()
{
    LOG("Client "<<m_username<<" killed\n");
    this->close();
    exit(0);
}

void MainWindow::on_sendButton_clicked()
{
    std::string messageText = ui->typeMessage->toPlainText().toStdString();
    if(!messageText.empty())
    {
        boost::asio::ip::udp::endpoint senderEndpoint = uep("192.168.1.20", 11102); // ?????????????????????????????????????
        LOG(m_username << " is sending message \"" << messageText << "\"\n");
        m_sessionWrapperPtr->sendMessage(senderEndpoint, messageText);
        ui->typeMessage->clear();
    }
}




