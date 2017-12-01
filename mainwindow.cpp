#include "mainwindow.h"
#include <stdexcept>

#define NFFT 32768
#define MAX_NUM_EQS_PER_CHAN 20

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), rtIO(nullptr), actChan(0), actEQ(0) {
    QBrush plotBrush(QColor(150,150,150,150));
    QFileInfo currentPath(QCoreApplication::applicationFilePath());
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    QSharedPointer<QCPAxisTickerText> textTickerFreq(new QCPAxisTickerText);

    const unsigned int smallestBlockLen = 64;
    const unsigned int numBlockLengths = 8;
    streamFlag = false;
    blockLenIO = 512;
    fs = 48000;
    double loFreq = 20;
    double hiFreq = fs*0.499;


    /*___________________GUI initialization____________________*/

    QString tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/logo.png");
    applicationIcon.addFile(tmpStr);

    tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/InOut.png");
    inOutIcon.addFile(tmpStr);

    tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/pause.png");
    pauseIcon.addFile(tmpStr);

    this->setWindowIcon(applicationIcon);
    this->setWindowTitle("virtualDSP");
    this->setAcceptDrops(false);

    inOutButton.setParent(this);
    inOutButton.setIcon(inOutIcon);
    inOutButton.move(QPoint(10,30));
    inOutButton.resize(QSize(60,60));
    inOutButton.setIconSize(QSize(50,50));

    channelWidget.setParent(this);
    channelWidget.move(110,25);
    channelWidget.setLabel(QString("Channel Nr."));
    channelWidget.setUnit(QString(""));
    channelWidget.setLimits(1, 1);
    channelWidget.setStepSize(1.0);
    channelWidget.setValue(1);
    channelWidget.setReadOnly(true);

    eqNrWidget.setParent(this);
    eqNrWidget.move(190,25);
    eqNrWidget.setLabel(QString("EQ Nr."));
    eqNrWidget.setUnit(QString(""));
    eqNrWidget.setLimits(1, MAX_NUM_EQS_PER_CHAN);
    eqNrWidget.setStepSize(1.0);
    eqNrWidget.setValue(1);
    eqNrWidget.setReadOnly(true);

    eqGainWidget.setParent(this);
    eqGainWidget.move(270,25);
    eqGainWidget.setLabel(QString("EQ Gain"));
    eqGainWidget.setUnit(QString("dB"));
    eqGainWidget.setLimits(-20.0, 20.0);
    eqGainWidget.setStepSize(0.5);
    eqGainWidget.setValue(0.0);

    eqFreqWidget.setParent(this);
    eqFreqWidget.move(350,25);
    eqFreqWidget.setLabel(QString("EQ Freq"));
    eqFreqWidget.setUnit(QString("Hz"));
    eqFreqWidget.setLimits(20.0, std::min(fs*0.5, 20000.0));
    eqFreqWidget.setStepSize(1.01);
    eqFreqWidget.setValue(1000);
    eqFreqWidget.setPrecision(0.1);
    eqFreqWidget.setLogScaling(true);

    eqQFactWidget.setParent(this);
    eqQFactWidget.move(430,25);
    eqQFactWidget.setLabel(QString("EQ QFact"));
    eqQFactWidget.setUnit(QString("NV"));
    eqQFactWidget.setLimits(0.1, 10);
    eqQFactWidget.setStepSize(0.01);
    eqQFactWidget.setValue(1.0);

    eqTypeWidget.setParent(this);
    eqTypeWidget.move(510,25);
    eqTypeWidget.setLabel(QString("EQ Type"));
    eqTypeWidget.setUnit(QString(""));
    eqTypeWidget.setLimits(1, 8);
    eqTypeWidget.setStepSize(1);
    eqTypeWidget.setValue(6);
    eqTypeWidget.setReadOnly(true);

    limitThresWidget.setParent(this);
    limitThresWidget.move(350,105);
    limitThresWidget.setLabel(QString("Lim. Thres"));
    limitThresWidget.setUnit(QString("dB"));
    limitThresWidget.setLimits(-50, 0);
    limitThresWidget.setStepSize(0.5);
    limitThresWidget.setValue(0);

    limitMakeupWidget.setParent(this);
    limitMakeupWidget.move(430,105);
    limitMakeupWidget.setLabel(QString("Lim. Gain"));
    limitMakeupWidget.setUnit(QString("dB"));
    limitMakeupWidget.setLimits(-20.0, 20.0);
    limitMakeupWidget.setStepSize(0.5);
    limitMakeupWidget.setValue(0);

    limitRelWidget.setParent(this);
    limitRelWidget.move(510,105);
    limitRelWidget.setLabel(QString("Lim. Rel"));
    limitRelWidget.setUnit(QString("sec"));
    limitRelWidget.setLimits(0.01, 10.0);
    limitRelWidget.setStepSize(0.01);
    limitRelWidget.setValue(2.0);

    tfPlot.setParent(this);
    tfPlot.move(10,100);
    tfPlot.resize(QSize(340,180));
    tfPlot.addGraph();
    tfPlot.graph(0)->setPen(QColor(50,50,230));
    tfPlot.graph(0)->setBrush(plotBrush);
    tfPlot.xAxis->setScaleType(QCPAxis::stLogarithmic);
    tfPlot.xAxis->setTicker(logTicker);
    tfPlot.xAxis->setRange(loFreq,hiFreq);
    tfPlot.yAxis->setRange(-40,20);
    tfPlot.xAxis->setLabel("Frequency f [Hz]");
    tfPlot.yAxis->setLabel("Amplitude A [dBFS]");
    tfPlot.setBackground(this->palette().background().color());
    xPlot.resize(NFFT/2+1);
    yPlot.resize(NFFT/2+1);
    for (int i=0; i<xPlot.size(); i++) {
        xPlot[i] = i*(fs/2)/(NFFT/2.0+1.0);
        yPlot[i] = 0;
    }
    tfPlot.graph(0)->setData(xPlot, yPlot, true);

    plotTimer.setInterval(50);

    statusTxt.setParent(this);
    statusTxt.setText(QString("Welcome to virtualDSP!"));
    statusTxt.move(QPoint(10,290));
    statusTxt.resize(QSize(340,100));
    statusTxt.setPalette(this->palette());
    statusTxt.viewport()->setAutoFillBackground(false);
    statusTxt.setReadOnly(true);

    menuBar.setParent(this);
    menuBar.move(0,0);
    menuBar.resize(800,20);
    menuBar.setPalette(this->palette());

    blockLenMenu = menuBar.addMenu("Set block length");
    for(unsigned int i = 0; i<numBlockLengths; i++) {
        blockLenMenu->addAction(QString::number(smallestBlockLen<<i));
        blockLenMenu->actions().at(i)->setData(smallestBlockLen<<i);
    }

    CppRTA::getDevices(inDevices, outDevices);
    inDevice = inDevices.at(0);
    outDevice = outDevices.at(0);
    rtIO = new CppRTA(inDevice, outDevice, blockLenIO, fs);
    channelWidget.setLimits(1, outDevice.numChans);

    inDeviceMenu = menuBar.addMenu("Set input device");
    for(unsigned int i = 0; i<inDevices.size(); i++) {
        inDeviceMenu->addAction(QString::fromStdString(inDevices.at(i).name));
        inDeviceMenu->actions().at(i)->setData(i);
    }

    outDeviceMenu = menuBar.addMenu("Set output device");
    for(unsigned int i = 0; i<outDevices.size(); i++) {
        outDeviceMenu->addAction(QString::fromStdString(outDevices.at(i).name));
        outDeviceMenu->actions().at(i)->setData(i);
    }

    resizeChannelParams();
    resizeAllEQParams();
    updateLimiterWidgets();
    updateEQWidgets();

    connect(&inOutButton, SIGNAL (clicked()), this, SLOT(inOutButtonHandle()));
    connect(&plotTimer, SIGNAL (timeout()), this, SLOT(plotUpdate()));
    connect(&channelWidget, SIGNAL (valueChanged(double)), this, SLOT(channelWidgetHandle(double)));

    connect(&eqNrWidget, SIGNAL (valueChanged(double)), this, SLOT(eqNrWidgetHandle(double)));
    connect(&eqGainWidget, SIGNAL (valueChanged(double)), this, SLOT(eqGainWidgetHandle(double)));
    connect(&eqFreqWidget, SIGNAL (valueChanged(double)), this, SLOT(eqFreqWidgetHandle(double)));
    connect(&eqQFactWidget, SIGNAL (valueChanged(double)), this, SLOT(eqQFactWidgetHandle(double)));
    connect(&eqTypeWidget, SIGNAL (valueChanged(double)), this, SLOT(eqTypeWidgetHandle(double)));
    connect(&limitThresWidget, SIGNAL (valueChanged(double)), this, SLOT(limitThresWidgetHandle(double)));
    connect(&limitRelWidget, SIGNAL (valueChanged(double)), this, SLOT(limitRelWidgetHandle(double)));
    connect(&limitMakeupWidget, SIGNAL (valueChanged(double)), this, SLOT(limitMakeupWidgetHandle(double)));

    connect(blockLenMenu, SIGNAL(triggered(QAction*)), this, SLOT(blockLenMenuHandle(QAction*)));
    connect(inDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(inDeviceMenuHandle(QAction*)));
    connect(outDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(outDeviceMenuHandle(QAction*)));
}

void MainWindow::inOutButtonHandle() {
    statusTxt.setText("inOut Button Toggled.");

    if (!streamFlag) {
        inOutButton.setIcon(pauseIcon);
        resetRtIO();
    }
    else {
        inOutButton.setIcon(inOutIcon);
        statusTxt.append("\nAudio roundtrip paused.");
        if (rtIO != nullptr) {
            rtIO->stopStream();
        }
    }
    streamFlag = !streamFlag;
}

void MainWindow::plotUpdate() {
    if (rtIO != nullptr) {
        std::vector<double> transferFcn(NFFT/2+1, 0.0);
        rtIO->getTransferFunction(transferFcn, actChan, NFFT);

        for (unsigned int i=0; i<NFFT/2+1; i++) {
            yPlot[i] = transferFcn[i];
        }

        tfPlot.graph(0)->setData(xPlot, yPlot, true);
        tfPlot.replot();
    }
}

void MainWindow::blockLenMenuHandle(QAction *currentAction) {
    blockLenIO = currentAction->data().toInt();
    if (streamFlag) {
        resetRtIO();
    }
}

void MainWindow::inDeviceMenuHandle(QAction *currentAction) {
    inDevice = inDevices.at(currentAction->data().toInt());
    resetOnDeviceSwitch();
    if (streamFlag) {
        resetRtIO();
    }
    statusTxt.setText(QString("Changed input device to ") + QString::fromStdString(inDevice.name));
}

void MainWindow::outDeviceMenuHandle(QAction *currentAction) {
    outDevice = outDevices.at(currentAction->data().toInt());
    channelWidget.setLimits(1, outDevice.numChans);
    channelWidget.setValue(1);
    resetOnDeviceSwitch();
    if (streamFlag) {
        resetRtIO();
    }
    statusTxt.setText(QString("Changed output device to ") + QString::fromStdString(outDevice.name));
}

void MainWindow::channelWidgetHandle(double chanNr) {
    if (actChan<outDevice.numChans) {
        actChan = (unsigned int) chanNr-1;
        if (gain.at(actChan).size() < numEQs.at(actChan)) {
            resizeActEQParams();
        }
        updateLimiterWidgets();
        updateEQWidgets();
        plotUpdate();
    } else {
        actChan=outDevice.numChans-1;
    }
    statusTxt.setText(QString("Set channel nr. to ") + QString::number(actChan+1));
}

void MainWindow::eqNrWidgetHandle(double eqNr) {
    actEQ.at(actChan) = (unsigned int) eqNr-1;
    if (actEQ.at(actChan) >= numEQs.at(actChan)) {
        numEQs.at(actChan) = actEQ.at(actChan)+1;
        resizeActEQParams();
        if (rtIO != nullptr) {
            rtIO->setNumEQs(actChan, numEQs.at(actChan));
        }
    } else if ((int32_t)actEQ.at(actChan) < (int32_t)numEQs.at(actChan)-1
    		&& gain.at(actChan).back() == 0.0 && type.at(actChan).back() > 4) {
        numEQs.at(actChan) = actEQ.at(actChan)+1;
        resizeActEQParams();
        if (rtIO != nullptr) {
            rtIO->setNumEQs(actChan, numEQs.at(actChan));
        }
    }
    updateEQWidgets();
    plotUpdate();
    statusTxt.setText(QString("Set EQ nr. of channel ") + QString::number(actChan+1)
                      + QString(" to ") + QString::number(actEQ.at(actChan)+1));
}

void MainWindow::eqGainWidgetHandle(double gain) {
    this->gain.at(actChan).at(actEQ.at(actChan)) = gain;
    if (rtIO != nullptr) {
        rtIO->setGain(actChan, actEQ.at(actChan), gain);
    }
    plotUpdate();
    statusTxt.setText(QString("Set EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                      + QString::number(actChan+1) + QString(" to ") + QString::number(gain)
                      + QString(", ") + QString::number(numEQs.at(actChan)) + QString(" EQs allocated."));
}

void MainWindow::eqFreqWidgetHandle(double freq) {
    this->freq.at(actChan).at(actEQ.at(actChan)) = freq;
    if (rtIO != nullptr) {
        rtIO->setFreq(actChan, actEQ.at(actChan), freq);
    }
    plotUpdate();
    statusTxt.setText(QString("Set EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                      + QString::number(actChan+1) + QString(" to ") + QString::number(freq)
                      + QString(", ") + QString::number(numEQs.at(actChan)) + QString(" EQs allocated."));
}

void MainWindow::eqQFactWidgetHandle(double QFact) {
    this->QFact.at(actChan).at(actEQ.at(actChan)) = QFact;
    if (rtIO != nullptr) {
        rtIO->setQFactor(actChan, actEQ.at(actChan), QFact);
    }
    plotUpdate();
    statusTxt.setText(QString("Set EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                      + QString::number(actChan+1) + QString(" to ") + QString::number(QFact)
                      + QString(", ") + QString::number(numEQs.at(actChan)) + QString(" EQs allocated."));
}

void MainWindow::eqTypeWidgetHandle(double type) {
    this->type.at(actChan).at(actEQ.at(actChan)) = (unsigned int) type-1;
    if (rtIO != nullptr) {
        rtIO->setType(actChan, actEQ.at(actChan), type-1);
    }
    plotUpdate();
    statusTxt.setText(QString("Set EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                      + QString::number(actChan+1) + QString(" to ") + QString::number(type)
                      + QString(", ") + QString::number(numEQs.at(actChan)) + QString(" EQs allocated."));
}

void MainWindow::limitThresWidgetHandle(double thres) {
    this->thres.at(actChan) = thres;
    if (rtIO != nullptr) {
        rtIO->setThreshold(actChan, thres);
    }
    statusTxt.setText(QString("Set Limiter of channel ") + QString::number(actChan+1)
                      + QString(" to ") + QString::number(thres));
}

void MainWindow::limitMakeupWidgetHandle(double gain) {
    this->makeup.at(actChan) = gain;
    if (rtIO != nullptr) {
        rtIO->setMakeupGain(actChan, gain);
    }
    statusTxt.setText(QString("Set Limiter of channel ") + QString::number(actChan+1)
                      + QString(" to ") + QString::number(gain));
}

void MainWindow::limitRelWidgetHandle(double relTime) {
    this->relTime.at(actChan) = relTime;
    if (rtIO != nullptr) {
        rtIO->setReleaseTime(actChan, relTime);
    }
    statusTxt.setText(QString("Set Limiter of channel ") + QString::number(actChan+1)
                      + QString(" to ") + QString::number(relTime));
}

void MainWindow::resetRtIO() {
    if (rtIO != nullptr) {
        delete rtIO;
        rtIO = nullptr;
    }
    try {
        rtIO = new CppRTA(inDevice, outDevice, blockLenIO, fs);
        rtIO->startStream();
        for (unsigned int i=0; i<outDevice.numChans; i++) {
            rtIO->setNumEQs(i, numEQs.at(i));
            for (unsigned int j=0; j<numEQs.at(i); j++) {
                rtIO->setGain(i, j, gain.at(i).at(j));
                rtIO->setFreq(i, j, freq.at(i).at(j));
                rtIO->setQFactor(i, j, QFact.at(i).at(j));
                rtIO->setType(i, j, type.at(i).at(j));
            }
            rtIO->setThreshold(i, thres.at(i));
            rtIO->setThreshold(i, makeup.at(i));
            rtIO->setThreshold(i, relTime.at(i));
        }
        statusTxt.append("\nAudio roundtrip started.");
    } catch (const std::exception& err) {
        statusTxt.append(QString("\nAudio initialization error:\n")+QString(err.what())
                         +QString("\nYou may try another device combination."));
    }
}

void MainWindow::resetOnDeviceSwitch() {
    actChan = 0;
    resizeChannelParams();
    resizeAllEQParams();
    for(unsigned int i=0; i<outDevice.numChans; i++) {
        actEQ.at(i) = 0;
    }
    updateLimiterWidgets();
    updateEQWidgets();
}

void MainWindow::updateEQWidgets() {
    unsigned int eqNr = actEQ.at(actChan);
    eqNrWidget.setValue(eqNr+1);
    eqGainWidget.setValue(gain.at(actChan).at(eqNr));
    eqFreqWidget.setValue(freq.at(actChan).at(eqNr));
    eqQFactWidget.setValue(QFact.at(actChan).at(eqNr));
    eqTypeWidget.setValue(type.at(actChan).at(eqNr)+1);
}

void MainWindow::updateLimiterWidgets() {
    if (thres.size()!=outDevice.numChans) {
        resizeChannelParams();
    }
    limitThresWidget.setValue(thres.at(actChan));
    limitMakeupWidget.setValue(makeup.at(actChan));
    limitRelWidget.setValue(relTime.at(actChan));
}

void MainWindow::resizeChannelParams() {
    if (actEQ.size() == outDevice.numChans) {
        return;
    } else {
        actEQ.resize(outDevice.numChans, 0);
        numEQs.resize(outDevice.numChans, 1);
        thres.resize(outDevice.numChans, 0.0);
        makeup.resize(outDevice.numChans, 0.0);
        relTime.resize(outDevice.numChans, 2.0);
        gain.resize(outDevice.numChans);
        freq.resize(outDevice.numChans);
        QFact.resize(outDevice.numChans);
        type.resize(outDevice.numChans);
        resizeActEQParams();
    }
}

void MainWindow::resizeAllEQParams() {
    for (unsigned int i=0; i<outDevice.numChans; i++) {
        gain.at(i).resize(numEQs.at(i), 0.0);
        freq.at(i).resize(numEQs.at(i), 1000.0);
        QFact.at(i).resize(numEQs.at(i), 1.0);
        type.at(i).resize(numEQs.at(i), 5);
    }
}

void MainWindow::resizeActEQParams() {
    unsigned int numEQ = numEQs.at(actChan);
    gain.at(actChan).resize(numEQ, 0.0);
    freq.at(actChan).resize(numEQ, 1000.0);
    QFact.at(actChan).resize(numEQ, 1.0);
    type.at(actChan).resize(numEQ, 5);
}

void MainWindow::updateParams() {
    if (rtIO != nullptr) {
        for (unsigned int i=0; i<outDevice.numChans; i++) {
            for (unsigned int j=0; j<numEQs.at(i); j++) {
                rtIO->setGain(i, j, gain.at(i).at(j));
                rtIO->setFreq(i, j, freq.at(i).at(j));
                rtIO->setQFactor(i, j, QFact.at(i).at(j));
                rtIO->setType(i, j, type.at(i).at(j));
            }
            rtIO->setThreshold(i, thres.at(i));
            rtIO->setMakeupGain(i, makeup.at(i));
            rtIO->setReleaseTime(i, relTime.at(i));
        }
    }
}

MainWindow::~MainWindow(){
    if (streamFlag) {
        inOutButtonHandle();
    }

    if (rtIO != nullptr) {
        delete rtIO;
        rtIO = nullptr;
    }
}

/*
int32_t CppRTA::storeParams() {
    uint32_t tmpInt;
    double tmpDouble;
    std::ofstream fStr("params.vdsp", std::ios::binary | std::ios::trunc);

    if (fStr.good()) {
        fStr.write((char*)&numOutChans, sizeof(uint32_t));
        fStr.write((char*)&numEQsPerChan, sizeof(uint32_t));
        for (uint32_t i=0; i<numOutChans; i++) {
            for (uint32_t j=0; j<numEQsPerChan; j++) {
                tmpDouble = EQ.at(i).at(j).getSampleRate();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getGain();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getFreq();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getQFactor();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpInt = EQ.at(i).at(j).getType();
                fStr.write((char*)&tmpInt, sizeof(uint32_t));
            }
            tmpDouble = limiter.at(i).getThreshold();
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = limiter.at(i).getMakeupGain();
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = limiter.at(i).getReleaseTime();
            fStr.write((char*)&tmpDouble, sizeof(double));
        }
        return 0;
    } else {
        return -1;
    }
}

int32_t CppRTA::readParams() {
    uint32_t tmpInt, numOutChansFile, numEQsPerChanFile;
    double tmpDouble;
    std::ifstream fStr("params.vdsp", std::ios::binary);

    if (fStr.good()) {
        fStr.read((char*)&numOutChansFile, sizeof(uint32_t));
        fStr.read((char*)&numEQsPerChanFile, sizeof(uint32_t));
        for (uint32_t i=0; i<numOutChans && i<numOutChansFile; i++) {
            for (uint32_t j=0; j<numEQsPerChan && j<numEQsPerChanFile; j++) {
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setSampleRate(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setGain(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setFreq(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setQFactor(tmpDouble);
                fStr.read((char*)&tmpInt, sizeof(uint32_t));
                EQ.at(i).at(j).setType(tmpInt);
            }
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setThreshold(tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setMakeupGain(tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setReleaseTime(tmpDouble);
        }
        return 0;
    } else {
        return -1;
    }
}
*/
