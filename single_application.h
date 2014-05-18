//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: single_application.h                  //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 18.05.2014                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef SINGLE_APPLICATION_H
#define SINGLE_APPLICATION_H

#include <QApplication>
#include <QSharedMemory>
#include <QStringList>

class SingleApplication : public QApplication
{
        Q_OBJECT
public:
        SingleApplication(int &argc, char *argv[], const QString uniqueKey);

        bool alreadyExists() const{
            return bAlreadyExists;
        }
        bool isMasterApp() const{
            return !alreadyExists();
        }

        bool sendMessage(const QString &message);

public slots:
        void checkForMessage();

signals:
        void messageAvailable(const QStringList& messages);

private:
        bool bAlreadyExists;
        QSharedMemory sharedMemory;
};
#endif // SINGLE_APPLICATION_H
