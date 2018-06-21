#include <QSizePolicy>
#include "mainwindow.h"
#include <stdexcept>

#define NFFT 0x4000
#define MAX_NUM_EQS_PER_CHAN 20

MainWindow::MainWindow(int width, int height, QWidget *parent)
    : QMainWindow(parent), rtIO(nullptr), actChan(0), actEQ(0) {
    QBrush plotBrush(QColor(150,150,150,150));
    QFileInfo currentPath(QCoreApplication::applicationFilePath());
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    QSharedPointer<QCPAxisTickerText> textTickerFreq(new QCPAxisTickerText);

    int uiElementWidth = 0.99/7.0*(double)width;
    int uiElementHeight = 0.99/5.0*(double)height;
    const unsigned int smallestBlockLen = 64;
    const unsigned int numBlockLengths = 8;
    streamFlag = false;
    blockLenIO = 512;
    fs = 48000;
    double loFreq = 20;
    double hiFreq = fs*0.499;

    /*___________________GUI initialization____________________*/

    this->resize(width, height);

    QString tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/logo01.png");
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

    menuBar.setParent(this);
    menuBar.move(0,0);
    menuBar.resize(width, 0.05*height);
    menuBar.setPalette(this->palette());

    blockLenMenu = menuBar.addMenu("Audio block size");
    for(unsigned int i = 0; i<numBlockLengths; i++) {
        blockLenMenu->addAction(QString::number(smallestBlockLen<<i));
        blockLenMenu->actions().back()->setData(smallestBlockLen<<i);
    }

    CppRTA::getHostAPIs(hostAPIs);
    hostAPI = hostAPIs.at(0);
    hostApiMenu = menuBar.addMenu("Host API");
    for(unsigned int i = 0; i<hostAPIs.size(); i++) {
    	hostApiMenu->addAction(QString::fromStdString(hostAPIs.at(i)));
    	hostApiMenu->actions().back()->setData(i);
    }

    CppRTA::getDevices(inDevices, outDevices);
    inDevice = inDevices.at(0);
    outDevice = outDevices.at(0);
    rtIO = new CppRTA(inDevice, outDevice, blockLenIO, fs);
    channelWidget.setLimits(1, outDevice.numChans);

    inDeviceMenu = menuBar.addMenu("Input device");
    outDeviceMenu = menuBar.addMenu("Output device");;
    this->deviceMenuUpdate();

    inOutButton.setParent(this);
    inOutButton.move(0.25*uiElementWidth, 0.075*uiElementWidth+0.05*height);
    inOutButton.resize(0.5*uiElementWidth, 0.4*uiElementHeight);
    inOutButton.setIconSize(inOutButton.size()*0.95);
    inOutButton.setIcon(inOutIcon);

    tenTimesButton.setParent(this);
    tenTimesButton.move(0.25*uiElementWidth, 0.525*uiElementWidth+0.05*height);
    tenTimesButton.resize(0.5*uiElementWidth, 0.4*uiElementHeight);
    tenTimesButton.setCheckable(true);
    tenTimesButton.setText("10x");

    channelWidget.setParent(this);
    channelWidget.move(1.0*uiElementWidth, 0.05*height);
    channelWidget.resize(uiElementWidth, uiElementHeight);
    channelWidget.setLabel(QString("Channel Nr."));
    channelWidget.setUnit(QString(""));
    channelWidget.setLimits(1, 1);
    channelWidget.setStepSize(1.0);
    channelWidget.setValue(1);
    channelWidget.setReadOnly(true);

    eqNrWidget.setParent(this);
    eqNrWidget.move(2.0*uiElementWidth, 0.05*height);
    eqNrWidget.resize(uiElementWidth, uiElementHeight);
    eqNrWidget.setLabel(QString("EQ Nr."));
    eqNrWidget.setUnit(QString(""));
    eqNrWidget.setLimits(1, MAX_NUM_EQS_PER_CHAN);
    eqNrWidget.setStepSize(1.0);
    eqNrWidget.setValue(1);
    eqNrWidget.setReadOnly(true);

    eqGainWidget.setParent(this);
    eqGainWidget.move(3.0*uiElementWidth, 0.05*height);
    eqGainWidget.resize(uiElementWidth, uiElementHeight);
    eqGainWidget.setLabel(QString("EQ Gain"));
    eqGainWidget.setUnit(QString("dB"));
    eqGainWidget.setLimits(-20.0, 20.0);
    eqGainWidget.setStepSize(0.5);
    eqGainWidget.setValue(0.0);

    eqFreqWidget.setParent(this);
    eqFreqWidget.move(4.0*uiElementWidth, 0.05*height);
    eqFreqWidget.resize(uiElementWidth, uiElementHeight);
    eqFreqWidget.setLabel(QString("EQ Freq"));
    eqFreqWidget.setUnit(QString("Hz"));
    eqFreqWidget.setLimits(20.0, std::min(fs*0.5, 20000.0));
    eqFreqWidget.setStepSize(1.01);
    eqFreqWidget.setValue(1000);
    eqFreqWidget.setPrecision(0.1);
    eqFreqWidget.setLogScaling(true);

    eqQFactWidget.setParent(this);
    eqQFactWidget.move(5.0*uiElementWidth, 0.05*height);
    eqQFactWidget.resize(uiElementWidth, uiElementHeight);
    eqQFactWidget.setLabel(QString("EQ QFact"));
    eqQFactWidget.setUnit(QString("NV"));
    eqQFactWidget.setLimits(0.1, 10);
    eqQFactWidget.setStepSize(0.01);
    eqQFactWidget.setValue(1.0);

    eqTypeWidget.setParent(this);
    eqTypeWidget.move(6.0*uiElementWidth, 0.05*height);
    eqTypeWidget.resize(uiElementWidth, uiElementHeight);
    eqTypeWidget.setLabel(QString("EQ Type"));
    eqTypeWidget.setUnit(QString(""));
    eqTypeWidget.setLimits(1, 8);
    eqTypeWidget.setStepSize(1);
    eqTypeWidget.setValue(6);
    eqTypeWidget.setReadOnly(true);

    limitThresWidget.setParent(this);
    limitThresWidget.move(4.0*uiElementWidth, 1.0*uiElementHeight+0.05*height);
    limitThresWidget.resize(uiElementWidth, uiElementHeight);
    limitThresWidget.setLabel(QString("Lim. Thres"));
    limitThresWidget.setUnit(QString("dB"));
    limitThresWidget.setLimits(-50, 0);
    limitThresWidget.setStepSize(0.5);
    limitThresWidget.setValue(0);

    limitMakeupWidget.setParent(this);
    limitMakeupWidget.move(5.0*uiElementWidth, 1.0*uiElementHeight+0.05*height);
    limitMakeupWidget.resize(uiElementWidth, uiElementHeight);
    limitMakeupWidget.setLabel(QString("Lim. Gain"));
    limitMakeupWidget.setUnit(QString("dB"));
    limitMakeupWidget.setLimits(-20.0, 20.0);
    limitMakeupWidget.setStepSize(0.5);
    limitMakeupWidget.setValue(0);

    limitRelWidget.setParent(this);
    limitRelWidget.move(6.0*uiElementWidth, 1.0*uiElementHeight+0.05*height);
    limitRelWidget.resize(uiElementWidth, uiElementHeight);
    limitRelWidget.setLabel(QString("Lim. Rel"));
    limitRelWidget.setUnit(QString("sec"));
    limitRelWidget.setLimits(0.01, 10.0);
    limitRelWidget.setStepSize(0.01);
    limitRelWidget.setValue(2.0);

    tfPlot.setParent(this);
    tfPlot.move(0, 1.0*uiElementHeight+0.05*height);
    tfPlot.resize(4*uiElementWidth, 2*uiElementHeight);
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

    statusTxt.setParent(this);
    statusTxt.move(0.1*uiElementWidth, 3.1*uiElementHeight+0.05*height);
    statusTxt.resize(3.9*uiElementWidth, 1.53*uiElementHeight);
    statusTxt.setText(QString("Welcome to virtualDSP!"));
    statusTxt.setPalette(this->palette());
    statusTxt.viewport()->setAutoFillBackground(false);
    statusTxt.setReadOnly(true);

    this->resizeChannelParams();
    this->resizeAllEQParams();
    this->updateLimiterWidgets();
    this->updateEQWidgets();

    connect(&inOutButton, SIGNAL (clicked()), this, SLOT(inOutButtonHandle()));
    connect(&tenTimesButton, SIGNAL (clicked()), this, SLOT(tenTimesButtonHandle()));
    connect(&channelWidget, SIGNAL (valueChanged(double)), this, SLOT(channelWidgetHandle(double)));

    connect(&eqNrWidget, SIGNAL (valueChanged(double)), this, SLOT(eqNrWidgetHandle(double)));
    connect(&eqGainWidget, SIGNAL (valueChanged(double)), this, SLOT(eqGainWidgetHandle(double)));
    connect(&eqFreqWidget, SIGNAL (valueChanged(double)), this, SLOT(eqFreqWidgetHandle(double)));
    connect(&eqQFactWidget, SIGNAL (valueChanged(double)), this, SLOT(eqQFactWidgetHandle(double)));
    connect(&eqTypeWidget, SIGNAL (valueChanged(double)), this, SLOT(eqTypeWidgetHandle(double)));
    connect(&limitThresWidget, SIGNAL (valueChanged(double)), this, SLOT(limitThresWidgetHandle(double)));
    connect(&limitRelWidget, SIGNAL (valueChanged(double)), this, SLOT(limitRelWidgetHandle(double)));
    connect(&limitMakeupWidget, SIGNAL (valueChanged(double)), this, SLOT(limitMakeupWidgetHandle(double)));

    if (blockLenMenu != nullptr && hostApiMenu != nullptr && inDeviceMenu != nullptr && outDeviceMenu != nullptr) {
		connect(blockLenMenu, SIGNAL(triggered(QAction*)), this, SLOT(blockLenMenuHandle(QAction*)));
		connect(hostApiMenu, SIGNAL(triggered(QAction*)), this, SLOT(hostApiMenuHandle(QAction*)));
		connect(inDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(inDeviceMenuHandle(QAction*)));
		connect(outDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(outDeviceMenuHandle(QAction*)));
    }
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

void MainWindow::tenTimesButtonHandle() {
    if (tenTimesButton.isChecked()) {
        eqGainWidget.setStepSize(5.0);
        eqFreqWidget.setStepSize(1.1);
        eqQFactWidget.setStepSize(0.1);
    } else {
        eqGainWidget.setStepSize(0.5);
        eqFreqWidget.setStepSize(1.01);
        eqQFactWidget.setStepSize(0.01);
    }
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

void MainWindow::deviceMenuUpdate() {
	if (inDeviceMenu != nullptr) {
		inDeviceMenu->clear();
	}
    for(unsigned int i = 0; i<inDevices.size(); i++) {
    	if (inDevices.at(i).hostAPI == hostAPI) {
			inDeviceMenu->addAction(QString::fromStdString(inDevices.at(i).name));
			inDeviceMenu->actions().back()->setData(i);
    	}
    }

    if (outDeviceMenu != nullptr) {
		outDeviceMenu->clear();
	}
    for(unsigned int i = 0; i<outDevices.size(); i++) {
    	if (inDevices.at(i).hostAPI == hostAPI) {
			outDeviceMenu->addAction(QString::fromStdString(outDevices.at(i).name));
			outDeviceMenu->actions().back()->setData(i);
    	}
    }
}

void MainWindow::blockLenMenuHandle(QAction *currentAction) {
    blockLenIO = currentAction->data().toInt();
    if (streamFlag) {
        resetRtIO();
    }
}

void MainWindow::hostApiMenuHandle(QAction *currentAction) {
    hostAPI = hostAPIs.at(currentAction->data().toInt());
    this->deviceMenuUpdate();
    statusTxt.setText(QString("Changed host API to ") + QString::fromStdString(hostAPI));
}

void MainWindow::inDeviceMenuHandle(QAction *currentAction) {
    inDevice = inDevices.at(currentAction->data().toInt());
    this->paramReset();
    if (streamFlag) {
        resetRtIO();
    }
    statusTxt.setText(QString("Changed input device to ") + QString::fromStdString(inDevice.name));
}

void MainWindow::outDeviceMenuHandle(QAction *currentAction) {
    outDevice = outDevices.at(currentAction->data().toInt());
    channelWidget.setLimits(1, outDevice.numChans);
    channelWidget.setValue(1);
    this->paramReset();
    if (streamFlag) {
        resetRtIO();
    }
    statusTxt.setText(QString("Changed output device to ") + QString::fromStdString(outDevice.name));
}

void MainWindow::channelWidgetHandle(double chanNr) {
    if (actChan<outDevice.numChans) {
        actChan = (unsigned int) chanNr-1;
        if (gain.at(actChan).size() < numEQs.at(actChan)) {
            this->resizeActEQParams();
        }
        this->updateLimiterWidgets();
        this->updateEQWidgets();
        this->plotUpdate();
    } else {
        actChan=outDevice.numChans-1;
    }
    statusTxt.setText(QString("Set channel nr. to ") + QString::number(actChan+1));
}

void MainWindow::eqNrWidgetHandle(double eqNr) {
    actEQ.at(actChan) = (unsigned int) eqNr-1;
    if (actEQ.at(actChan) >= numEQs.at(actChan)) {
        numEQs.at(actChan) = actEQ.at(actChan)+1;
        this->resizeActEQParams();
        if (rtIO != nullptr) {
            rtIO->setNumEQs(actChan, numEQs.at(actChan));
        }
    } else if ((int32_t)actEQ.at(actChan) < (int32_t)numEQs.at(actChan)-1
    		&& gain.at(actChan).back() == 0.0 && type.at(actChan).back() > 4) {
        numEQs.at(actChan) = actEQ.at(actChan)+1;
        this->resizeActEQParams();
        if (rtIO != nullptr) {
            rtIO->setNumEQs(actChan, numEQs.at(actChan));
        }
    }
    this->updateEQWidgets();
    this->plotUpdate();
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

void MainWindow::paramReset() {
    actChan = 0;
    this->resizeChannelParams();
    this->resizeAllEQParams();
    for(unsigned int i=0; i<outDevice.numChans; i++) {
        actEQ.at(i) = 0;
    }
    this->updateLimiterWidgets();
    this->updateEQWidgets();
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
        this->resizeChannelParams();
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
        this->resizeActEQParams();
    }
}

void MainWindow::resizeAllEQParams() {
    for (unsigned int i=0; i<outDevice.numChans; i++) {
        gain.at(i).resize(numEQs.at(i), 0.0);
        freq.at(i).resize(numEQs.at(i), 1000.0);
        QFact.at(i).resize(numEQs.at(i), 0.71);
        type.at(i).resize(numEQs.at(i), 5);
    }
}

void MainWindow::resizeActEQParams() {
    unsigned int numEQ = numEQs.at(actChan);
    gain.at(actChan).resize(numEQ, 0.0);
    freq.at(actChan).resize(numEQ, 1000.0);
    QFact.at(actChan).resize(numEQ, 0.71);
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


int MainWindow::storeParams() {
    uint32_t numChans, numEQsPerChan, tmpInt;
    double tmpDouble;
    std::ofstream fStr("params.vdsp", std::ios::binary | std::ios::trunc);

    if (fStr.good()) {
    	fStr.write((char*)&fs, sizeof(uint32_t));
    	fStr.write((char*)&outDevice.numChans, sizeof(uint32_t));
        for (uint32_t i=0; i<outDevice.numChans; i++) {
        	numEQsPerChan = numEQs.at(actChan);
        	fStr.write((char*)&numEQs.at(actChan), sizeof(uint32_t));
            for (uint32_t j=0; j<numEQsPerChan; j++) {
                tmpDouble = gain.at(i).at(j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = freq.at(i).at(j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = QFact.at(i).at(j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpInt = type.at(i).at(j);
                fStr.write((char*)&tmpInt, sizeof(uint32_t));
            }
            tmpDouble = thres.at(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = makeup.at(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = relTime.at(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
        }
        return 0;
    } else {
        return -1;
    }
}

int MainWindow::readParams() {
    uint32_t tmpInt, numOutChansFile, numEQsPerChanFile;
    double tmpDouble;
    std::ifstream fStr("params.vdsp", std::ios::binary);

    this->paramReset();
    if (fStr.good()) {
        fStr.read((char*)&fs, sizeof(uint32_t));
        fStr.read((char*)&numOutChansFile, sizeof(uint32_t));
        for (uint32_t i=0; i<outDevice.numChans && i<numOutChansFile; i++) {
        	fStr.read((char*)&numEQsPerChanFile, sizeof(uint32_t));
        	numEQs.at(actChan) = numEQsPerChanFile;
        	this->resizeActEQParams();
            for (uint32_t j=0; j<numEQsPerChanFile; j++) {
                fStr.read((char*)&tmpDouble, sizeof(double));
                gain.at(i).at(j) = tmpDouble;
                fStr.read((char*)&tmpDouble, sizeof(double));
                freq.at(i).at(j) = tmpDouble;
                fStr.read((char*)&tmpDouble, sizeof(double));
                QFact.at(i).at(j) = tmpDouble;
                fStr.read((char*)&tmpInt, sizeof(uint32_t));
                type.at(i).at(j) = tmpInt;
            }
            fStr.read((char*)&tmpDouble, sizeof(double));
            thres.at(i) = tmpDouble;
            fStr.read((char*)&tmpDouble, sizeof(double));
            makeup.at(i) = tmpDouble;
            fStr.read((char*)&tmpDouble, sizeof(double));
            relTime.at(i) = tmpDouble;
        }
        this->updateParams();
        this->updateLimiterWidgets();
		this->updateEQWidgets();
        return 0;
    } else {
        return -1;
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
