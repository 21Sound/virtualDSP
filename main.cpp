/*------------------------------------------------------------------*\
Implmentation of a Peak Equalizer Class
the design is based on the EQ cookbook by Robert bristow Johnson
public domain

Author: Joerg Bitzer (TGM) (Jade-Hochschule) 

Modified by Hagen Jaeger, 22.09.2014, now with all cookbook-variations of a BiQuad
(LoPass, HiPass, LoShelv, HiShelf, pEQ, ...). Modification is based on Joerg Bitzers 
'filterdesign.m', i.e. RBJ Cookbook, for MATLAB.

Version 1.0.1 (debugged and tested, 22.09.2014).
\*------------------------------------------------------------------*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    int width = QApplication::desktop()->screenGeometry().width();
    int height = QApplication::desktop()->screenGeometry().height();

    MainWindow mainWin(0.5*width, 0.6*height);
    mainWin.move(width*0.25,height*0.2);

    mainWin.show();

    return app.exec();
}
