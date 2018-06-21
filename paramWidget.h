#ifndef PARAMWIDGET_H
#define PARAMWIDGET_H

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QEvent>
#include <cmath>

class QParamEdit : public QLineEdit
{
  Q_OBJECT

public:
  QParamEdit(QWidget *parent = nullptr);
  ~QParamEdit();

signals:
  void paramChanged(double);

private slots:
	void editingFinishedHandle();

protected:
  virtual void focusInEvent(QFocusEvent *e);
  virtual void focusOutEvent(QFocusEvent *e);
};

class paramWidget : public QWidget {
    Q_OBJECT

public:
    paramWidget(QWidget *parent = nullptr, double value=0.0, double stepSize=1.0, double lowerLim=-0xFFFF,
                double upperLim = 0xFFFF, QString label=QString("Label"), QString unit=QString("NV"),
				double precision=0.01, bool logFlag=false);
    ~paramWidget();

    inline double getValue() {
        return value;
    }

    void setValue(double value);

    inline void setPrecision(double precision) {
        this->precision = 1.0/precision;
    }

    inline int setStepSize(double stepSize) {
        if (stepSize>=0.0001 && stepSize<10000) {
            this->stepSize=stepSize;
            return 0;
        } else {
            return -1;
        }
    }

    inline void setLabel(QString str) {
        fieldLabel.setText(str);
    }

    inline void setUnit(QString str) {
        unit = str;
    }

    inline void setLimits(double lowerLim, double upperLim) {
        this->lowerLim = lowerLim;
        this->upperLim = upperLim;
    }

    inline void setReadOnly(bool flag) {
        valueField.setReadOnly(flag);
    }

    inline void setLogScaling(bool logFlag) {
        this->logFlag = logFlag;
    }

    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void valueChanged(double);

private slots:
    void increaseValue();

    void decreaseValue();

    void paramEdited(double);

private:
    void valueChangeHandle();

    QGridLayout layout;

    QPushButton plusButton, minusButton;

    QIcon plusIcon, minusIcon;

    QLabel fieldLabel;

    QParamEdit valueField;

    QString unit, actText;

    double value, stepSize, lowerLim, upperLim, precision;

    bool logFlag;
};

#endif // MAINWINDOW_H
