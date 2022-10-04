#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <string>
#include "Serial_Comm_Footpedal.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Group
{
public:
  int startLED;
  int stopLED;
  int type;
  int r, g, b, d, t, B;
  int sh, ss, sv, eh, es, ev;
};

//make a strucutre to keep track of the settings
struct CONFIG{
    int h, s, v;
    //rgb daylight tungsten brightness
    int r, g, b, d, t, B;
    int sh,ss,sv,eh,es,ev;
    //possibly add ability to change the num of leds? for the gradient.
    int mode;
};



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //int curHighlight = 0, curSelection = -1;
    //void highlightSlider(int sld);
    //void selectSlider(int sld);

private:
    Ui::MainWindow *ui;    
    double lastArduinoSend = 0;
    void loadDataFromFile();
    void saveDataToFile();
    void sendInitData(int idx);
    void sendArduinoCmd(QString in);
    void sendGroupInfo(int groupIdx, Group g);
    void loadSliders(int idx);
    void loadGroupToSliders(Group g);
    void setSliderSilent(QSlider * qs, int val);
   // void parseUSBCmd(string in);


private slots:
       void on_btnClose_clicked();       
       void on_sldRed_valueChanged(int value);
       void on_sldGreen_valueChanged(int value);
       void on_sldBlue_valueChanged(int value);
       void on_sldTungsten_valueChanged(int value);
       void on_sldDaylight_valueChanged(int value);
       void on_sldBrightness_valueChanged(int value);
       void on_sldHue_valueChanged(int value);
       void on_sldSat_valueChanged(int value);
       void on_sldVal_valueChanged(int value);
       void on_sldStartHue_valueChanged(int value);
       void on_sldStartSat_valueChanged(int value);
       void on_sldStartVal_valueChanged(int value);
       void on_sldEndHue_valueChanged(int value);
       void on_sldEndSat_valueChanged(int value);
       void on_sldEndVal_valueChanged(int value);
       void on_tabWidget_tabBarClicked(int index);
       void on_cmbRGB_Group_currentIndexChanged(int index);
};
#endif // MAINWINDOW_H
