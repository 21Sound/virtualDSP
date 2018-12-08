#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QPlainTextEdit>
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
    void sampleRateMenuHandle(QAction *currentAction);
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
    void hiPassCharWidgetHandle(double charac);
	void hiPassFreqWidgetHandle(double freq);
	void hiPassOrdWidgetHandle(double ord);
	void loPassCharWidgetHandle(double charac);
	void loPassFreqWidgetHandle(double freq);
	void loPassOrdWidgetHandle(double ord);
    void limitThresWidgetHandle(double thres);
    void limitMakeupWidgetHandle(double gain);
    void limitRelWidgetHandle(double relTime);

private:
    void copyParams(CppRTA *oldInst, CppRTA *newInst);
    void updateEQWidgets();
    void updateCutWidgets();
    void updateLimiterWidgets();
    void deviceMenuUpdate();
    void copyMenuUpdate();
    int storeParams(const char* fileName);
    int loadParams(const char* fileName);

    QCustomPlot tfPlot;
    QVector<double> xPlot, yPlot;
    QVector<complex_float64> complexFreqVec;

    paramWidget channelWidget, hiPassTypeWidget, hiPassCharWidget, hiPassFreqWidget, hiPassOrdWidget, loPassTypeWidget,
		loPassCharWidget, loPassFreqWidget, loPassOrdWidget, eqNrWidget, eqGainWidget, eqFreqWidget, eqQFactWidget,
		eqTypeWidget, limitThresWidget, limitMakeupWidget, limitRelWidget;

    QPushButton inOutButton;

    QIcon applicationIcon, inOutIcon, pauseIcon;

    QPlainTextEdit statusTxt;

    QMenuBar menuBar;
    QMenu *settingsMenu, *blockLenMenu, *sampleRateMenu, *hostApiMenu, *inDeviceMenu,
          *outDeviceMenu, *copyMenu;

    CppRTA *rtIO;
    std::vector<unsigned int> actEQ;
    std::vector<std::string> hostAPIs;
    std::vector<deviceContainerRTA> inDevices, outDevices;
    std::string hostAPI;
    deviceContainerRTA inDevice, outDevice;

    uint32_t blockLenIO, fs, actChan;
    bool streamFlag, tenTimesFlag, stereoLockFlag;
};

#endif // MAINWINDOW_H
