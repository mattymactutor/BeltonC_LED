#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <QSerialPort>
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


private:
    Ui::MainWindow *ui;
    QSerialPort *arduino;
    QByteArray serialData;
    std::string serialBuffer;
    void loadCOMPorts();
    void initArduino(QString port);
    void parseArduinoCmd(string in);
    void sendArduinoCmd(QString in);

private slots:
       void readSerial();
       void on_btnClose_clicked();
       void on_cmbCOM_currentIndexChanged(int index);
       void on_sldRed_valueChanged(int value);
};
#endif // MAINWINDOW_H
