#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include "Serial_Comm_Footpedal.h"
using namespace std;

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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void loadSliders();

private:
    Ui::MainWindow *ui;    
    std::string serialBuffer;
    int curHighlight = 0, curSelection = -1;   
    void highlightSlider(int sld);
    void selectSlider(int sld);
    void loadDataFromFile();
    void saveDataToFile();
    void sendInitData();
    void sendArduinoCmd(QString in);





private slots:     
       void on_btnClose_clicked();
       void on_cmbCOM_currentIndexChanged(int index);
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
};
#endif // MAINWINDOW_H
