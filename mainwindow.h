#pragma once
//#define MAINWINDOW_H

#include <QMainWindow>
#include "SessionWrapper.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow, public SessionWrapperDelegate
{
    Q_OBJECT

private:
    Ui::MainWindow *                ui;
    std::shared_ptr<SessionWrapper> m_sessionWrapperPtr = nullptr;

    std::string                     m_username;
    std::string                     m_address;
    uint16_t                        m_port;

private:
    boost::asio::ip::udp::endpoint uep(char const* ip, uint16_t port);
    std::shared_ptr<SessionWrapper> createLtSessionPtr( const std::string& addressAndPort, std::shared_ptr<SessionWrapperDelegate> delegate );

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    virtual void onMessage(const std::string& messageText, boost::asio::ip::udp::endpoint senderEndpoint ) override;

private slots:
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    void on_sendButton_clicked();

signals:
    void signal(QString temp);
};
