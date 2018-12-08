#include <QSizePolicy>
#include "mainwindow.h"
#include <stdexcept>

#define NFFT 0x8000
#define MAX_ORD 16
#define MAX_NUM_EQS_PER_CHAN 10

MainWindow::MainWindow(int width, int height, QWidget *parent)
    : QMainWindow(parent), rtIO(nullptr), actChan(0), actEQ(0), tenTimesFlag(false), stereoLockFlag(false),
	  settingsMenu(nullptr), blockLenMenu(nullptr), hostApiMenu(nullptr), inDeviceMenu(nullptr), outDeviceMenu(nullptr), copyMenu(nullptr) {
    QBrush plotBrush(QColor(150,150,150,150));
    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    QSharedPointer<QCPAxisTickerText> textTickerFreq(new QCPAxisTickerText);

    const unsigned int numVerticalGrids = 5;
    const unsigned int numHorizontalGrids = 10;
	const unsigned int gridHeight = 0.98*(double)height/numVerticalGrids;
	const unsigned int gridWidth = 0.98*(double)width/numHorizontalGrids;
	const unsigned int leftZeroPos = 0.02*width;
    const unsigned int smallestBlockLen = 64;
	const unsigned int numBlockLengths = 8;
    streamFlag = false;
    blockLenIO = 512;
    fs = 44100;
    double loFreq = 20;
    double hiFreq = qMin(20000.0, fs*0.499);

    /*___________________GUI initialization____________________*/

    this->resize(width, height);

    QString tmpStr = QCoreApplication::applicationDirPath();
    tmpStr.append("/symbols/logo01.png");
    applicationIcon.addFile(tmpStr);

    tmpStr = QCoreApplication::applicationDirPath();
    tmpStr.append("/symbols/InOut.png");
    inOutIcon.addFile(tmpStr);

    tmpStr = QCoreApplication::applicationDirPath();
    tmpStr.append("/symbols/pause.png");
    pauseIcon.addFile(tmpStr);

    this->setWindowIcon(applicationIcon);
    this->setWindowTitle("virtualDSP");
    this->setAcceptDrops(false);

    menuBar.setParent(this);
    menuBar.move(0,0);
    menuBar.resize(width, gridHeight/4);
    menuBar.setPalette(this->palette());

    settingsMenu = menuBar.addMenu("Settings");
    settingsMenu->addAction(QString("Stereo lock (inactive)"));
    settingsMenu->actions().back()->setData(0);
    settingsMenu->addAction(QString("Ten fold step size (inactive)"));
	settingsMenu->actions().back()->setData(1);
    settingsMenu->addAction(QString("Store setting"));
    settingsMenu->actions().back()->setData(2);
    settingsMenu->addAction(QString("Store default setting"));
	settingsMenu->actions().back()->setData(3);
    settingsMenu->addAction(QString("Load setting"));
    settingsMenu->actions().back()->setData(4);

    blockLenMenu = menuBar.addMenu("Audio block size");
    for(unsigned int i = 0; i<numBlockLengths; i++) {
        blockLenMenu->addAction(QString::number(smallestBlockLen<<i));
        blockLenMenu->actions().back()->setData(smallestBlockLen<<i);
    }

    sampleRateMenu = menuBar.addMenu("Sample rate");
    QVector<unsigned int> possibleSamplingRates({16000, 22050, 32000, 44100, 48000, 88200, 96000, 192000});
    for(int i = 0; i<possibleSamplingRates.size(); i++) {
        sampleRateMenu->addAction(QString::number(possibleSamplingRates.at(i)));
        sampleRateMenu->actions().back()->setData(possibleSamplingRates.at(i));
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
    outDeviceMenu = menuBar.addMenu("Output device");
    this->deviceMenuUpdate();

    copyMenu = menuBar.addMenu("Copy to Channel");
    this->copyMenuUpdate();

    inOutButton.setParent(this);
    inOutButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    inOutButton.setIcon(inOutIcon);
    inOutButton.move(leftZeroPos,gridHeight/2);
    inOutButton.resize(gridWidth/2,gridHeight/2);
    inOutButton.setIconSize(0.95*inOutButton.size());

    channelWidget.setParent(this);
    channelWidget.move(1*gridWidth,gridHeight/4);
    channelWidget.resize(gridWidth,gridHeight);
    channelWidget.setLabel(QString("Chan Nr."));
    channelWidget.setUnit(QString(""));
    channelWidget.setLimits(1, outDevice.numChans);
    channelWidget.setStepSize(1);
    channelWidget.setValue(1);
    channelWidget.setReadOnly(true);

    eqNrWidget.setParent(this);
    eqNrWidget.move(5*gridWidth,gridHeight/4);
    eqNrWidget.resize(gridWidth,gridHeight);
    eqNrWidget.setLabel(QString("EQ Nr."));
    eqNrWidget.setUnit(QString(""));
    eqNrWidget.setLimits(1, MAX_NUM_EQS_PER_CHAN);
    eqNrWidget.setStepSize(1.0);
    eqNrWidget.setValue(1);
    eqNrWidget.setReadOnly(true);

    eqGainWidget.setParent(this);
    eqGainWidget.move(6*gridWidth,gridHeight/4);
    eqGainWidget.resize(gridWidth,gridHeight);
    eqGainWidget.setLabel(QString("EQ Gain"));
    eqGainWidget.setUnit(QString("dB"));
    eqGainWidget.setLimits(-20.0, 20.0);
    eqGainWidget.setStepSize(0.5);
    eqGainWidget.setValue(0.0);

    eqFreqWidget.setParent(this);
    eqFreqWidget.move(7*gridWidth,gridHeight/4);
    eqFreqWidget.resize(gridWidth,gridHeight);
    eqFreqWidget.setLabel(QString("EQ Freq"));
    eqFreqWidget.setUnit(QString("Hz"));
    eqFreqWidget.setLimits(20.0, qMin(fs*0.5, 20000.0));
    eqFreqWidget.setStepSize(1.01);
    eqFreqWidget.setValue(1000);
    eqFreqWidget.setPrecision(0.1);
    eqFreqWidget.setLogScaling(true);

    eqQFactWidget.setParent(this);
    eqQFactWidget.move(8*gridWidth,gridHeight/4);
    eqQFactWidget.resize(gridWidth,gridHeight);
    eqQFactWidget.setLabel(QString("EQ QFact"));
    eqQFactWidget.setUnit(QString("NV"));
    eqQFactWidget.setLimits(0.1, 10);
    eqQFactWidget.setStepSize(0.01);
    eqQFactWidget.setValue(1.0);

    eqTypeWidget.setParent(this);
    eqTypeWidget.move(9*gridWidth,gridHeight/4);
    eqTypeWidget.resize(gridWidth,gridHeight);
    eqTypeWidget.setLabel(QString("EQ Type"));
    eqTypeWidget.setUnit(QString(""));
    eqTypeWidget.setLimits((int) ALLPASS, (int) HIGHSHELV);
    eqTypeWidget.setStepSize(1);
    eqTypeWidget.setValue(PEAKEQ);
    eqTypeWidget.setReadOnly(true);
    eqTypeWidget.setValueText(QString::fromStdString(std::string(CppEQ::getTypeName(PEAKEQ))));

    hiPassCharWidget.setParent(this);
    hiPassCharWidget.move(7*gridWidth,5*gridHeight/4);
    hiPassCharWidget.resize(gridWidth,gridHeight);
    hiPassCharWidget.setLabel(QString("HP Char."));
    hiPassCharWidget.setUnit(QString(""));
    hiPassCharWidget.setLimits(FLAT_THRU, CHEBYSHEV2);
    hiPassCharWidget.setStepSize(1);
    hiPassCharWidget.setValue(FLAT_THRU);
    hiPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(FLAT_THRU))));

    hiPassFreqWidget.setParent(this);
    hiPassFreqWidget.move(8*gridWidth,5*gridHeight/4);
    hiPassFreqWidget.resize(gridWidth,gridHeight);
    hiPassFreqWidget.setLabel(QString("HP Freq"));
    hiPassFreqWidget.setUnit(QString("Hz"));
    hiPassFreqWidget.setLimits(20.0, qMin(fs*0.5, 20000.0));
    hiPassFreqWidget.setStepSize(1.01);
    hiPassFreqWidget.setValue(1000);
    hiPassFreqWidget.setPrecision(0.1);
    hiPassFreqWidget.setLogScaling(true);

    hiPassOrdWidget.setParent(this);
    hiPassOrdWidget.move(9*gridWidth,5*gridHeight/4);
    hiPassOrdWidget.resize(gridWidth,gridHeight);
    hiPassOrdWidget.setLabel(QString("HP Order"));
    hiPassOrdWidget.setUnit(QString(""));
    hiPassOrdWidget.setLimits(1, MAX_ORD);
    hiPassOrdWidget.setStepSize(1);
    hiPassOrdWidget.setValue(4);

    loPassCharWidget.setParent(this);
    loPassCharWidget.move(7*gridWidth,9*gridHeight/4);
    loPassCharWidget.resize(gridWidth,gridHeight);
    loPassCharWidget.setLabel(QString("LP Char."));
    loPassCharWidget.setUnit(QString(""));
    loPassCharWidget.setLimits(FLAT_THRU, CHEBYSHEV2);
    loPassCharWidget.setStepSize(1);
    loPassCharWidget.setValue(FLAT_THRU);
    loPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(FLAT_THRU))));

    loPassFreqWidget.setParent(this);
    loPassFreqWidget.move(8*gridWidth,9*gridHeight/4);
    loPassFreqWidget.resize(gridWidth,gridHeight);
    loPassFreqWidget.setLabel(QString("LP Freq"));
    loPassFreqWidget.setUnit(QString("Hz"));
    loPassFreqWidget.setLimits(20.0, qMin(fs*0.5, 20000.0));
    loPassFreqWidget.setStepSize(1.01);
    loPassFreqWidget.setValue(1000);
    loPassFreqWidget.setPrecision(0.1);
    loPassFreqWidget.setLogScaling(true);

    loPassOrdWidget.setParent(this);
    loPassOrdWidget.move(9*gridWidth,9*gridHeight/4);
    loPassOrdWidget.resize(gridWidth,gridHeight);
    loPassOrdWidget.setLabel(QString("LP Order"));
    loPassOrdWidget.setUnit(QString(""));
    loPassOrdWidget.setLimits(1, MAX_ORD);
    loPassOrdWidget.setStepSize(1);
    loPassOrdWidget.setValue(4);

    limitThresWidget.setParent(this);
    limitThresWidget.move(7*gridWidth,13*gridHeight/4);
    limitThresWidget.resize(gridWidth,gridHeight);
    limitThresWidget.setLabel(QString("Lim. Thres"));
    limitThresWidget.setUnit(QString("dB"));
    limitThresWidget.setLimits(-50, 0);
    limitThresWidget.setStepSize(0.5);
    limitThresWidget.setValue(0);

    limitMakeupWidget.setParent(this);
    limitMakeupWidget.move(8*gridWidth,13*gridHeight/4);
    limitMakeupWidget.resize(gridWidth,gridHeight);
    limitMakeupWidget.setLabel(QString("Lim. Gain"));
    limitMakeupWidget.setUnit(QString("dB"));
    limitMakeupWidget.setLimits(-20.0, 20.0);
    limitMakeupWidget.setStepSize(0.5);
    limitMakeupWidget.setValue(0);

    limitRelWidget.setParent(this);
    limitRelWidget.move(9*gridWidth,13*gridHeight/4);
    limitRelWidget.resize(gridWidth,gridHeight);
    limitRelWidget.setLabel(QString("Lim. Rel"));
    limitRelWidget.setUnit(QString("sec"));
    limitRelWidget.setLimits(0.01, 10.0);
    limitRelWidget.setStepSize(0.01);
    limitRelWidget.setValue(2.0);

    tfPlot.setParent(this);
    tfPlot.move(leftZeroPos,5*gridHeight/4);
    tfPlot.resize(6*gridWidth, 2*gridHeight);
    tfPlot.addGraph();
    tfPlot.graph(0)->setPen(QColor(50,50,230));
    tfPlot.graph(0)->setBrush(plotBrush);
    tfPlot.xAxis->setScaleType(QCPAxis::stLogarithmic);
    tfPlot.xAxis->setTicker(logTicker);
    tfPlot.xAxis->setRange(loFreq,hiFreq);
    tfPlot.yAxis->setRange(-45,25);
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
    statusTxt.move(leftZeroPos,13*gridHeight/4);
    statusTxt.resize(6*gridWidth, 7*gridHeight/4);
    statusTxt.setPalette(this->palette());
    statusTxt.viewport()->setAutoFillBackground(false);
    statusTxt.setReadOnly(true);
    statusTxt.setMaximumBlockCount(25);

    actEQ.resize(outDevice.numChans, 0);
    this->updateEQWidgets();
    this->updateCutWidgets();
    this->updateLimiterWidgets();

    connect(&inOutButton, SIGNAL (clicked()), this, SLOT(inOutButtonHandle()));
    connect(&channelWidget, SIGNAL (valueChanged(double)), this, SLOT(channelWidgetHandle(double)));

    connect(&eqNrWidget, SIGNAL (valueChanged(double)), this, SLOT(eqNrWidgetHandle(double)));
    connect(&eqGainWidget, SIGNAL (valueChanged(double)), this, SLOT(eqGainWidgetHandle(double)));
    connect(&eqFreqWidget, SIGNAL (valueChanged(double)), this, SLOT(eqFreqWidgetHandle(double)));
    connect(&eqQFactWidget, SIGNAL (valueChanged(double)), this, SLOT(eqQFactWidgetHandle(double)));
    connect(&eqTypeWidget, SIGNAL (valueChanged(double)), this, SLOT(eqTypeWidgetHandle(double)));
    connect(&hiPassCharWidget, SIGNAL (valueChanged(double)), this, SLOT(hiPassCharWidgetHandle(double)));
    connect(&hiPassFreqWidget, SIGNAL (valueChanged(double)), this, SLOT(hiPassFreqWidgetHandle(double)));
    connect(&hiPassOrdWidget, SIGNAL (valueChanged(double)), this, SLOT(hiPassOrdWidgetHandle(double)));
    connect(&loPassCharWidget, SIGNAL (valueChanged(double)), this, SLOT(loPassCharWidgetHandle(double)));
    connect(&loPassFreqWidget, SIGNAL (valueChanged(double)), this, SLOT(loPassFreqWidgetHandle(double)));
    connect(&loPassOrdWidget, SIGNAL (valueChanged(double)), this, SLOT(loPassOrdWidgetHandle(double)));
    connect(&limitThresWidget, SIGNAL (valueChanged(double)), this, SLOT(limitThresWidgetHandle(double)));
    connect(&limitRelWidget, SIGNAL (valueChanged(double)), this, SLOT(limitRelWidgetHandle(double)));
    connect(&limitMakeupWidget, SIGNAL (valueChanged(double)), this, SLOT(limitMakeupWidgetHandle(double)));

    if (settingsMenu != nullptr) {
    	connect(settingsMenu, SIGNAL(triggered(QAction*)), this, SLOT(settingsMenuHandle(QAction*)));
    }
    if (blockLenMenu != nullptr) {
    	connect(blockLenMenu, SIGNAL(triggered(QAction*)), this, SLOT(blockLenMenuHandle(QAction*)));
	}
    if (sampleRateMenu != nullptr) {
        connect(sampleRateMenu, SIGNAL(triggered(QAction*)), this, SLOT(sampleRateMenuHandle(QAction*)));
    }
    if (hostApiMenu != nullptr) {
		connect(hostApiMenu, SIGNAL(triggered(QAction*)), this, SLOT(hostApiMenuHandle(QAction*)));
    }
    if (inDeviceMenu != nullptr) {
		connect(inDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(inDeviceMenuHandle(QAction*)));
    }
	if (outDeviceMenu != nullptr) {
		connect(outDeviceMenu, SIGNAL(triggered(QAction*)), this, SLOT(outDeviceMenuHandle(QAction*)));
	}
	if (copyMenu != nullptr) {
		connect(copyMenu, SIGNAL(triggered(QAction*)), this, SLOT(copyMenuHandle(QAction*)));
	}

	this->loadParams("default_params.vdsp");
    statusTxt.clear();
    statusTxt.appendPlainText(QString("Welcome to virtualDSP!") + QString("\n"));
    this->plotUpdate();
}

void MainWindow::inOutButtonHandle() {
    statusTxt.appendPlainText("inOutButtonHandle: Round trip Button Toggled." + QString("\n"));

    if (!streamFlag) {
        inOutButton.setIcon(pauseIcon);
        if (rtIO != nullptr) {
        	this->storeParams("params.vdsp");
            delete rtIO;
            rtIO = nullptr;
        }
        try {
            rtIO = new CppRTA(inDevice, outDevice, blockLenIO, fs);
            rtIO->startStream();
            this->loadParams("params.vdsp");
            statusTxt.appendPlainText(QString("inOutButtonHandle: Audio round trip started.") + QString("\n"));
        } catch (const std::exception& err) {
            statusTxt.appendPlainText(QString("inOutButtonHandle: Audio initialization error:\n")+QString(err.what())
                             +QString("\nYou may try another device combination and/or sampling rate.") + QString("\n"));
        }
    }
    else {
        inOutButton.setIcon(inOutIcon);
        statusTxt.appendPlainText("inOutButtonHandle: Audio round trip paused." + QString("\n"));
        if (rtIO != nullptr) {
            rtIO->stopStream();
        }  else {
        	statusTxt.appendPlainText(QString("inOutButtonHandle: Error stopping play back: Audio instance not initialized. Try restarting."));
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
    } else {
    	statusTxt.appendPlainText(QString("plotUpdate: Error on plot update: Audio instance not initialized. Try restarting."));
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
    	if (outDevices.at(i).hostAPI == hostAPI) {
			outDeviceMenu->addAction(QString::fromStdString(outDevices.at(i).name));
			outDeviceMenu->actions().back()->setData(i);
    	}
    }
}

void MainWindow::copyMenuUpdate() {
	if (copyMenu != nullptr) {
		copyMenu->clear();
	}
	for(unsigned int i = 0; i<outDevice.numChans; i++) {
		copyMenu->addAction(QString::number(i+1));
		copyMenu->actions().back()->setData(i);
	}
}

void MainWindow::settingsMenuHandle(QAction *currentAction) {
	int caseVal = currentAction->data().toInt();
	QString tmpTxt = currentAction->text();

	if (caseVal==0) {
		stereoLockFlag = !stereoLockFlag;
		if (stereoLockFlag) {
			actChan = actChan/2*2;
			channelWidget.setValue(actChan);
            channelWidget.setValueText(QString::number(actChan+1)+"+"+QString::number(actChan+2));
			channelWidget.setLimits(1, outDevice.numChans-1);
			channelWidget.setStepSize(2);
            statusTxt.appendPlainText(QString("settingsMenuHandle: Activated stereo lock.") + QString("\n"));
            currentAction->setText(tmpTxt.replace("(inactive)", "(active)"));
		} else {
			channelWidget.setLimits(1, outDevice.numChans);
			channelWidget.setStepSize(1);
            statusTxt.appendPlainText(QString("settingsMenuHandle: Deactivated stereo lock.") + QString("\n"));
            currentAction->setText(tmpTxt.replace("(active)", "(inactive)"));
		}
	}

	if (caseVal==1) {
		tenTimesFlag = !tenTimesFlag;
		if (tenTimesFlag) {
			eqGainWidget.setStepSize(5.0);
			eqFreqWidget.setStepSize(1.1);
			eqQFactWidget.setStepSize(0.1);
			currentAction->setText(tmpTxt.replace("(inactive)", "(active)"));
            statusTxt.appendPlainText(QString("settingsMenuHandle: Activated ten fold step size.") + QString("\n"));
		} else {
			eqGainWidget.setStepSize(0.5);
			eqFreqWidget.setStepSize(1.01);
			eqQFactWidget.setStepSize(0.01);
			currentAction->setText(tmpTxt.replace("(active)", "(inactive)"));
            statusTxt.appendPlainText(QString("settingsMenuHandle: Deactivated ten fold step size.") + QString("\n"));
		}
	}

	if (caseVal==2) {
		this->storeParams("params.vdsp");
	}

	if (caseVal==3) {
		this->storeParams("default_params.vdsp");
	}

	if (caseVal==4) {
		this->loadParams("params.vdsp");
	}
}

void MainWindow::blockLenMenuHandle(QAction *currentAction) {
    blockLenIO = currentAction->data().toInt();
    if (streamFlag) {
    	inOutButtonHandle();
    }
    statusTxt.appendPlainText("blockLenMenuHandle: Set block length to <" + QString::number(blockLenIO) + QString(">.\n"));
}

void MainWindow::sampleRateMenuHandle(QAction *currentAction) {
	double loFreq, hiFreq;

	fs = currentAction->data().toInt();
	loFreq = 20.0;
    hiFreq = qMin(20000.0, fs*0.499);
    tfPlot.xAxis->setRange(loFreq,hiFreq);

    for (int i=0; i<xPlot.size(); i++) {
        xPlot[i] = i*(fs/2)/(NFFT/2.0+1.0);
    }
    if (streamFlag) {
        inOutButtonHandle();
    } else {
    	inOutButtonHandle();
    	inOutButtonHandle();
    }
    this->plotUpdate();
    statusTxt.appendPlainText("sampleRateMenuHandle: Set sampling rate to <" + QString::number(fs) + QString(">.\n"));
}

void MainWindow::hostApiMenuHandle(QAction *currentAction) {
    hostAPI = hostAPIs.at(currentAction->data().toInt());
    if (streamFlag) {
    	inOutButtonHandle();
    }
    for (int i=0; i<inDevices.size() && inDevice.hostAPI != hostAPI; i++) {
    	inDevice = inDevices.at(i);
    }
    for (int i=0; i<outDevices.size() && outDevice.hostAPI != hostAPI; i++) {
    	outDevice = outDevices.at(i);
    }

    this->deviceMenuUpdate();
    statusTxt.appendPlainText(QString("hostApiMenuHandle: Changed host API to ") + QString::fromStdString(hostAPI) + QString("\n"));
}

void MainWindow::inDeviceMenuHandle(QAction *currentAction) {
    inDevice = inDevices.at(currentAction->data().toInt());
    if (streamFlag) {
    	inOutButtonHandle();
    }
    statusTxt.appendPlainText(QString("inDeviceMenuHandle: Changed input device to ") + QString::fromStdString(inDevice.name) + QString("\n"));
}

void MainWindow::outDeviceMenuHandle(QAction *currentAction) {
    outDevice = outDevices.at(currentAction->data().toInt());
    channelWidget.setLimits(1, outDevice.numChans);
    actEQ.resize(outDevice.numChans, 0);
    channelWidget.setValue(1);
    if (stereoLockFlag) {
        channelWidget.setValueText(QString::number(actChan+1)+"+"+QString::number(actChan+2));
    }
    if (streamFlag) {
        inOutButtonHandle();
    }
    statusTxt.appendPlainText(QString("outDeviceMenuHandle: Changed output device to ") + QString::fromStdString(outDevice.name) + QString("\n"));
}

void MainWindow::copyMenuHandle(QAction *currentAction) {
	int copyChan = currentAction->data().toInt();
	rtIO->setNumEQs(copyChan, rtIO->getNumEQs(actChan));
	for (unsigned int i=0; i<rtIO->getNumEQs(copyChan); i++) {
		rtIO->setEqGain(copyChan, i, rtIO->getEqGain(actChan, i));
		rtIO->setEqFrequency(copyChan, i, rtIO->getEqFrequency(actChan, i));
		rtIO->setEqQFactor(copyChan, i, rtIO->getEqQFactor(actChan, i));
		rtIO->setEqType(copyChan, i, rtIO->getEqType(actChan, i));
	}
	rtIO->setThreshold(copyChan, rtIO->getThreshold(actChan));
	rtIO->setMakeupGain(copyChan, rtIO->getMakeupGain(actChan));
	rtIO->setReleaseTime(copyChan, rtIO->getReleaseTime(actChan));
    statusTxt.appendPlainText(QString("copyMenuHandle: Copied settings from channel <") + QString::number(actChan+1)
                    + QString("> to <") + QString::number(copyChan+1) + QString(">.") + QString("\n"));
}

void MainWindow::channelWidgetHandle(double chanNr) {
    if (chanNr<=outDevice.numChans && chanNr > 0) {
        actChan = (unsigned int) chanNr-1;
        this->updateLimiterWidgets();
        this->updateEQWidgets();
        this->updateCutWidgets();
        this->plotUpdate();
        if (stereoLockFlag) {
            channelWidget.setValueText(QString::number(actChan+1)+"+"+QString::number(actChan+2));
        }
    } else {
    	statusTxt.appendPlainText(QString("channelWidgetHandle: Critical warning: Channel number greater than output ")
    			 	 + QString("device channels, or channel number smaller than 1 detected."));
    }
    statusTxt.appendPlainText(QString("channelWidgetHandle: Set channel nr. to ") + QString::number(actChan+1) + QString("\n"));
}

void MainWindow::eqNrWidgetHandle(double eqNr) {
    actEQ.at(actChan) = (unsigned int) eqNr-1;
    if (rtIO->getNumEQs(actChan) < eqNr) {
        if (rtIO != nullptr) {
            rtIO->setNumEQs(actChan, eqNr);
        } else {
        	statusTxt.appendPlainText(QString("eqNrWidgetHandle: Error setting EQ Number: Audio instance not initialized. Try restarting."));
        }
    } else {
    	unsigned int i=0;
    	for (i=rtIO->getNumEQs(actChan)-1; qAbs(rtIO->getEqGain(actChan, i))<0.01 && rtIO->getEqType(actChan, i)>NOTCH && i>=eqNr; i--) {
			rtIO->setNumEQs(actChan, i);
    	}
    }
    if (stereoLockFlag && actChan+1<outDevice.numChans) {
        actEQ.at(actChan+1) = (unsigned int) eqNr-1;
        if (rtIO->getNumEQs(actChan+1) < eqNr) {
            if (rtIO != nullptr) {
                rtIO->setNumEQs(actChan+1, eqNr);
            }  else {
            	statusTxt.appendPlainText(QString("eqNrWidgetHandle: Error setting EQ Number: Audio instance not initialized. Try restarting."));
            }
        } else {
        	unsigned int i=0;
        	for (i=rtIO->getNumEQs(actChan+1)-1; qAbs(rtIO->getEqGain(actChan+1, i))<0.01 && rtIO->getEqType(actChan+1, i)>NOTCH && i>eqNr; i--) {}
        	rtIO->setNumEQs(actChan+1, i);
        }
    }
    this->updateEQWidgets();
    this->plotUpdate();
    statusTxt.appendPlainText(QString("eqNrWidgetHandle: Set EQ nr. of channel ") + QString::number(actChan+1)
                      + QString(" to ") + QString::number(actEQ.at(actChan)+1) + QString("\n"));
}

void MainWindow::eqGainWidgetHandle(double gain) {
    if (rtIO != nullptr) {
        rtIO->setEqGain(actChan, actEQ.at(actChan), gain);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setEqGain(actChan+1, actEQ.at(actChan+1), gain);
        }
        statusTxt.appendPlainText(QString("eqGainWidgetHandle: Set gain of EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                          + QString::number(actChan+1) + QString(" to ") + QString::number(rtIO->getEqGain(actChan, actEQ.at(actChan)))
                          + QString(", ") + QString::number(rtIO->getNumEQs(actChan)) + QString(" EQs allocated.") + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("eqGainWidgetHandle: Error setting EQ Gain: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::eqFreqWidgetHandle(double freq) {
    if (rtIO != nullptr) {
        rtIO->setEqFrequency(actChan, actEQ.at(actChan), freq);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setEqFrequency(actChan+1, actEQ.at(actChan+1), freq);
        }
        statusTxt.appendPlainText(QString("eqFreqWidgetHandle: Set frequency of EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                          + QString::number(actChan+1) + QString(" to ") + QString::number(rtIO->getEqFrequency(actChan, actEQ.at(actChan)))
                          + QString(", ") + QString::number(rtIO->getNumEQs(actChan)) + QString(" EQs allocated.") + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("eqFreqWidgetHandle: Error setting EQ Frequency: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::eqQFactWidgetHandle(double QFact) {
    if (rtIO != nullptr) {
        rtIO->setEqQFactor(actChan, actEQ.at(actChan), QFact);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setEqQFactor(actChan+1, actEQ.at(actChan+1), QFact);
        }
        statusTxt.appendPlainText(QString("eqQFactWidgetHandle: Set QFactor of EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                          + QString::number(actChan+1) + QString(" to ") + QString::number(rtIO->getEqQFactor(actChan, actEQ.at(actChan)))
                          + QString(", ") + QString::number(rtIO->getNumEQs(actChan)) + QString(" EQs allocated.") + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("eqQFactWidgetHandle: Error setting EQ QFactor: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::eqTypeWidgetHandle(double type) {
	eqType castedType = (eqType) ((int) type);
    if (rtIO != nullptr) {
        rtIO->setEqType(actChan, actEQ.at(actChan), castedType);
        this->eqTypeWidget.setValueText(QString::fromStdString(std::string(CppEQ::getTypeName(castedType))));
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setEqType(actChan+1, actEQ.at(actChan+1), castedType);
        }
        statusTxt.appendPlainText(QString("eqTypeWidgetHandle: Set type of EQ ") + QString::number(actEQ.at(actChan)+1) + QString(" of channel ")
                          + QString::number(actChan+1) + QString(" to ")
						  + QString::fromStdString(CppEQ::getTypeName(rtIO->getEqType(actChan, actEQ.at(actChan))))
                          + QString(", ") + QString::number(rtIO->getNumEQs(actChan)) + QString(" EQs allocated.") + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("eqTypeWidgetHandle: Error setting EQ type: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::hiPassCharWidgetHandle(double charac) {
	filterChar castedCharac = (filterChar) ((int) charac);
    if (rtIO != nullptr) {
        rtIO->setCutCharacteristic(HIGHPASS, actChan, castedCharac);
        hiPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(castedCharac))));
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutCharacteristic(HIGHPASS, actChan+1, (filterChar) ((int) charac));
        }
        statusTxt.appendPlainText(QString("hiPassCharWidgetHandle: Set high pass characteristics of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::fromStdString(CppXover::getCharName(rtIO->getCutCharacteristic(HIGHPASS, actChan))) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("hiPassCharWidgetHandle: Error setting high pass characteristics: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::hiPassFreqWidgetHandle(double freq) {
    if (rtIO != nullptr) {
        rtIO->setCutFrequency(HIGHPASS, actChan, freq);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutFrequency(HIGHPASS, actChan+1, freq);
        }
        statusTxt.appendPlainText(QString("hiPassFreqWidgetHandle: Set high pass frequency of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::number(rtIO->getCutFrequency(HIGHPASS, actChan)) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("hiPassFreqWidgetHandle: Error setting high pass frequency: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::hiPassOrdWidgetHandle(double ord) {
    if (rtIO != nullptr) {
        rtIO->setCutOrder(HIGHPASS, actChan, ord);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutOrder(HIGHPASS, actChan+1, ord);
        }
        statusTxt.appendPlainText(QString("hiPassOrdWidgetHandle: Set high pass order of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::number(rtIO->getCutOrder(HIGHPASS, actChan)) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("hiPassOrdWidgetHandle: Error setting high pass order: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::loPassCharWidgetHandle(double charac) {
	filterChar castedCharac = (filterChar) ((int) charac);
    if (rtIO != nullptr) {
        rtIO->setCutCharacteristic(LOWPASS, actChan, castedCharac);
        loPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(castedCharac))));
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutCharacteristic(LOWPASS, actChan+1, (filterChar) ((int) charac));
        }
        statusTxt.appendPlainText(QString("loPassCharWidgetHandle: Set low pass characteristics of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::fromStdString(CppXover::getCharName(rtIO->getCutCharacteristic(LOWPASS, actChan))) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("loPassCharWidgetHandle: Error setting low pass characteristics: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::loPassFreqWidgetHandle(double freq) {
    if (rtIO != nullptr) {
        rtIO->setCutFrequency(LOWPASS, actChan, freq);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutFrequency(LOWPASS, actChan+1, freq);
        }
        statusTxt.appendPlainText(QString("loPassFreqWidgetHandle: Set low pass frequency of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::number(rtIO->getCutFrequency(LOWPASS, actChan)) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("loPassFreqWidgetHandle: Error setting low pass frequency: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::loPassOrdWidgetHandle(double ord) {
    if (rtIO != nullptr) {
        rtIO->setCutOrder(LOWPASS, actChan, ord);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setCutOrder(LOWPASS, actChan+1, ord);
        }
        statusTxt.appendPlainText(QString("loPassOrdWidgetHandle: Set low pass order of channel ") + QString::number(actChan+1)
        						  + QString(" to ") + QString::number(rtIO->getCutOrder(LOWPASS, actChan)) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("loPassOrdWidgetHandle: Error setting low pass order: Audio instance not initialized. Try restarting."));
    }
    plotUpdate();
}

void MainWindow::limitThresWidgetHandle(double thres) {
    if (rtIO != nullptr) {
        rtIO->setThreshold(actChan, thres);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setThreshold(actChan+1, thres);
        }
        statusTxt.appendPlainText(QString("limitThresWidgetHandle: Set limiter threshold of channel ") + QString::number(actChan+1)
						  + QString(" to ") + QString::number(rtIO->getThreshold(actChan)) + QString("\n"));
    } else {
    	statusTxt.appendPlainText(QString("limitThresWidgetHandle: Error setting limiter threshold: Audio instance not initialized. Try restarting."));
    }
}

void MainWindow::limitMakeupWidgetHandle(double gain) {
    if (rtIO != nullptr) {
        rtIO->setMakeupGain(actChan, gain);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setMakeupGain(actChan+1, gain);
        }
        statusTxt.appendPlainText(QString("limitMakeupWidgetHandle: Set limiter makeup gain of channel ") + QString::number(actChan+1)
						  + QString(" to ") + QString::number(rtIO->getMakeupGain(actChan)) + QString("\n"));
    }  else {
    	statusTxt.appendPlainText(QString("limitMakeupWidgetHandle: Error setting limiter makeup gain: Audio instance not initialized. Try restarting."));
    }
}

void MainWindow::limitRelWidgetHandle(double relTime) {
    if (rtIO != nullptr) {
        rtIO->setReleaseTime(actChan, relTime);
        if (stereoLockFlag && actChan+1<outDevice.numChans) {
        	rtIO->setReleaseTime(actChan+1, relTime);
        }
        statusTxt.appendPlainText(QString("limitRelWidgetHandle: Set limiter release time of channel ") + QString::number(actChan+1)
						  + QString(" to ") + QString::number(rtIO->getReleaseTime(actChan)) + QString("\n"));
    }  else {
    	statusTxt.appendPlainText(QString("limitRelWidgetHandle: Error setting limiter release time: Audio instance not initialized. Try restarting."));
    }
}

void MainWindow::updateEQWidgets() {
    unsigned int eqNr = actEQ.at(actChan);
    eqNrWidget.setValue(eqNr+1);
    if (rtIO != nullptr) {
        eqGainWidget.setValue(rtIO->getEqGain(actChan, eqNr));
        eqFreqWidget.setValue(rtIO->getEqFrequency(actChan, eqNr));
        eqQFactWidget.setValue(rtIO->getEqQFactor(actChan, eqNr));
        eqTypeWidget.setValue(rtIO->getEqType(actChan, eqNr));
        eqTypeWidget.setValueText(QString::fromStdString(std::string(CppEQ::getTypeName(rtIO->getEqType(actChan, eqNr)))));
    } else {
    	statusTxt.appendPlainText(QString("updateEQWidgets: Error on EQ widgets update: Audio instance not initialized. Try restarting."));
    }
}

void MainWindow::updateCutWidgets() {
    if (rtIO != nullptr) {
		hiPassCharWidget.setValue(rtIO->getCutCharacteristic(HIGHPASS, actChan));
		hiPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(rtIO->getCutCharacteristic(HIGHPASS, actChan)))));
		hiPassFreqWidget.setValue(rtIO->getCutFrequency(HIGHPASS, actChan));
		hiPassOrdWidget.setValue(rtIO->getCutOrder(HIGHPASS, actChan));
		loPassCharWidget.setValue(rtIO->getCutCharacteristic(LOWPASS, actChan));
		loPassCharWidget.setValueText(QString::fromStdString(std::string(CppXover::getCharName(rtIO->getCutCharacteristic(LOWPASS, actChan)))));
		loPassFreqWidget.setValue(rtIO->getCutFrequency(LOWPASS, actChan));
		loPassOrdWidget.setValue(rtIO->getCutOrder(LOWPASS, actChan));
	} else {
		statusTxt.appendPlainText(QString("updateCutWidgets: Error on limiter widgets update: Audio instance not initialized. Try restarting."));
	}
}

void MainWindow::updateLimiterWidgets() {
    if (rtIO != nullptr) {
		limitThresWidget.setValue(rtIO->getThreshold(actChan));
		limitMakeupWidget.setValue(rtIO->getMakeupGain(actChan));
		limitRelWidget.setValue(rtIO->getReleaseTime(actChan));
	} else {
		statusTxt.appendPlainText(QString("updateLimiterWidgets: Error on limiter widgets update: Audio instance not initialized. Try restarting."));
	}
}

int MainWindow::storeParams(const char* fileName) {
    uint32_t numEQsPerChan, tmpInt;
    double tmpDouble;
    QString tmpStr = QCoreApplication::applicationDirPath();
	tmpStr.append(QString("/") + QString(fileName));
	QByteArray tmpBA = tmpStr.toLatin1();
	const char *filePath = tmpBA.data();
    std::ofstream fStr(filePath, std::ios::binary | std::ios::trunc);

    if (fStr.good() && rtIO != nullptr) {
    	fStr.write((char*)&fs, sizeof(uint32_t));
    	fStr.write((char*)&outDevice.numChans, sizeof(uint32_t));
        for (uint32_t i=0; i<outDevice.numChans; i++) {
        	numEQsPerChan = rtIO->getNumEQs(i);
        	fStr.write((char*)&numEQsPerChan, sizeof(uint32_t));
            for (uint32_t j=0; j<numEQsPerChan; j++) {
                tmpDouble = rtIO->getEqGain(i,j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = rtIO->getEqFrequency(i,j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = rtIO->getEqQFactor(i,j);
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpInt = rtIO->getEqType(i,j);
                fStr.write((char*)&tmpInt, sizeof(uint32_t));
            }
            tmpInt = rtIO->getCutOrder(HIGHPASS, i);
            fStr.write((char*)&tmpInt, sizeof(uint32_t));
            tmpInt = (uint32_t) rtIO->getCutCharacteristic(HIGHPASS, i);
            fStr.write((char*)&tmpInt, sizeof(uint32_t));
            tmpDouble = rtIO->getCutFrequency(HIGHPASS, i);
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpInt = rtIO->getCutOrder(LOWPASS, i);
			fStr.write((char*)&tmpInt, sizeof(uint32_t));
            tmpInt = (uint32_t) rtIO->getCutCharacteristic(LOWPASS, i);
            fStr.write((char*)&tmpInt, sizeof(uint32_t));
            tmpDouble = rtIO->getCutFrequency(LOWPASS, i);
            fStr.write((char*)&tmpDouble, sizeof(double));

            tmpDouble = rtIO->getThreshold(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = rtIO->getMakeupGain(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = rtIO->getReleaseTime(i);
            fStr.write((char*)&tmpDouble, sizeof(double));
        }
        statusTxt.appendPlainText(QString("storeParams: Successfully stored current parameters in <") + tmpStr + QString(">.") + QString("\n"));
        return 0;
    } else {
        statusTxt.appendPlainText(QString("storeParams: Error: Could not open and write to <") + tmpStr + QString(">.") + QString("\n"));
    	return -1;
    }
}

int MainWindow::loadParams(const char* fileName) {
    uint32_t tmpInt, numOutChansFile, numEQsPerChan;
    double tmpDouble;
    QString tmpStr = QCoreApplication::applicationDirPath();
	tmpStr.append(QString("/") + QString(fileName));
	QByteArray tmpBA = tmpStr.toLatin1();
	const char *filePath = tmpBA.data();
    std::ifstream fStr(filePath, std::ios::binary);

    if (fStr.good() && rtIO != nullptr) {
        fStr.read((char*)&fs, sizeof(uint32_t));
        fStr.read((char*)&numOutChansFile, sizeof(uint32_t));
        for (uint32_t i=0; i<outDevice.numChans && i<numOutChansFile; i++) {
        	fStr.read((char*)&numEQsPerChan, sizeof(uint32_t));
        	rtIO->setNumEQs(i, numEQsPerChan);
            for (uint32_t j=0; j<numEQsPerChan; j++) {
                fStr.read((char*)&tmpDouble, sizeof(double));
                rtIO->setEqGain(i, j, tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                rtIO->setEqFrequency(i, j, tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                rtIO->setEqQFactor(i, j, tmpDouble);
                fStr.read((char*)&tmpInt, sizeof(uint32_t));
                rtIO->setEqType(i, j, (eqType)tmpInt);
            }
			fStr.read((char*)&tmpInt, sizeof(uint32_t));
			rtIO->setCutOrder(HIGHPASS, i, tmpInt);
            fStr.read((char*)&tmpInt, sizeof(uint32_t));
			rtIO->setCutCharacteristic(HIGHPASS, i, (filterChar) tmpInt);
			fStr.read((char*)&tmpDouble, sizeof(double));
			rtIO->setCutFrequency(HIGHPASS, i, tmpDouble);
			fStr.read((char*)&tmpInt, sizeof(uint32_t));
			rtIO->setCutOrder(LOWPASS, i, tmpInt);
			fStr.read((char*)&tmpInt, sizeof(uint32_t));
			rtIO->setCutCharacteristic(LOWPASS, i, (filterChar) tmpInt);
			fStr.read((char*)&tmpDouble, sizeof(double));
			rtIO->setCutFrequency(LOWPASS, i, tmpDouble);

            fStr.read((char*)&tmpDouble, sizeof(double));
            rtIO->setThreshold(i, tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            rtIO->setMakeupGain(i, tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            rtIO->setReleaseTime(i, tmpDouble);
        }
		this->updateEQWidgets();
		this->updateCutWidgets();
		this->updateLimiterWidgets();
		this->plotUpdate();
        statusTxt.appendPlainText(QString("loadParams: Successfully loaded parameters from <") + tmpStr + QString(">.") + QString("\n"));
		return 0;
	} else {
        statusTxt.appendPlainText(QString("loadParams: Error: Could not open and read from <") + tmpStr + QString(">.") + QString("\n"));
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
