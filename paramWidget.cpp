#include "paramWidget.h"
#include <QCoreApplication>
#include <QFileInfo>

paramWidget::paramWidget(QWidget *parent, double value, double stepSize, double lowerLim,
                         double upperLim, QString label, QString unit)
    : QWidget(parent), value(value), stepSize(stepSize), lowerLim(lowerLim), upperLim(upperLim),
      unit(unit), actText(value+QString(" ")+unit) {

    QPalette tmpPal;

    QFileInfo currentPath(QCoreApplication::applicationFilePath());

    QString tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/plus.png");
    plusIcon.addFile(tmpStr);

    tmpStr = currentPath.absolutePath();
    tmpStr.append("/symbols/minus.png");
    minusIcon.addFile(tmpStr);

    fieldLabel.setParent(this);
    fieldLabel.move(0,0);
    fieldLabel.resize(60,20);
    fieldLabel.setText("Label");
    fieldLabel.setAlignment(Qt::AlignCenter);

    minusButton.setParent(this);
    minusButton.setIcon(minusIcon);
    minusButton.move(QPoint(0,20));
    minusButton.resize(QSize(30,30));
    minusButton.setIconSize(QSize(30,30));
    minusButton.setAutoRepeat(true);

    plusButton.setParent(this);
    plusButton.setIcon(plusIcon);
    plusButton.move(QPoint(30,20));
    plusButton.resize(QSize(30,30));
    plusButton.setIconSize(QSize(30,30));
    plusButton.setAutoRepeat(true);

    valueField.setParent(this);
    valueField.setText(QString("0"));
    valueField.move(QPoint(0,50));
    valueField.resize(QSize(60,20));
    valueField.setPalette(this->palette());
    valueField.setAutoFillBackground(true);
    valueField.setReadOnly(false);
    valueField.setAlignment(Qt::AlignCenter);

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
    value+=stepSize;
    this->setValue(value);
    emit valueChanged(this->value);
}

void paramWidget::decreaseValue() {
    double value = this->value;
    value-=stepSize;
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
