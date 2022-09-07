#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "styles.h"

#include <iostream>
using namespace std;
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <unistd.h>

QStringList strPositions;
QStringList comPORTS;
bool firstLoad = true;
bool isArduinoConnected = false;

enum ENC_MODE{
    SCROLL_HIGHLIGHT,
    CHANGE_VALUE
};
ENC_MODE encMode = SCROLL_HIGHLIGHT;

#define NUM_MODES 3
enum MODE{
    RGB,
    HSV,
    GRADIENT
};

MODE mode = RGB;

/*
 * TODO FUTURE
 * Instead of RGB sliders use a progress bar that fills up with red based on how far you drag it
 *
 *
 * TODO OFFLINE
 *
 */

void changeBackgroundOfHSVSlider(QSlider *, int, int,int);
void changeBackgroundOfRGBSlider(QSlider * sld, int R, int G, int B, double percent);

QList<QList<QFrame*>> borders;
QList<QList<QSlider*>> sliders;

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

    //A 2D array of the boarders and sliders is used to highlight certain sliders and also to change their value
     //Each row is a mode, and the column is a list of pointers to the objects on the page for that mode
   //put borders on list to make highlight/selection easier
    QList<QFrame*> tempBorders;
    tempBorders.push_back(ui->brdRed);
    tempBorders.push_back(ui->brdGreen);
    tempBorders.push_back(ui->brdBlue);
    tempBorders.push_back(ui->brdTungsten);
    tempBorders.push_back(ui->brdDaylight);
    tempBorders.push_back(ui->brdBrightness);
    borders.push_back(tempBorders);
    QList<QSlider *> tempSliders;
    //put sliders on list to make changing values easier
    tempSliders.push_back(ui->sldRed);
    tempSliders.push_back(ui->sldGreen);
    tempSliders.push_back(ui->sldBlue);
    tempSliders.push_back(ui->sldTungsten);
    tempSliders.push_back(ui->sldDaylight);
    tempSliders.push_back(ui->sldBrightness);
    sliders.push_back(tempSliders);
    //move all sliders down and show no background
    changeBackgroundOfRGBSlider(ui->sldRed, 0,0,0,0.0);
    changeBackgroundOfRGBSlider(ui->sldGreen,0,0,0,0);
    changeBackgroundOfRGBSlider(ui->sldBlue, 0,0,0,0);
    changeBackgroundOfRGBSlider(ui->sldDaylight, 0,0,0,0);
    changeBackgroundOfRGBSlider(ui->sldTungsten, 0,0,0,0);
    changeBackgroundOfRGBSlider(ui->sldBrightness, 0,0,0,0);
    sendArduinoCmd("r:0");
    sendArduinoCmd("g:0");
    sendArduinoCmd("b:0");

    //setup HSV page
    tempBorders.clear();
    tempBorders.push_back(ui->brdHue);
    tempBorders.push_back(ui->brdSat);
    tempBorders.push_back(ui->brdVal);
    borders.push_back(tempBorders);
    tempSliders.clear();
    tempSliders.push_back(ui->sldHue);
    tempSliders.push_back(ui->sldSat);
    tempSliders.push_back(ui->sldVal);
    sliders.push_back(tempSliders);
    // put sliders to 0
    ui->sldHue->setValue(0);
    ui->sldSat->setValue(0);
    ui->sldVal->setValue(0);
    changeBackgroundOfHSVSlider(ui->sldHue, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldSat, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldVal,0,0,0);
    //set value in arduino
    sendArduinoCmd("h:0");
    sendArduinoCmd("s:0");
    sendArduinoCmd("v:0");

    //setup gradient page
    tempBorders.clear();
    tempBorders.push_back(ui->brdStartHue);
    tempBorders.push_back(ui->brdStartSat);
    tempBorders.push_back(ui->brdStartVal);
    tempBorders.push_back(ui->brdEndHue);
    tempBorders.push_back(ui->brdEndSat);
    tempBorders.push_back(ui->brdEndVal);
    borders.push_back(tempBorders);
    tempSliders.clear();
    tempSliders.push_back(ui->sldStartHue);
    tempSliders.push_back(ui->sldStartSat);
    tempSliders.push_back(ui->sldStartVal);
    tempSliders.push_back(ui->sldEndHue);
    tempSliders.push_back(ui->sldEndSat);
    tempSliders.push_back(ui->sldEndVal);
    sliders.push_back(tempSliders);
    //put sliders to 0
    ui->sldStartHue->setValue(0);
    ui->sldStartSat->setValue(0);
    ui->sldStartVal->setValue(0);
    ui->sldEndHue->setValue(0);
    ui->sldEndSat->setValue(0);
    ui->sldEndVal->setValue(0);
    changeBackgroundOfHSVSlider(ui->sldStartHue, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldStartSat, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldStartVal,0,0,0);
    changeBackgroundOfHSVSlider(ui->sldEndHue, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldEndSat, 0,0,0);
    changeBackgroundOfHSVSlider(ui->sldEndVal,0,0,0);
    sendArduinoCmd("sh:0");
    sendArduinoCmd("ss:0");
    sendArduinoCmd("sv:0");
    sendArduinoCmd("eh:0");
    sendArduinoCmd("es:0");
    sendArduinoCmd("ev:0");

    highlightSlider(curHighlight);   

    //during testing it would open on tab 3 but the Arduino would not be in that mode because we didnt click and send the message
    //set it to be the first tab that way you have to click to go to another tab with actually sends the mode number
    ui->tabWidget->setCurrentIndex(0);


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
   // while (!isArduinoConnected){
        sendArduinoCmd("init");
        //sleep(1);
   // }
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

        if (in == "ready"){
            isArduinoConnected = true;
        } else if (in == "enc"){

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
             if (curHighlight > sliders[mode].size()-1) { curHighlight = 0;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[mode][curSelection]->value();
             sliders[mode][curSelection]->setValue(curVal + 3);
         }
     } else if (in == "enc-"){
         if (encMode == SCROLL_HIGHLIGHT){
             curHighlight--;
             if (curHighlight < 0) { curHighlight = sliders[mode].size()-1;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[mode][curSelection]->value();
             sliders[mode][curSelection]->setValue(curVal - 3);
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
    arduino->close();
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
     changeBackgroundOfRGBSlider(ui->sldRed, 200, 0, 0 , value/255.0);
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
    changeBackgroundOfRGBSlider(ui->sldGreen, 0, 200, 0 , value/255.0);
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
     changeBackgroundOfRGBSlider(ui->sldBlue, 0, 0, 200, value/255.0);
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
    changeBackgroundOfRGBSlider(ui->sldTungsten, 255,212,125 , value/255.0);
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
    changeBackgroundOfRGBSlider(ui->sldDaylight, 198, 243, 255 , value/255.0);
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
    changeBackgroundOfRGBSlider(ui->sldBrightness, 194,194,194, value/255.0);
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
    for(int i = 0; i < borders[mode].size(); i++){
        if (i != sld)
            borders[mode][i]->setStyleSheet(STYLE_WHITE_BORDER);
    }
    //now highlight one of them
    borders[mode][sld]->setStyleSheet(STYLE_HIGHLIGHT_BORDER);
}

void MainWindow::selectSlider(int sld){

    //slider should already be highlighted so just turn that one selected
    borders[mode][sld]->setStyleSheet(STYLE_SELECTED_BORDER);
}


void MainWindow::on_tabWidget_currentChanged(int index)
{
    qDebug() << "Clicked index " << index;
    QString msg = "m:" + QString::number(index);
    sendArduinoCmd(msg);
    mode = static_cast<MODE>(index);
    curHighlight = 0;
    curSelection = -1;
    highlightSlider(curHighlight);
}

void changeBackgroundOfHSVSlider(QSlider * sld,int h, int s, int v){

    //The HUE color in QTCREATOR doesnt match what the LEDS actually look like so create an offset to make them match
    //On Arduino RED-> PURPLE is 0 to 235
    //On QTCreator RED -> PURPLE is 0 to 255
    //the value that comes in is in Arduino mode so convert
   // double zeroTo1 = h / 235.0;
    //h = zeroTo1 * 358;

    QString color = "hsva(" + QString::number(h) + "," + QString::number(s) + "," + QString::number(v) + ",100%)";

         QString style = "QSlider::groove:vertical { \
            width: 80px;  \
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 " + color + " stop:1 " + color + "); \
            margin: 2px 0; \
        } \
        \
        QSlider::handle:vertical { \
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f); \
            height: 25px; \
            margin: -2px 0; \
            border-radius: 8px; \
        }";
           sld->setStyleSheet(style);
}

void changeBackgroundOfRGBSlider(QSlider * sld, int R, int G, int B, double percent){
    //percent should be inverted
    percent = 1 - percent;
    double percOffset = percent + 0.02;
    QString color = "rgb(" +  QString::number(R) + "," + QString::number(G) + "," + QString::number(B) + ")";
    QString style = " QSlider::groove:vertical { \
        width: 80px; \
        background: qlineargradient(x1:0, y1: " + QString::number(percent) +", x2: 0, y2: " + QString::number(percOffset) +
        ", stop:0 rgb(255, 255,255), stop:1 " + color + "); \
        margin: 2px 0; \
    } \
    \
    QSlider::handle:vertical { \
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f); \
        height: 25px; \
        margin: -2px 0;  \
        border-radius: 8px; \
    } ";

      sld->setStyleSheet(style);
}






void MainWindow::on_sldHue_valueChanged(int value)
{
    if (curSelection != HUE){
       curSelection = HUE;
       curHighlight = HUE;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    changeBackgroundOfHSVSlider(ui->sldHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldSat, value,ui->sldSat->value(),255);
    changeBackgroundOfHSVSlider(ui->sldVal, value,ui->sldSat->value(),ui->sldVal->value());
    //QTCreator is 0 to 359 but arduino is 0 to 254
    double zeroTo1 = value / 359.0;
    value = (int)(zeroTo1 * 254);
    QString msg = "h:" + QString::number(value);

    sendArduinoCmd(msg);

    //temp
    ui->lblHue->setText(QString::number(value));
}


void MainWindow::on_sldSat_valueChanged(int value)
{
    if (curSelection != SAT){
       curSelection = SAT;
       curHighlight = SAT;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldSat, ui->sldHue->value(),value,255);
    changeBackgroundOfHSVSlider(ui->sldVal, ui->sldHue->value(),value,ui->sldVal->value());
    QString msg = "s:" + QString::number(value);
    sendArduinoCmd(msg);
}


void MainWindow::on_sldVal_valueChanged(int value)
{
    if (curSelection != VAL){
       curSelection = VAL;
       curHighlight = VAL;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldVal, ui->sldHue->value(),ui->sldSat->value(),value);
    QString msg = "v:" + QString::number(value);
    sendArduinoCmd(msg);
}

void changeGradientStyleBasedOnSliders(Ui::MainWindow * ui){
  int sh = ui->sldStartHue->value();
  int ss = ui->sldStartSat->value();
  int sv = ui->sldStartVal->value();
  int eh = ui->sldEndHue->value();
  int es = ui->sldEndSat->value();
  int ev = ui->sldEndVal->value();
QString startHSV = "hsv(" + QString::number(sh) + "," + QString::number(ss) + "," + QString::number(sv) + ")";
QString endHSV = "hsv(" + QString::number(eh) + "," + QString::number(es) + "," + QString::number(ev) + ")";
QString style = " background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 " + startHSV + ", stop:1 " + endHSV + ");";
ui->frameGradient->setStyleSheet(style);
}

void MainWindow::on_sldStartHue_valueChanged(int value)
{

    if (curSelection != STARTHUE){
       curSelection = STARTHUE;
       curHighlight = STARTHUE;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    changeBackgroundOfHSVSlider(ui->sldStartHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldStartSat, value,ui->sldStartSat->value(),255);
    changeBackgroundOfHSVSlider(ui->sldStartVal, value,ui->sldStartSat->value(),ui->sldStartVal->value());
    //QTCreator is 0 to 359 but arduino is 0 to 254
    double zeroTo1 = value / 359.0;
    value = (int)(zeroTo1 * 254);
    QString msg = "sh:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}


void MainWindow::on_sldStartSat_valueChanged(int value)
{

    if (curSelection != STARTSAT){
       curSelection = STARTSAT;
       curHighlight = STARTSAT;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldStartSat, ui->sldStartHue->value(),value,255);
    changeBackgroundOfHSVSlider(ui->sldStartVal, ui->sldStartHue->value(),value,ui->sldStartVal->value());

    QString msg = "ss:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}


void MainWindow::on_sldStartVal_valueChanged(int value)
{
    if (curSelection != STARTVAL){
       curSelection = STARTVAL;
       curHighlight = STARTVAL;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldStartVal, ui->sldStartHue->value(),ui->sldStartSat->value(),value);

    QString msg = "sv:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}


void MainWindow::on_sldEndHue_valueChanged(int value)
{
    if (curSelection != ENDHUE){
       curSelection = ENDHUE;
       curHighlight = ENDHUE;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    changeBackgroundOfHSVSlider(ui->sldEndHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldEndSat, value,ui->sldEndSat->value(),255);
    changeBackgroundOfHSVSlider(ui->sldEndVal, value,ui->sldEndSat->value(),ui->sldEndVal->value());

    //QTCreator is 0 to 359 but arduino is 0 to 254
    double zeroTo1 = value / 359.0;
    value = (int)(zeroTo1 * 254);
    QString msg = "eh:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}


void MainWindow::on_sldEndSat_valueChanged(int value)
{
    if (curSelection != ENDSAT){
       curSelection = ENDSAT;
       curHighlight = ENDSAT;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldEndSat, ui->sldEndHue->value(),value,255);
    changeBackgroundOfHSVSlider(ui->sldEndVal, ui->sldEndHue->value(),value,ui->sldEndVal->value());

    QString msg = "es:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}


void MainWindow::on_sldEndVal_valueChanged(int value)
{
    if (curSelection != ENDVAL){
       curSelection = ENDVAL;
       curHighlight = ENDVAL;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    changeBackgroundOfHSVSlider(ui->sldEndVal, ui->sldEndHue->value(),ui->sldEndSat->value(),value);
    QString msg = "ev:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
}

