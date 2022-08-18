#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
using namespace std;
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

QStringList strPositions;
QStringList comPORTS;
bool firstLoad = true;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    arduino = new QSerialPort(this);
        serialBuffer = "";

        bool arduino_is_available = false;
        //adding a blank to the com ports combo box fires off
        //cmbCOM selected index change, that function does a check for first load which tries
        //to connect to the last com port selected
        ui->cmbCOM->addItem("");

}

void MainWindow::loadCOMPorts(){
    QString arduino_uno_port_name;
    //while (ui->cmbCOM->count() > 0){
    //ui->cmbCOM->removeItem(0);
    //}
    //
    //  For each available serial port
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()){
        comPORTS.append(serialPortInfo.portName());
    }

    //show current com ports
    ui->cmbCOM->addItems(comPORTS);
    firstLoad = false;

    //just automatically try to connect to the first item in the list if there is one
    if (comPORTS.size() > 0){
        initArduino(comPORTS[0]);
    }


}

void MainWindow::initArduino(QString port){
    arduino->setPortName(port);
    if (!arduino->open(QSerialPort::ReadWrite)){
        cout << "Could not open: " << port.toStdString() << endl;
        QMessageBox::information(this, "Serial Port Error", "Could not open previous com port. Choose a COM port to connect");
        return;
    }
    arduino->setBaudRate(QSerialPort::Baud115200);
    arduino->setDataBits(QSerialPort::Data8);
    arduino->setFlowControl(QSerialPort::NoFlowControl);
    arduino->setParity(QSerialPort::NoParity);
    arduino->setStopBits(QSerialPort::OneStop);
    QObject::connect(arduino, SIGNAL(readyRead()), this, SLOT(readSerial()));
    sendArduinoCmd("init");
    ui->lblStatus->setText("Connected to Arduino on " + port);
    cout << "Connected to Arduino on " << port.toStdString() << endl;
    firstLoad = false;
}


void MainWindow::readSerial()
{
    /*
     * readyRead() doesn't guarantee that the entire message will be received all at once.
     * The message can arrive split into parts.  Need to buffer the serial data and then parse for the temperature value.
     *
     */

    //sometimes you get
    //<Hell
    //o58>
    //which means it doesnt read everything in one go

   serialData = arduino->readAll();

   //read through everything you have, start a new message at every <, parse the message as command at every >,
   //build up the string otherwise
   for(int i =0; i < serialData.size(); i++){
       if (serialData[i] == '<'){
        serialBuffer = "";
       } else if (serialData[i] == '>'){
           parseArduinoCmd(serialBuffer);
       } else {
           serialBuffer += serialData[i];
           //cout << (char) serialData[i];
       }
   }


}

void MainWindow::parseArduinoCmd(string in){

     cout << "USB IN: " << in << endl;
     ui->lblIncMsg->setText(QString::fromStdString(in));


     if (in.find("CMD HERE:") != std::string::npos){

     }

     this->setCursor(Qt::ArrowCursor);


}

void MainWindow::sendArduinoCmd(QString in){
    //in = "<" + in + ">";
    in += '\n';
    arduino->write(in.toUtf8());
    cout << "USB OUT: " << in.toStdString();
}

void MainWindow::on_btnClose_clicked()
{
    MainWindow::close();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_cmbCOM_currentIndexChanged(int index)
{
    if (firstLoad){
        loadCOMPorts();
        return;
    }

    if (arduino->isOpen()){
        arduino->close();
    }
    //+1 because the first option should be blank
    QString port = comPORTS[index-1];
    cout << "trying to connect to Arduino on port : " << port.toStdString() << endl;
    initArduino(port);


}

void MainWindow::on_sldRed_valueChanged(int value)
{
    QString msg = "r:" + QString::number(value);
    sendArduinoCmd(msg);
}

