#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "SessionWrapper.hpp"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(this, &MainWindow::signal, ui->feed, &QTextEdit::append);
    LOG("MainWindow started");
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
    LOG("Creating session pointer...");
    return std::make_shared<SessionWrapper>( addressAndPort, delegate );
};

void MainWindow::onMessage( const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint )
{
//    std::thread readingThread([&]{
//        std::string toPost = "<html><span>"+messageText+"</span></html>";
////        ui->feed->setHtml(QString::fromStdString(toPost));
//        emit signal(QString::fromStdString(toPost));
//    });
//    readingThread.join();

    std::string toPost = "<html><span>"+messageText+"</span></html>";
//    ui->feed->setHtml(QString::fromStdString(toPost));
    emit signal(QString::fromStdString(toPost));

}

void MainWindow::on_connectButton_clicked()
{
    LOG("Setting up client...");
    m_username = ui->myUsernameEdit->text().toStdString();
    m_address = ui->addressEdit->text().toStdString();
    m_port = ui->portEdit->text().toUShort();

    ui->connectButton->setEnabled(false);
    ui->disconnectButton->setEnabled(true);
    ui->sendButton->setEnabled(true);

    m_sessionWrapperPtr = createLtSessionPtr(m_address +":"+ std::to_string(m_port), shared_from_this());//////////////
    if(m_sessionWrapperPtr != nullptr)
    {
        m_sessionWrapperPtr->start();
        LOG(m_username << ": Session started");
    }
    else
    {
        LOG(m_username << ": Session failed");
    }
}

void MainWindow::on_disconnectButton_clicked()
{
    LOG("Client "<<m_username<<" killed");
    this->close();
//    exit(0);
    qApp->exit(0);
}

void MainWindow::on_sendButton_clicked()
{
    std::string messageText = ui->typeMessage->toPlainText().toStdString();
    if(!messageText.empty())
    {
        boost::asio::ip::udp::endpoint senderEndpoint = uep("192.168.1.20", 11102);
        LOG(m_username << " is sending message \"" << messageText << "\"");
        m_sessionWrapperPtr->sendMessage(senderEndpoint, messageText);
        ui->typeMessage->clear();
    }
}




