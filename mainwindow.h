#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QGridLayout>
#include <vector>

#include "qcustomplot.h"
#include "paramWidget.h"
#include "CppRTA.h"
#include "fft.h"

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    MainWindow(int width, int height, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void inOutButtonHandle();
    void plotUpdate();
    void settingsMenuHandle(QAction *currentAction);
    void blockLenMenuHandle(QAction *currentAction);
    void hostApiMenuHandle(QAction *currentAction);
    void inDeviceMenuHandle(QAction *currentAction);
    void outDeviceMenuHandle(QAction *currentAction);
    void copyMenuHandle(QAction *currentAction);

    void channelWidgetHandle(double chanNr);
    void eqNrWidgetHandle(double eqNr);
    void eqGainWidgetHandle(double gain);
    void eqFreqWidgetHandle(double freq);
    void eqQFactWidgetHandle(double QFact);
    void eqTypeWidgetHandle(double type);
    void limitThresWidgetHandle(double thres);
    void limitMakeupWidgetHandle(double gain);
    void limitRelWidgetHandle(double relTime);

private:
    void updateRtIO();
    void paramReset();
    void resizeChannelParams();
    void resizeAllEQParams();
    void resizeEQParams(int chanNr);
    void updateParamsRtIO();
    void updateEQWidgets();
    void updateLimiterWidgets();
    void deviceMenuUpdate();
    void copyMenuUpdate();
    int storeParams(const char* fileName);
    int loadParams(const char* fileName);

    QCustomPlot tfPlot;
    QVector<double> xPlot, yPlot;
    QVector<complex_float64> complexFreqVec;

    paramWidget channelWidget, eqNrWidget, eqGainWidget, eqFreqWidget, eqQFactWidget,
                eqTypeWidget, limitThresWidget, limitMakeupWidget, limitRelWidget;

    QPushButton inOutButton, tenTimesButton;

    QIcon applicationIcon, inOutIcon, pauseIcon;

    QTextEdit statusTxt;

    QMenuBar menuBar;
    QMenu *settingsMenu, *blockLenMenu, *hostApiMenu, *inDeviceMenu, *outDeviceMenu, *copyMenu;

    CppRTA *rtIO;
    std::vector<unsigned int> actEQ, numEQs;
    std::vector<double> thres, makeup, relTime;
    std::vector< std::vector<double> > gain, freq, QFact, type;
    std::vector<std::string> hostAPIs;
    std::vector<deviceContainerRTA> inDevices, outDevices;
    std::string hostAPI;
    deviceContainerRTA inDevice, outDevice;

    unsigned int blockLenIO, fs, actChan;
    bool streamFlag, tenTimesFlag, stereoLockFlag;
};

#endif // MAINWINDOW_H
