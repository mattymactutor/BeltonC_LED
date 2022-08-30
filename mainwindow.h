#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <QSerialPort>
using namespace std;

#define RED 0
#define GREEN 1
#define BLUE 2
#define TUNGSTEN 3
#define DAYLIGHT 4
#define BRIGHTNESS 5

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    QSerialPort *arduino;
    QByteArray serialData;
    std::string serialBuffer;
    int curHighlight = 0, curSelection = -1;
    void loadCOMPorts();
    void initArduino(QString port);
    void parseArduinoCmd(string in);
    void sendArduinoCmd(QString in);
    void highlightSlider(int sld);
    void selectSlider(int sld);

private slots:
       void readSerial();
       void on_btnClose_clicked();
       void on_cmbCOM_currentIndexChanged(int index);
       void on_sldRed_valueChanged(int value);
       void on_sldGreen_valueChanged(int value);
       void on_sldBlue_valueChanged(int value);
       void on_sldTungsten_valueChanged(int value);
       void on_sldDaylight_valueChanged(int value);
       void on_sldBrightness_valueChanged(int value);
       void on_sldRed_sliderPressed();
};
#endif // MAINWINDOW_H
