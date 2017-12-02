#include "paramWidget.h"
#include <QCoreApplication>
#include <QFileInfo>

paramWidget::paramWidget(QWidget *parent, double value, double stepSize, double lowerLim,
                         double upperLim, QString label, QString unit, double precision, bool logFlag)
    : QWidget(parent), value(value), stepSize(stepSize), lowerLim(lowerLim), upperLim(upperLim),
      unit(unit), actText(value+QString(" ")+unit), precision(1.0/precision), logFlag(logFlag) {

    QPalette tmpPal;

    QFileInfo currentPath(QCoreApplication::applicationFilePath());

    QString tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/plus.png");
    plusIcon.addFile(tmpStr);

    tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/minus.png");
    minusIcon.addFile(tmpStr);

    fieldLabel.setParent(this);
    fieldLabel.setText("Label");
    fieldLabel.setAlignment(Qt::AlignCenter);

    minusButton.setParent(this);
    minusButton.setIcon(minusIcon);
    minusButton.setAutoRepeat(true);

    plusButton.setParent(this);
    plusButton.setIcon(plusIcon);
    plusButton.setAutoRepeat(true);

    valueField.setParent(this);
    valueField.setText(QString("0"));
    valueField.setPalette(this->palette());
    valueField.setAutoFillBackground(true);
    valueField.setReadOnly(false);
    valueField.setAlignment(Qt::AlignCenter);

    layout.addWidget(&fieldLabel, 0, 0, 1, 2);
    layout.addWidget(&minusButton, 1, 0, 1, 1);
    layout.addWidget(&plusButton, 1, 1, 1, 1);
    layout.addWidget(&valueField, 2, 0, 1, 2);
    this->setLayout(&layout);

    tmpPal = valueField.palette();
    tmpPal.setColor(QPalette::Background, QColor(0,0,0,0));
    valueField.setPalette(tmpPal);

    connect(&plusButton, SIGNAL (clicked()), this, SLOT(increaseValue()));
    connect(&minusButton, SIGNAL (clicked()), this, SLOT(decreaseValue()));
    connect(&valueField, SIGNAL (returnPressed()), this, SLOT(lineEditHandle()));

    this->setValue(value);
}

void paramWidget::increaseValue() {
    double value = this->value;
    if(logFlag) {
    	value*=stepSize;
    } else {
    	value+=stepSize;
    }
    this->setValue(value);
    emit valueChanged(this->value);
}

void paramWidget::decreaseValue() {
    double value = this->value;
    if(logFlag) {
    	value/=stepSize;
    } else {
    	value-=stepSize;
    }
    this->setValue(value);
    emit valueChanged(this->value);
}

void paramWidget::lineEditHandle() {
    bool successFlag;
    double value;
    value = valueField.text().toDouble(&successFlag);
    if (successFlag) {
        this->setValue(value);
        emit valueChanged(this->value);
    } else {
        valueField.setText(actText);
    }
}

paramWidget::~paramWidget(){

}
