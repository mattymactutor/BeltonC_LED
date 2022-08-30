#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "styles.h"

#include <iostream>
using namespace std;
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

QStringList strPositions;
QStringList comPORTS;
bool firstLoad = true;

enum ENC_MODE{
    SCROLL_HIGHLIGHT,
    CHANGE_VALUE
};

ENC_MODE encMode = SCROLL_HIGHLIGHT;

/*
 * TODO FUTURE
 * Instead of RGB sliders use a progress bar that fills up with red based on how far you drag it
 *
 *
 * TODO OFFLINE
 *
 */

QList<QFrame*> borders;
QList<QSlider*> sliders;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    arduino = new QSerialPort(this);
        serialBuffer = "";

        //bool arduino_is_available = false;
        //adding a blank to the com ports combo box fires off
        //cmbCOM selected index change, that function does a check for first load which tries
        //to connect to the last com port selected
        ui->cmbCOM->addItem("");

   //put borders on list to make highlight/selection easier
    borders.push_back(ui->brdRed);
    borders.push_back(ui->brdGreen);
    borders.push_back(ui->brdBlue);
    borders.push_back(ui->brdTungsten);
    borders.push_back(ui->brdDaylight);
    borders.push_back(ui->brdBrightness);

    //put sliders on list to make changing values easier
    sliders.push_back(ui->sldRed);
    sliders.push_back(ui->sldGreen);
    sliders.push_back(ui->sldBlue);
    sliders.push_back(ui->sldTungsten);
    sliders.push_back(ui->sldDaylight);
    sliders.push_back(ui->sldBrightness);

    highlightSlider(curHighlight);

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


     if (in == "enc"){

         if (encMode == SCROLL_HIGHLIGHT){
             //if we are scrolling now a click should select whatever is highlighted
             selectSlider(curHighlight);
             curSelection = curHighlight;
             encMode = CHANGE_VALUE;
         } else if (encMode == CHANGE_VALUE){
             highlightSlider(curSelection);
             curHighlight = curSelection;
             curSelection = -1; //deselect
             encMode = SCROLL_HIGHLIGHT;
         }

     } else if (in == "enc+"){
         if (encMode == SCROLL_HIGHLIGHT){
             curHighlight++;
             if (curHighlight > 5) { curHighlight = 0;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[curSelection]->value();
             sliders[curSelection]->setValue(curVal + 3);
         }
     } else if (in == "enc-"){
         if (encMode == SCROLL_HIGHLIGHT){
             curHighlight--;
             if (curHighlight < 0) { curHighlight = 5;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[curSelection]->value();
             sliders[curSelection]->setValue(curVal - 3);
         }
     }

     if (this->cursor() != Qt::ArrowCursor){
        this->setCursor(Qt::ArrowCursor);
     }


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
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "r:" + QString::number(value);   
    sendArduinoCmd(msg);

    if (curSelection != RED){
       curSelection = RED;
       curHighlight = RED;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
}


void MainWindow::on_sldGreen_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "g:" + QString::number(value);
    sendArduinoCmd(msg);

    if (curSelection != GREEN){
       curSelection = GREEN;
       curHighlight = GREEN;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
}


void MainWindow::on_sldBlue_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "b:" + QString::number(value);
    sendArduinoCmd(msg);
    if (curSelection != BLUE){
       curSelection = BLUE;
       curHighlight = BLUE;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
}




void MainWindow::on_sldTungsten_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "t:" + QString::number(value);
    sendArduinoCmd(msg);
    if (curSelection != TUNGSTEN){
       curSelection = TUNGSTEN;
       curHighlight = TUNGSTEN;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
}




void MainWindow::on_sldDaylight_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "d:" + QString::number(value);
    sendArduinoCmd(msg);
    if (curSelection != DAYLIGHT){
       curSelection = DAYLIGHT;
       curHighlight = DAYLIGHT;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
}


void MainWindow::on_sldBrightness_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    QString msg = "B:" + QString::number(value);
    sendArduinoCmd(msg);
    if (curSelection != BRIGHTNESS){
       curSelection = BRIGHTNESS;
       curHighlight = BRIGHTNESS;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }


}


void MainWindow::on_sldRed_sliderPressed()
{

}

void MainWindow::highlightSlider(int sld){
    //make them all white
    for(int i = 0; i < borders.size(); i++){
        if (i != sld)
            borders[i]->setStyleSheet(STYLE_WHITE_BORDER);
    }
    //now highlight one of them
    borders[sld]->setStyleSheet(STYLE_HIGHLIGHT_BORDER);
}

void MainWindow::selectSlider(int sld){

    //slider should already be highlighted so just turn that one selected
    borders[sld]->setStyleSheet(STYLE_SELECTED_BORDER);
}

