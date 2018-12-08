#include "paramWidget.h"
#include <QCoreApplication>
#include <QWheelEvent>
#include <cmath>

QParamEdit::QParamEdit(QWidget *parent)
 : QLineEdit(parent){
	connect(this, SIGNAL (editingFinished()), this, SLOT(editingFinishedHandle()));
}

QParamEdit::~QParamEdit(){

}

void QParamEdit::editingFinishedHandle() {
	this->clearFocus();
}

void QParamEdit::focusInEvent(QFocusEvent *e){
  QLineEdit::focusInEvent(e);
  this->oldText = this->text();
  this->setText(QString());
}

void QParamEdit::focusOutEvent(QFocusEvent *e)
{
  bool successFlag;
  QString actText = this->text();
  double value = actText.toDouble(&successFlag);

  if (successFlag) {
      emit paramChanged(value);
  } else {
      this->setText(this->oldText);
  }
  QLineEdit::focusOutEvent(e);
}

paramWidget::paramWidget(QWidget *parent, double value, double stepSize, double lowerLim,
                         double upperLim, QString label, QString unit, double precision, bool logFlag)
    : QWidget(parent), value(value), stepSize(stepSize), lowerLim(lowerLim), upperLim(upperLim),
      unit(unit), actText(value+QString(" ")+unit), precision(precision), logFlag(logFlag) {

    QPalette tmpPal;

    QString tmpStr = QCoreApplication::applicationDirPath();
    tmpStr.append("/symbols/plus.png");
    plusIcon.addFile(tmpStr);

    tmpStr = QCoreApplication::applicationDirPath();
    tmpStr.append("/symbols/minus.png");
    minusIcon.addFile(tmpStr);

    fieldLabel.setParent(this);
    fieldLabel.setText("Label");
    fieldLabel.setAlignment(Qt::AlignCenter);

    minusButton.setParent(this);
    minusButton.setIcon(minusIcon);
    minusButton.setAutoRepeat(true);
    minusButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    plusButton.setParent(this);
    plusButton.setIcon(plusIcon);
    plusButton.setAutoRepeat(true);
    plusButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    valueField.setParent(this);
    valueField.setText(QString("0"));
    valueField.setPalette(this->palette());
    valueField.setAutoFillBackground(true);
    valueField.setReadOnly(false);
    valueField.setAlignment(Qt::AlignCenter);
    valueField.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    layout.addWidget(&fieldLabel, 0, 0, 1, 4);
    layout.addWidget(&minusButton, 1, 0, 2, 2);
    layout.addWidget(&plusButton, 1, 2, 2, 2);
    layout.addWidget(&valueField, 3, 0, 2, 4);
    this->setLayout(&layout);

    tmpPal = valueField.palette();
    tmpPal.setColor(QPalette::Background, QColor(0,0,0,0));
    valueField.setPalette(tmpPal);

    connect(&plusButton, SIGNAL (clicked()), this, SLOT(increaseValue()));
    connect(&minusButton, SIGNAL (clicked()), this, SLOT(decreaseValue()));
    connect(&valueField, SIGNAL (paramChanged(double)), this, SLOT(paramEdited(double)));
    this->installEventFilter(this);

    this->setValue(value);
}

void paramWidget::setValue(double value) {
    if (value<lowerLim) {
        this->value = lowerLim;
    } else if (value>upperLim) {
        this->value = upperLim;
    } else {
        this->value = (double)qRound(value/precision)*precision;
    }
    actText = QString::number(this->value) + QString(" ") + unit;
    valueField.setText(actText);
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

bool paramWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Wheel)
  {
      QWheelEvent *wE = (QWheelEvent *)event;
      if (wE->delta() > 0) {
      	this->increaseValue();
      } else {
    	this->decreaseValue();
      }
  }

  return false;
}

void paramWidget::paramEdited(double value) {
	this->setValue(value);
	emit valueChanged(this->value);
}

paramWidget::~paramWidget(){

}
