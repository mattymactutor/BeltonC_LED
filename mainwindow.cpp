#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "styles.h"

#include <iostream>
#include <fstream>
#include <QThread>
using namespace std;
#include <QMessageBox>
#include <unistd.h>
#include "variables.h"

/*
 * TODO FUTURE

 *
 *
 * TODO OFFLINE
 *
 */


//RGB SLIDERS
#define RED 0
#define GREEN 1
#define BLUE 2
#define TUNGSTEN 3
#define DAYLIGHT 4
#define BRIGHTNESS 5
//HSV SLIDERS
#define HUE 0
#define SAT 1
#define VAL 2
//GRADIENT SLIDERS
#define STARTHUE 0
#define STARTSAT 1
#define STARTVAL 2
#define ENDHUE 3
#define ENDSAT 4
#define ENDVAL 5


#define SLD_CHANGE_THRESHOLD 2
enum ENC_MODE{
    SCROLL_HIGHLIGHT,
    CHANGE_VALUE
};
ENC_MODE encMode = SCROLL_HIGHLIGHT;

#define NUM_MODES 3
enum MODE{
    RGB = 0,
    HSV = 1,
    GRADIENT = 2
};
bool configDataSentToArduino[NUM_MODES];


//make a strucutre to keep track of the settings
struct CONFIG{
    int h, s, v;
    //rgb daylight tungsten brightness
    int r, g, b, d, t, B;
    int sh,ss,sv,eh,es,ev;
    //possibly add ability to change the num of leds? for the gradient.
    enum MODE mode;
};

CONFIG config;


QList<QList<QFrame*>> borders;
QList<QList<QSlider*>> sliders;
USB_Comm * arduino;

QStringList strPositions;
QStringList comPORTS;
bool firstLoad = true;
bool isArduinoConnected = false;
const QString FILE_NAME = "config.txt";


Ui::MainWindow * ui2;
void changeBackgroundOfHSVSlider(QSlider *, int, int,int);
void changeBackgroundOfRGBSlider(QSlider * sld, int R, int G, int B, double percent);
void changeGradientStyleBasedOnSliders(Ui::MainWindow * ui);


void parseUSBCmd(string cmd){
    cout << "USB IN: " << cmd << endl;
    ui2->lblIncMsg->setText(QString::fromStdString(cmd));
    if (cmd == "ready"){
        arduino->setConnected(true);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui2 = ui;

    //mark that no page data has been sent to arduino yet
    for(int i = 0 ; i < NUM_MODES; i++){
        configDataSentToArduino[i] = false;
    }

    string port = "/dev/ttyACM0";
    arduino = new USB_Comm(port);
    arduino->setParseFunc(parseUSBCmd);
    usleep(2000*1000);

    while(!arduino->isConnected()){
        arduino->sendMsg("init");
        usleep(500*1000);
    }
    ui->lblStatus->setText("Connected to Arduino on " + QString::fromStdString(port));
    loadDataFromFile();


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

    highlightSlider(curHighlight);

    //during testing it would open on tab 3 but the Arduino would not be in that mode because we didnt click and send the message
    //set it to be the first tab that way you have to click to go to another tab with actually sends the mode number
    ui->tabWidget->setCurrentIndex(static_cast<int>(config.mode));
    arduino->sendMsg("m:" + to_string(static_cast<int>(config.mode)));
    sendInitData(config.mode);

}

void MainWindow::sendInitData(int idx){
    MODE m = static_cast<MODE>(idx);
    if (m == MODE::RGB){
    //rgb
    sendArduinoCmd("r:" + QString::number(config.r));
    sendArduinoCmd("g:" + QString::number(config.g));
    sendArduinoCmd("b:" + QString::number(config.b));
    sendArduinoCmd("d:" + QString::number(config.d));
    sendArduinoCmd("t:" + QString::number(config.t));
    sendArduinoCmd("B:" + QString::number(config.B));
    loadSliders(idx);

    } else if (m == MODE::HSV){
    //hsv
    sendArduinoCmd("h:" + QString::number(config.h));
    sendArduinoCmd("s:" + QString::number(config.s));
    sendArduinoCmd("v:" + QString::number(config.v));
    loadSliders(idx);
    } else if (m == MODE::GRADIENT){
    //gradient
    sendArduinoCmd("sh:" + QString::number(config.sh));
    sendArduinoCmd("ss:" + QString::number(config.ss));
    sendArduinoCmd("sv:" + QString::number(config.sv));
    sendArduinoCmd("eh:" + QString::number(config.eh));
    sendArduinoCmd("es:" + QString::number(config.es));
    sendArduinoCmd("ev:" + QString::number(config.ev));
    loadSliders(idx);
    }
    //mark that we sent data for that page
    configDataSentToArduino[static_cast<int>(m)] = true;
}

//call this after everything has been drawn
void MainWindow::loadSliders(int idx){
    MODE m = static_cast<MODE>(idx);
    if (m == MODE::RGB){
    //move all sliders down and show no background
    changeBackgroundOfRGBSlider(ui->sldRed, 255,0,0, config.r / 255.0);
    changeBackgroundOfRGBSlider(ui->sldGreen,0,255,0, config.g / 255.0);
    changeBackgroundOfRGBSlider(ui->sldBlue, 0,0,255, config.b / 255.0);
    changeBackgroundOfRGBSlider(ui->sldDaylight, 198, 243, 255, config.t / 255.0);
    changeBackgroundOfRGBSlider(ui->sldTungsten, 255,212,125 , config.d / 255.0);
    changeBackgroundOfRGBSlider(ui->sldBrightness,194,194,194, config.B / 255.0);
    ui->sldRed->setValue(config.r);
    ui->sldGreen->setValue(config.g);
    ui->sldBlue->setValue(config.b);
    ui->sldDaylight->setValue(config.d);
    ui->sldTungsten->setValue(config.t);
    ui->sldBrightness->setValue(config.B);
    } else  if (m == MODE::HSV){


    //hsv
    //QTCreator is 0 to 359 but arduino is 0 to 254
    double zeroTo1 = config.h / 255.0;
    int value = (int)(zeroTo1 * 359.0);
    ui->sldHue->setValue(value);
    ui->sldSat->setValue(config.s);
    ui->sldVal->setValue(config.v);
    changeBackgroundOfHSVSlider(ui->sldHue, config.h,180,255);
    changeBackgroundOfHSVSlider(ui->sldSat, config.h,config.s,255);
    changeBackgroundOfHSVSlider(ui->sldVal,config.h,config.s,config.v);    

    } else  if (m == MODE::GRADIENT){

    //gradient
        //convert the hue
        double zeroTo1 = config.sh / 255.0;
        int value = (int)(zeroTo1 * 359.0);
    ui->sldStartHue->setValue(value);
    ui->sldStartSat->setValue(config.ss);
    ui->sldStartVal->setValue(config.sv);
    changeBackgroundOfHSVSlider(ui->sldStartHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldStartSat, value,config.ss,255);
    changeBackgroundOfHSVSlider(ui->sldStartVal, value, config.ss, config.sv);

    zeroTo1 = config.eh / 255.0;
    value = (int)(zeroTo1 * 359.0);
    ui->sldEndHue->setValue(value);
    ui->sldEndSat->setValue(config.es);
    ui->sldEndVal->setValue(config.sv);
    changeBackgroundOfHSVSlider(ui->sldEndHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldEndSat, value,config.es,255);
    changeBackgroundOfHSVSlider(ui->sldEndVal,value,config.es,config.ev);

    changeGradientStyleBasedOnSliders(ui);
    }

   // sendArduinoCmd("lastload");
}

/*
void MainWindow::parseArduinoCmd(string in){

     cout << "USB IN: " << in << endl;
     ui->lblIncMsg->setText(QString::fromStdString(in));

        if (in == "ready"){
            isArduinoConnected = true;
           // loadSliders();
        } else if (in == "lastload"){
            cout << " LOAD FINISHED"<<endl;
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
             if (curHighlight > sliders[config.mode].size()-1) { curHighlight = 0;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[config.mode][curSelection]->value();
             sliders[config.mode][curSelection]->setValue(curVal + 3);
         }
     } else if (in == "enc-"){
         if (encMode == SCROLL_HIGHLIGHT){
             curHighlight--;
             if (curHighlight < 0) { curHighlight = sliders[config.mode].size()-1;}
             highlightSlider(curHighlight);
         } else if (encMode == CHANGE_VALUE){
             int curVal = sliders[config.mode][curSelection]->value();
             sliders[config.mode][curSelection]->setValue(curVal - 3);
         }
     }

     if (this->cursor() != Qt::ArrowCursor){
        this->setCursor(Qt::ArrowCursor);
     }


}*/

void MainWindow::sendArduinoCmd(QString in){
  arduino->sendMsg(in.toStdString());
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
    /*if (firstLoad){
        loadCOMPorts();
        return;
    }

    if (arduino->isOpen()){
        arduino->close();
    }
    //+1 because the first option should be blank
    QString port = comPORTS[index-1];
    cout << "trying to connect to Arduino on port : " << port.toStdString() << endl;
    initArduino(port);*/

}

void MainWindow::on_sldRed_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

    changeBackgroundOfRGBSlider(ui->sldRed, 200, 0, 0 , value/255.0);
    if (curSelection != RED){
       curSelection = RED;
       curHighlight = RED;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    //check if we've moved the slider enough to send the new data
    if (abs(config.r - value) > SLD_CHANGE_THRESHOLD )
    {
        config.r = value;
        saveDataToFile();
        QString msg = "r:" + QString::number(value);
        sendArduinoCmd(msg);
    }



}


void MainWindow::on_sldGreen_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

    changeBackgroundOfRGBSlider(ui->sldGreen, 0, 200, 0 , value/255.0);
    if (curSelection != GREEN){
       curSelection = GREEN;
       curHighlight = GREEN;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    if (abs(config.g - value) > SLD_CHANGE_THRESHOLD )
    {
    config.g = value;
    saveDataToFile();
    QString msg = "g:" + QString::number(value);
    sendArduinoCmd(msg);
    }
}


void MainWindow::on_sldBlue_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

     changeBackgroundOfRGBSlider(ui->sldBlue, 0, 0, 200, value/255.0);
    if (curSelection != BLUE){
       curSelection = BLUE;
       curHighlight = BLUE;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    if (abs(config.b - value) > SLD_CHANGE_THRESHOLD )
    {
    config.b = value;
    saveDataToFile();
    QString msg = "b:" + QString::number(value);
    sendArduinoCmd(msg);
    }
}




void MainWindow::on_sldTungsten_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

    changeBackgroundOfRGBSlider(ui->sldTungsten, 255,212,125 , value/255.0);
    if (curSelection != TUNGSTEN){
       curSelection = TUNGSTEN;
       curHighlight = TUNGSTEN;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }
    if (abs(config.t - value) > SLD_CHANGE_THRESHOLD )
    {
        config.t = value;
        saveDataToFile();
        QString msg = "t:" + QString::number(value);
        sendArduinoCmd(msg);
    }
}




void MainWindow::on_sldDaylight_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

    changeBackgroundOfRGBSlider(ui->sldDaylight, 198, 243, 255 , value/255.0);
    if (curSelection != DAYLIGHT){
       curSelection = DAYLIGHT;
       curHighlight = DAYLIGHT;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    if (abs(config.d - value) > SLD_CHANGE_THRESHOLD )
    {
        config.d = value;
        saveDataToFile();
        QString msg = "d:" + QString::number(value);
        sendArduinoCmd(msg);
    }

}


void MainWindow::on_sldBrightness_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100

    changeBackgroundOfRGBSlider(ui->sldBrightness, 194,194,194, value/255.0);
    if (curSelection != BRIGHTNESS){
       curSelection = BRIGHTNESS;
       curHighlight = BRIGHTNESS;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    if (abs(config.B - value) > SLD_CHANGE_THRESHOLD )
    {
    config.B = value;
    saveDataToFile();
    QString msg = "B:" + QString::number(value);
    sendArduinoCmd(msg);
    }

}


void MainWindow::highlightSlider(int sld){

    if (sld == -1){
        cout << "No slider to highlight!" <<endl;
        return;
    }
    //make them all white
    for(int i = 0; i < borders[config.mode].size(); i++){
        if (i != sld)
            borders[config.mode][i]->setStyleSheet(STYLE_WHITE_BORDER);
    }
    //now highlight one of them
    borders[config.mode][sld]->setStyleSheet(STYLE_HIGHLIGHT_BORDER);
}

void MainWindow::selectSlider(int sld){

    if (sld == -1){
        cout << "No slider to highlight!" <<endl;
        return;
    }
    //slider should already be highlighted so just turn that one selected
    borders[config.mode][sld]->setStyleSheet(STYLE_SELECTED_BORDER);
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


    if (abs(config.h - value) > SLD_CHANGE_THRESHOLD )
    {
        //QTCreator is 0 to 359 but arduino is 0 to 254
        double zeroTo1 = value / 359.0;
        value = (int)(zeroTo1 * 254);
    QString msg = "h:" + QString::number(value);
    sendArduinoCmd(msg);
    config.h = value;
    saveDataToFile();
    cout << "SLIDER HUE CHANGED" << endl;
    }


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

    if (abs(config.s - value) > SLD_CHANGE_THRESHOLD )
    {
    QString msg = "s:" + QString::number(value);
    sendArduinoCmd(msg);
    config.s = value;
    saveDataToFile();
    }
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
    if (abs(config.v - value) > SLD_CHANGE_THRESHOLD )
    {
    QString msg = "v:" + QString::number(value);
    sendArduinoCmd(msg);
    config.v = value;
    saveDataToFile();
    }
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

    if (abs(config.sh - value) > SLD_CHANGE_THRESHOLD )
    {
    QString msg = "sh:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
    config.sh = value;
    saveDataToFile();
    }
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

    if (abs(config.ss - value) > SLD_CHANGE_THRESHOLD )
    {
    QString msg = "ss:" + QString::number(value);
    sendArduinoCmd(msg);
    changeGradientStyleBasedOnSliders(ui);
    config.ss = value;
    saveDataToFile();
    }
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

    if (abs(config.sv - value) > SLD_CHANGE_THRESHOLD )
    {
        QString msg = "sv:" + QString::number(value);
        sendArduinoCmd(msg);
        changeGradientStyleBasedOnSliders(ui);
        config.sv = value;
        saveDataToFile();
    }
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
    if (abs(config.eh - value) > SLD_CHANGE_THRESHOLD )
    {
        QString msg = "eh:" + QString::number(value);
        sendArduinoCmd(msg);
        changeGradientStyleBasedOnSliders(ui);
        config.eh = value;
        saveDataToFile();
    }
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

    if (abs(config.es - value) > SLD_CHANGE_THRESHOLD )
    {
        QString msg = "es:" + QString::number(value);
        sendArduinoCmd(msg);
        changeGradientStyleBasedOnSliders(ui);
        config.es = value;
        saveDataToFile();
    }
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

    if (abs(config.ev - value) > SLD_CHANGE_THRESHOLD )
    {
        QString msg = "ev:" + QString::number(value);
        sendArduinoCmd(msg);
        changeGradientStyleBasedOnSliders(ui);
        config.ev = value;
        saveDataToFile();
    }
}

void MainWindow::saveDataToFile(){
    ofstream outfile(FILE_NAME.toStdString().c_str());
    if (!outfile.is_open()){
        //TODO make this a message box
        cout << "Error could not open the settings file to save data!" <<endl;
        exit(1);
    }

    //first line is rgb
    outfile << config.r << " " << config.g << " " << config.b << " " << config.d << " " << config.t << " " << config.B << endl;
    //second is HSV
    outfile << config.h << " " << config.s << " " << config.v << endl;
    //third line is gradient
    outfile << config.sh << " " << config.ss << " " << config.sv << " " << config.eh << " " << config.es << " " << config.ev << endl;
    //last line is the mode
    outfile << static_cast<int>(config.mode) << endl;
    outfile.close();
}

void MainWindow::loadDataFromFile(){
    ifstream infile(FILE_NAME.toStdString().c_str());
    if (!infile.is_open()){
        //TODO make this a message box
        cout << "Created new settings file!" <<endl;
        CONFIG newConf = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,static_cast<MODE>(0)};
        config = newConf;
        saveDataToFile();
    }

    //first line is rgb
    infile >> config.r >> config.g >> config.b >> config.d >> config.t >> config.B;
    //second is HSV
    infile >> config.h >> config.s >> config.v;
    //third line is gradient
    infile >> config.sh >> config.ss >> config.sv >> config.eh >> config.es >> config.ev;
    //last line is the mode
    int tempMode;
    infile >> tempMode;
    config.mode = static_cast<MODE>(tempMode);
    infile.close();

    cout << "---Loaded config settings---" << endl;
    cout << "R,G,B,D,T,B:\t" << config.r << " "  << config.g  << " "  << config.b <<  " "  << config.d <<  " "  << config.t << " "  << config.B <<endl;
    cout << "H,S,V:\t" << config.h << " "  << config.s  << " "  << config.v << endl;
    cout << "GRADIENT" << endl;
    cout << "\tSTART--> H,S,V:\t" << config.sh << " "  << config.ss  << " "  << config.sv << endl;
    cout << "\tEND  --> H,S,V:\t" << config.eh << " "  << config.es  << " "  << config.ev << endl;
    cout << "MODE: " << static_cast<int> (config.mode) << endl;
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    cout << "CLICKED: " << index << endl;
    config.mode = static_cast<MODE>(index);
    curHighlight = 0;
    curSelection = -1;
    highlightSlider(curHighlight);
    saveDataToFile();

    QString msg = "m:" + QString::number(index);
    sendArduinoCmd(msg);

    if (!configDataSentToArduino[index]){
        sendInitData(config.mode);
    }
}
