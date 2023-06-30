#ifndef PACKAGEWORKER_H
#define PACKAGEWORKER_H
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <iostream>
class PackageWorker:public QObject
{
    Q_OBJECT
public:
    void setCommand(const QString &program, const QString &aut_params);
    explicit PackageWorker(QObject *parent);
signals:
  void killProcess();
  void deleteProcess();
  void finished(int v);
  void readyReadSTDout(QString out);
  void readyReadSTDerr(QString err);


public slots:
  void process();
  void processStarted();
  void processError(QProcess::ProcessError error);
  void slotReadStdErr();
  void slotReadStdOut();
  void processFinished(int exitCode);
  void processDestroyed();
  void processStateChanged(QProcess::ProcessState newState);
  void errorMessageBox(QString error);
  void Cancel();

private:
  void processReadyReadSTDerr(QProcess &toRead);  // helps slotReadStdErr()
  void processReadyReadSTDout(QProcess &toRead); // helps slotReadStdOut()


  QProcess *workProcess;
};

#endif // PACKAGEWORKER_H
