#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "styles.h"
#include "millis.h"
#include <iostream>
#include <fstream>
#include <QThread>
#include <QStringListModel>
using namespace std;
#include <QMessageBox>
#include <unistd.h>

/*
 * TODO FUTURE
Editing groups doesn't work, when you edit you shoudl send a command? the arduino needs to release those group LEDs back to the master


Groups need active boolean check mark
On each page only show the groups for that page
Only change groups on the groups
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

#define SLD_CHANGE_THRESHOLD 5
enum ENC_MODE{
    SCROLL_HIGHLIGHT,
    CHANGE_VALUE
};
ENC_MODE encMode = SCROLL_HIGHLIGHT;

#define NUM_MODES 4
#define MODE_RGB 0
#define MODE_HSV 1
#define MODE_GRADIENT 2
#define MODE_GROUPS 3
bool configDataSentToArduino[NUM_MODES];

//Save all global info
CONFIG config;
//save individual group info
QList<Group> groups;
//use models for the group dropdown boxes
QStringList groupsRGB, groupsHSV, groupsGradient;

QList<QList<QFrame*>> borders;
QList<QList<QSlider*>> sliders;
QList<QString> groupNames;
USB_Comm * arduino;

QStringList strPositions;
QStringList comPORTS;
bool firstLoad = true;
bool isArduinoConnected = false;
const QString FILE_NAME = "config.txt";
const QString FILENAME_GROUPS = "groups.txt";
int curHighlight = 0, curSelection = -1;

Ui::MainWindow * ui2;
void changeBackgroundOfHSVSlider(QSlider *, int, int,int);

void changeBackgroundOfRGBSlider(QSlider * sld, int R, int G, int B, double percent);

void changeGradientStyleBasedOnSliders(Ui::MainWindow * ui);

void highlightSlider(int sld){

    if (config.mode == MODE_GROUPS){
        cout << "Groups page no sliders" <<endl;
        return;
    }

    //make them all white
    for(int i = 0; i < borders[config.mode].size(); i++){
       borders[config.mode][i]->setStyleSheet(STYLE_WHITE_BORDER);
    }
    if (sld == -1){
        cout << "No slider to highlight!" <<endl;
        return;
    }

    //now highlight one of them
    borders[config.mode][sld]->setStyleSheet(STYLE_HIGHLIGHT_BORDER);
}

void selectSlider(int sld){

    //groups page has no sliders
    if (sld == -1 || config.mode == MODE_GROUPS){
        cout << "No slider to highlight!" <<endl;
        return;
    }
    //slider should already be highlighted so just turn that one selected
    borders[config.mode][sld]->setStyleSheet(STYLE_SELECTED_BORDER);
}


unsigned char ENC_CW = 129;
#define ENC_CCW 130
#define ENC_PUSH 131
#define ENC_LONG 132
unsigned char  INIT = 133;
void parseByteFunct(unsigned char in){
    cout << "BYTE FUNCT: " << (int) in << endl;
    if (in == INIT){
        arduino->setConnected(true);
    }  else if (in == (char)ENC_CW){
        cout << "ENC CLOCKWISE" << endl;
        if (encMode == SCROLL_HIGHLIGHT){
            curHighlight++;
            if (curHighlight > sliders[config.mode].size()-1) { curHighlight = 0;}
            highlightSlider(curHighlight);
        } else if (encMode == CHANGE_VALUE){
            int curVal = sliders[config.mode][curSelection]->value();
            sliders[config.mode][curSelection]->setValue(curVal + 3);
        }
    }
}

void parseUSBCmd(string in){
    cout << "USB IN: " << in << endl;
    ui2->lblIncMsg->setText(QString::fromStdString(in));
    if (in == "ready"){
        arduino->setConnected(true);
        cout << "Arduino is live" << endl;
    }
        else if (in == "lastload"){
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
}

void appendNum(QString& data, int n){
    //if the last char is the { then dont add a comma in front
    if (data.at(data.length()-1) == '{'){
         data.append(QString::number(n));
    } else {
       data.append("," + QString::number(n));
    }
}


//create a combo box for row of groups
QComboBox * MainWindow::createGroupCombo(int row,int idx){
    QComboBox * cmbMode = new QComboBox();
    cmbMode->addItem("RGB"); cmbMode->addItem("HSV"); cmbMode->addItem("Gradient");
    cmbMode->setCurrentIndex(idx);

    cmbMode->setStyleSheet("QComboBox QAbstractItemView { selection-background-color: rgb(0,0,127); selection-color: rgb(0, 0, 0); }");
    //connect the combo box to a lambda right here for the index changed
   /* connect(cmbMode, &QComboBox::currentIndexChanged, [this,row]( int idx){
        cout << "Combo Box Changed on row " << row << endl;
        groups[row].type = idx;
        //TODO SAVE TO FILE WHEN THE MODE CHANGES
        saveGroupsToFile();
    });*/
    return cmbMode;
}

QPushButton * MainWindow::createGroupEditButton(int row){
    QPushButton * btn = new QPushButton("EDIT");

    //connect the button to a lambda right here for the click
    connect(btn, &QPushButton::clicked, [this,row](){
        //go to the correct page based on the group
        if (groups[row].type == MODE_RGB){
            ui->tabWidget->setCurrentIndex(MODE_RGB);
            //select the right group in the combo box
            for(int i = 0; i < ui->cmbRGB_Group->count(); i++){
                cout << ui->cmbRGB_Group->itemText(i).toStdString() << endl;
                if (ui->cmbRGB_Group->itemText(i) == groups[row].name){
                    ui->cmbRGB_Group->setCurrentIndex(i);
                    break;
                }
            }
        }



    });

    return btn;
}

//define the columns
#define COL_NAME 0
#define COL_STARTLED 1
#define COL_STOPLED 2
#define COL_MODE 3
#define COL_EDITBTN 4
void MainWindow::showGroups(){

    //clear the current list
    ui->tblGroups->model()->removeRows(0, ui2->tblGroups->rowCount());

    groupsRGB.clear(); groupsRGB.append("MASTER");
    for(int i = 0; i < groups.size(); i++){
        //make a row in the table
        ui->tblGroups->insertRow(i);
        Group cur = groups[i];
        ui->tblGroups->setItem(i,COL_NAME, new QTableWidgetItem(cur.name));
        ui->tblGroups->setItem(i,COL_STARTLED, new QTableWidgetItem( QString::number(cur.startLED)));
        ui->tblGroups->setItem(i,COL_STOPLED, new QTableWidgetItem(QString::number(cur.stopLED)));
        QComboBox * cmbMode = createGroupCombo(i, cur.type);
        ui->tblGroups->setCellWidget(i,COL_MODE, cmbMode);
        QPushButton * btnEdit = createGroupEditButton(i);
        ui->tblGroups->setCellWidget(i,COL_EDITBTN, btnEdit);

        if (cur.type == MODE_RGB){
           groupsRGB.append(cur.name);
        }
    }

    //fill the combo boxes with the correct groups
    ui->cmbRGB_Group->setModel( new QStringListModel(groupsRGB));


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
    arduino->setParseByteFunc(parseByteFunct);
    usleep(2000*1000);

    while(!arduino->isConnected()){
        arduino->sendMsg("init");
        //usleep(500*1000);
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


    //You should only show groups that have been flagged with this type,
    //To change the group type you'd go to the group page
    groupsRGB.append("MASTER");


    //make a test group
   /* Group g1;
    g1.type = MODE_RGB;
    g1.startLED = 0;
    g1.stopLED = 3;
    g1.r = 255;
    g1.g = 0;
    g1.b = 0;
    g1.d = 0;
    g1.t = 0;
    g1.B = 100;
    g1.name = "Group 1";
    groups.append(g1);
    ui->cmbRGB_Group->addItem("Group 1");
*/

    //set header list
    ui->tblGroups->setHorizontalHeaderItem(COL_NAME, new QTableWidgetItem("Name"));
    ui->tblGroups->setHorizontalHeaderItem(COL_STARTLED, new QTableWidgetItem("Start LED"));
    ui->tblGroups->setHorizontalHeaderItem(COL_STOPLED, new QTableWidgetItem("Stop LED"));
    ui->tblGroups->setHorizontalHeaderItem(COL_MODE, new QTableWidgetItem("Type"));
    ui->tblGroups->setHorizontalHeaderItem(COL_EDITBTN, new QTableWidgetItem("Edit"));
    loadGroupsFromFile();
    //show the groups in the table on the groups page
    showGroups();




    //send the group info to the arduino
    for(int i = 0; i < groups.size(); i++){
        sendGroupInfo(i, groups[i]);
        usleep(20*1000);
    }
     /*ui->sldRed->setValue(45);
     setSliderSilent(ui->sldRed,125);
     changeBackgroundOfRGBSlider(ui->sldRed, 255,0,0,125/255.0);
     cout << "STOP EARLY" << endl;*/


}

void MainWindow::sendInitData(int idx){

    if (idx == MODE_RGB){
    //rgb
   /* sendArduinoCmd("r:" + QString::number(config.r));
    sendArduinoCmd("g:" + QString::number(config.g));
    sendArduinoCmd("b:" + QString::number(config.b));
    sendArduinoCmd("d:" + QString::number(config.d));
    sendArduinoCmd("t:" + QString::number(config.t));
    sendArduinoCmd("B:" + QString::number(config.B));*/
    QString msg = "l0{";
    appendNum(msg, config.r);
    appendNum(msg,config.g);
    appendNum(msg,config.b);
    appendNum(msg,config.d);
    appendNum(msg,config.t);
    appendNum(msg,config.B);
    msg += "}";
    sendArduinoCmd(msg);
    loadSliders(idx);
    highlightSlider(0);

    } else if (idx == MODE_HSV){
        //THIS SHOULD BE CONVERTED FOR THE SINGLE MESSAGE TO LOAD BUT HOLD OFF BECAUSE IT MIGHT CHANGE TO JUST BYTES ANYWAY
        //AND NOW THAT THERE"S ACK THIS SEEMS TO BE WORKING BETTER
    //hsv
    QString msg = "l1{";
    appendNum(msg, config.h);
    appendNum(msg,config.s);
    appendNum(msg,config.v);
    msg += "}";
    sendArduinoCmd(msg);
    loadSliders(idx);
    highlightSlider(0);
    } else if (idx == MODE_GRADIENT){
        //gradient
        QString msg = "l2{";
        appendNum(msg, config.sh);
        appendNum(msg,config.ss);
        appendNum(msg,config.sv);
        appendNum(msg, config.eh);
        appendNum(msg,config.es);
        appendNum(msg,config.ev);
        msg += "}";
        sendArduinoCmd(msg);
        loadSliders(idx);
        highlightSlider(0);
    }
    //mark that we sent data for that page
    configDataSentToArduino[idx] = true;
}

//call this after everything has been drawn
void MainWindow::loadSliders(int idx){

    if (idx == MODE_RGB){
    //move all sliders down and show no background
    changeBackgroundOfRGBSlider(ui->sldRed, 255,0,0, config.r / 255.0);
    changeBackgroundOfRGBSlider(ui->sldGreen,0,255,0, config.g / 255.0);
    changeBackgroundOfRGBSlider(ui->sldBlue, 0,0,255, config.b / 255.0);
    changeBackgroundOfRGBSlider(ui->sldDaylight, 198, 243, 255, config.t / 255.0);
    changeBackgroundOfRGBSlider(ui->sldTungsten, 255,212,125 , config.d / 255.0);
    changeBackgroundOfRGBSlider(ui->sldBrightness,194,194,194, config.B / 255.0);
    setSliderSilent(ui->sldRed, config.r);
    setSliderSilent(ui->sldGreen,config.g);
    setSliderSilent(ui->sldBlue,config.b);
    setSliderSilent(ui->sldDaylight,config.d);
    setSliderSilent(ui->sldTungsten,config.t);
    setSliderSilent(ui->sldBrightness,config.B);
    } else  if (idx == MODE_HSV){


    //hsv
    //QTCreator is 0 to 359 but arduino is 0 to 254
    double zeroTo1 = config.h / 255.0;
    int value = (int)(zeroTo1 * 359.0);
    setSliderSilent(ui->sldHue,value);
    setSliderSilent(ui->sldSat,config.s);
    setSliderSilent(ui->sldVal,config.v);
    changeBackgroundOfHSVSlider(ui->sldHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldSat, value,config.s,255);
    changeBackgroundOfHSVSlider(ui->sldVal,value,config.s,config.v);

    } else  if (idx == MODE_GRADIENT){

    //gradient
        //convert the hue
        double zeroTo1 = config.sh / 255.0;
        int value = (int)(zeroTo1 * 359.0);
    setSliderSilent(ui->sldStartHue,value);
    setSliderSilent(ui->sldStartSat,config.ss);
    setSliderSilent(ui->sldStartVal,config.sv);
    changeBackgroundOfHSVSlider(ui->sldStartHue, value,180,255);
    changeBackgroundOfHSVSlider(ui->sldStartSat, value,config.ss,255);
    changeBackgroundOfHSVSlider(ui->sldStartVal, value, config.ss, config.sv);

    zeroTo1 = config.eh / 255.0;
    value = (int)(zeroTo1 * 359.0);
    setSliderSilent(ui->sldEndHue,value);
    setSliderSilent(ui->sldEndSat,config.es);
    setSliderSilent(ui->sldEndVal,config.sv);
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

   //when the sliders move it can cause a lot of messages to send
    //only send messages once every 30 ms
   /* while (getMillis() - lastArduinoSend < 25 ){
        ;;
    }
  */
  arduino->sendMsg(in.toStdString());
  //usleep(20*1000);
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

void MainWindow::on_sldRed_valueChanged(int value)
{
    //turn the value into a valid Arduino command
    //r:100
    //cout << "RED CHANGED" << value << endl;

    changeBackgroundOfRGBSlider(ui->sldRed, 200, 0, 0 , value/255.0);
    if (curSelection != RED){
       curSelection = RED;
       curHighlight = RED;
       highlightSlider(curHighlight);
       selectSlider(curSelection);
   }

    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.r - value) > SLD_CHANGE_THRESHOLD )
        {
                config.r = value;
                saveDataToFile();
                QString msg = "r:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].r - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].r = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
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
    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.g - value) > SLD_CHANGE_THRESHOLD )
        {
                config.g = value;
                saveDataToFile();
                QString msg = "g:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].g - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].g = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
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

    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.b - value) > SLD_CHANGE_THRESHOLD )
        {
                config.b = value;
                saveDataToFile();
                QString msg = "b:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].b - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].b = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
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
    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.t - value) > SLD_CHANGE_THRESHOLD )
        {
                config.t = value;
                saveDataToFile();
                QString msg = "t:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].t - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].t = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
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

    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.d - value) > SLD_CHANGE_THRESHOLD )
        {
                config.d = value;
                saveDataToFile();
                QString msg = "d:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].d - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].d = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
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

    //on load this can get called before the view is loaded which gives -1 at the index
    int groupIdx = ui->cmbRGB_Group->currentIndex();
    if (groupIdx == -1){
        return;
    }
    //if master group check if it changed enough
    if (groupIdx == 0){
        //check if we've moved the slider enough to send the new data
        if (abs(config.B - value) > SLD_CHANGE_THRESHOLD )
        {
                config.B = value;
                saveDataToFile();
                QString msg = "B:" + QString::number(value);
                sendArduinoCmd(msg);
        }

    }
    //check if the slider changed this much for this group
    else {
        groupIdx -= 1;
        if (abs(groups[groupIdx].B - value) > SLD_CHANGE_THRESHOLD )
        {
            groups[groupIdx].B = value;
            saveGroupsToFile();
            sendGroupInfo(groupIdx,groups[groupIdx]);
        }
    }
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
        CONFIG newConf = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
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
    infile >> config.mode ;
    infile.close();

    cout << "---Loaded config settings---" << endl;
    cout << "R,G,B,D,T,B:\t" << config.r << " "  << config.g  << " "  << config.b <<  " "  << config.d <<  " "  << config.t << " "  << config.B <<endl;
    cout << "H,S,V:\t" << config.h << " "  << config.s  << " "  << config.v << endl;
    cout << "GRADIENT" << endl;
    cout << "\tSTART--> H,S,V:\t" << config.sh << " "  << config.ss  << " "  << config.sv << endl;
    cout << "\tEND  --> H,S,V:\t" << config.eh << " "  << config.es  << " "  << config.ev << endl;
    cout << "MODE: " << config.mode << endl;
}

void MainWindow::loadGroupsFromFile(){

    groups.clear();
    ifstream infile(FILENAME_GROUPS.toStdString().c_str());
    if (!infile.is_open()){
        //TODO make this a message box
        cout << "Created new groups file!" <<endl;
        saveGroupsToFile();
        //reopen and load
        loadGroupsFromFile();
        return;
    }

    int NUM_GROUPS;
    string line;
    //first line is num groups
    infile >> NUM_GROUPS;
    infile.ignore(1000, '\n');
    cout << "Loading " << NUM_GROUPS << " Groups..." << endl;
    for(int i = 0; i < NUM_GROUPS; i++){
        //all groups are saved on one line, comma separated
        getline(infile,line);

        QStringList lP = QString::fromStdString(line).split(",");
        //convert all numbers to integers
        QList<int> nums;
        //the first is the name so skip that
        for(int i = 1; i < lP.size(); i++){
            nums.append( lP[i].toInt());
        }
        //make new group to append
        Group g = { lP[0],nums[0],nums[1],nums[2],nums[3],nums[4],nums[5],nums[6],nums[7],nums[8],nums[9],nums[10],nums[11],nums[12],nums[13],nums[14]};
        cout << g.name.toStdString() << " " << g.startLED << " " << g.stopLED << " " << g.type << " " << g.r << " " << g.g << " " << g.b << " " << g.d << " ";
        cout << g.t << " "<< g.B << endl;
        groups.append(g);
    }




}

void MainWindow::saveGroupsToFile(){
    ofstream outfile(FILENAME_GROUPS.toStdString().c_str());
    if (!outfile.is_open()){
        //TODO make this a message box
        cout << "Error could not open the settings file to save data!" <<endl;
        exit(1);
    }

    if (groups.size() == 0){
        outfile << 0 << endl;
        outfile.close();
        return;
    }

    outfile << groups.size() << endl;
    //save all on a file, name first then the rest of the parameters in order
    for(int i = 0; i < groups.size(); i++){
        Group g = groups[i]; 
        outfile << g.name.toStdString() << "," << g.startLED << "," << g.stopLED << "," << g.type << ",";
        outfile << g.r << ","<< g.g<< "," << g.b << ","  << g.d << "," << g.t << "," << g.B << ",";
        outfile << g.sh << ","<< g.ss<< "," << g.sv << ","  << g.eh << "," << g.es << "," << g.ev << endl;
    }
    outfile.close();


}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    cout << "CLICKED: " << index << endl;
    config.mode = index;
    curHighlight = 0;
    curSelection = 0;
    selectSlider(curSelection);
    highlightSlider(curHighlight);
    saveDataToFile();

    QString msg = "m:" + QString::number(index);
    sendArduinoCmd(msg);


    //if (!configDataSentToArduino[index]){
    sendInitData(config.mode);
    //}
}

void MainWindow::on_cmbRGB_Group_currentIndexChanged(int index)
{
    //if it's on master then change the whole entire strip
    if (index == 0){
        loadSliders(0);
    } else {
        //Now we're in groups, but they start at index 1 and are saved as 0 index so
        index -= 1;
        //sendGroupInfo(index, groups[index]);
        loadGroupToSliders(groups[index]);
    }
}

void MainWindow::loadGroupToSliders(Group g){
    if (g.type == MODE_RGB){
        changeBackgroundOfRGBSlider(ui->sldRed, 255,0,0, g.r / 255.0);
        changeBackgroundOfRGBSlider(ui->sldGreen,0,255,0, g.g / 255.0);
        changeBackgroundOfRGBSlider(ui->sldBlue, 0,0,255, g.b / 255.0);
        changeBackgroundOfRGBSlider(ui->sldDaylight, 198, 243, 255, g.t / 255.0);
        changeBackgroundOfRGBSlider(ui->sldTungsten, 255,212,125 , g.d / 255.0);
        changeBackgroundOfRGBSlider(ui->sldBrightness,194,194,194, g.B / 255.0);
        setSliderSilent(ui->sldRed,g.r);
        setSliderSilent(ui->sldGreen,g.g);
        setSliderSilent(ui->sldBlue,g.b);
        setSliderSilent(ui->sldDaylight,g.d);
        setSliderSilent(ui->sldTungsten,g.t);
        setSliderSilent(ui->sldBrightness,g.B);
    }
}

void MainWindow::sendGroupInfo(int groupIndex, Group g){
    QString msg = "gr{" + QString::number(groupIndex);
    appendNum(msg, g.type);
    appendNum(msg, g.startLED);
    appendNum(msg, g.stopLED);
    if (g.type == MODE_RGB){
        appendNum(msg,g.r);
        appendNum(msg,g.g);
        appendNum(msg,g.b);
        appendNum(msg,g.d);
        appendNum(msg,g.t);
        appendNum(msg,g.B);
    } else if (g.type == MODE_HSV){
        appendNum(msg,g.sh);
        appendNum(msg,g.ss);
        appendNum(msg,g.sv);
    }

    msg += "}";
    sendArduinoCmd(msg);
   // cout << "Sent: " << msg.toStdString() << endl;
}

void MainWindow::setSliderSilent(QSlider *qs, int val){
    qs->blockSignals(true);
    qs->setValue(val);
    qs->blockSignals(false);
}

void MainWindow::on_btnAddGroup_clicked()
{
    Group g = {"",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    g.name = "Group " + QString::number(groups.size() + 1);
    groups.append(g);
    showGroups();
    saveGroupsToFile();


}


void MainWindow::on_tblGroups_cellChanged(int row, int column)
{

    Group * groupToChange = &groups[row];
    QTableWidgetItem * cell = ui->tblGroups->item(row,column);
    //TODO catch a toInt error on the cell
    switch(column){
        case COL_NAME:
            groupToChange->name = cell->text();
            break;
        case COL_STARTLED:
            groupToChange->startLED = cell->text().toInt();
            break;
        case COL_STOPLED:
            groupToChange->stopLED = cell->text().toInt();
            break;
        default:
            break;
    }

    saveGroupsToFile();


}

