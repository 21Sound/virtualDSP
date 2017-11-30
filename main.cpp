/*------------------------------------------------------------------*\
Implmentation of a Peak Equalizer Class
the design is based on the EQ cookbook by Robert bristow Johnson
public domain

Author: Joerg Bitzer (TGM) (Jade-Hochschule) 

Modified by Hagen Jaeger, 22.09.2014, now with all cookbook-variations of a BiQuad
(LoPass, HiPass, LoShelv, HiShelf, pEQ, ...). Modification is based on Joerg Bitzers 
'filterdesign.m', i.e. RBJ Cookbook, for MATLAB.

Version 1.0.1 (debuuged and tested, 22.09.2014).
\*------------------------------------------------------------------*/

#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWin;
    mainWin.setMaximumSize(600,400);
    mainWin.resize(800,400);

    mainWin.show();

    return app.exec();
}
