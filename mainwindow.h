#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <vector>

#include "qcustomplot.h"
#include "paramWidget.h"
#include "CppRTA.h"
#include "fft.h"

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void inOutButtonHandle();
    void plotUpdate();
    void blockLenMenuHandle(QAction *currentAction);
    void inDeviceMenuHandle(QAction *currentAction);
    void outDeviceMenuHandle(QAction *currentAction);

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

    void resetRtIO();
    void resetOnDeviceSwitch();
    void resizeChannelParams();
    void resizeAllEQParams();
    void resizeActEQParams();
    void updateParams();
    void updateEQWidgets();
    void updateLimiterWidgets();

    QCustomPlot tfPlot;
    QVector<double> xPlot, yPlot;
    QVector<complex_float64> complexFreqVec;
    QTimer plotTimer;

    paramWidget channelWidget, eqNrWidget, eqGainWidget, eqFreqWidget, eqQFactWidget,
                eqTypeWidget, limitThresWidget, limitMakeupWidget, limitRelWidget;

    QPushButton inOutButton;

    QIcon applicationIcon, inOutIcon, pauseIcon;

    QTextEdit statusTxt;

    QMenuBar menuBar;
    QMenu *blockLenMenu, *inDeviceMenu, *outDeviceMenu;

    CppRTA *rtIO;
    std::vector<unsigned int> actEQ, numEQs;
    std::vector<double> thres, makeup, relTime;
    std::vector< std::vector<double> > gain, freq, QFact, type;
    std::vector<deviceContainerRTA> inDevices, outDevices;
    deviceContainerRTA inDevice, outDevice;

    unsigned int blockLenIO, fs, actChan;
    bool streamFlag;
};

#endif // MAINWINDOW_H
