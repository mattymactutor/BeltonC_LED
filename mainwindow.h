#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QCheckBox>
#include <QStringListModel>
#include <string>
#include "Serial_Comm_Footpedal.h"

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define ACTIVE 0
#define INACTIVE 1
#define OFF 2
struct Group
{
public:
  QString name;
  int startLED;
  int stopLED;
  int type;
  int r, g, b, d, t, B;
  int sh, ss, sv, eh, es, ev;
  int status;

};

//make a strucutre to keep track of the settings
struct CONFIG{
    int h, s, v;
    //rgb daylight tungsten brightness
    int r, g, b, d, t, B;
    int sh,ss,sv,eh,es,ev;
    int mode;
    int numLEDs;
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
    void loadConfigFromFile();
    void saveConfigToFile();
    void loadGroupsFromFile();
    void saveGroupsToFile();
    void sendInitData(int idx);
    void sendArduinoCmd(QString in);
    void sendGroupInfo(QString name);
    void loadSliders();
    void loadGroupToSliders(QString);
    void setSliderSilent(QSlider * qs, int val);
    void showGroups();
    Group * getGroupFromName(QString name, int * idx = nullptr);
    void processSliderChange(int value, int * configVal, int * groupVal, QString groupName, QString arduinoCmd,bool hsvConvert = false);
    QComboBox * createGroupModeCombo(int row, int idx);
    QComboBox * createGroupStatusCombo(int row, int idx);
    QPushButton * createGroupEditButton(int row);
    QCheckBox * createGroupCheckbox(int row);
   // void parseUSBCmd(string in);


private slots:
       void tableComboModeChanged(int idx);
       void tableComboStatusChanged(int idx);
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
       void on_btnAddGroup_clicked();
       void on_tblGroups_cellChanged(int row, int column);
       void on_cmbRGB_Groups_currentIndexChanged(int index);
       void on_cmbHSV_Groups_currentIndexChanged(int index);
       void on_cmbGradient_Groups_currentIndexChanged(int index);
       void on_edtNumLeds_textChanged();
       void on_btnDeleteGroup_clicked();
};
#endif // MAINWINDOW_H
