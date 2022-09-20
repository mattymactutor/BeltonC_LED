#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include "Serial_Comm_Footpedal.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE



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

    void loadDataFromFile();
    void saveDataToFile();
    void sendInitData(int idx);
    void sendArduinoCmd(QString in);
    void loadSliders(int idx);
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
};
#endif // MAINWINDOW_H
