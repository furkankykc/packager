#include "packageworker.h"
PackageWorker::PackageWorker(QObject *parent) :
    QObject(parent)
{
    workProcess = new QProcess(this);
    workProcess->setProcessChannelMode(QProcess::MergedChannels);
    connect(workProcess, &QProcess::started, this, &PackageWorker::processStarted);
    connect(workProcess, &QProcess::errorOccurred, this, &PackageWorker::processError);
    connect(workProcess, &QProcess::readyReadStandardError, this, &PackageWorker::slotReadStdErr);
    connect(workProcess, &QProcess::readyReadStandardOutput, this, &PackageWorker::slotReadStdOut);
    connect(workProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &PackageWorker::processFinished);
    connect(workProcess, &QProcess::destroyed, this, &PackageWorker::processDestroyed);
    connect(this, &PackageWorker::killProcess, workProcess, &QProcess::kill);
    connect(this, &PackageWorker::deleteProcess, workProcess, &QProcess::deleteLater);
}

void PackageWorker::setCommand(const QString &program,const QString &aut_params) {
#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)
    workProcess->setNativeArguments(
        QString("%1 %2").arg(program, aut_params));
#else
    workProcess->setArguments(
        QStringList()<<program<<aut_params);
#endif
}

void PackageWorker::process() {
    QString Command = "cmd.exe";
    QStringList args={"/C"};
    workProcess->start(Command, args, QIODevice::ReadOnly);

}

void PackageWorker::processStarted()
{
  std::cout << "[PackageWorker] Process has started!\n";
}

void PackageWorker::processError(QProcess::ProcessError error)
{
  std::cout << "[PackageWorker] Process ERROR!\n";

  switch (error) {
    case QProcess::FailedToStart: {
        std::cout << "[PackageWorker ERROR]: FailedToStart\n";
        break;
      }
    case QProcess::Crashed: {
        std::cout << "[PackageWorker ERROR]: Crashed\n";
        break;
      }
    case QProcess::Timedout: {
        std::cout << "[PackageWorker ERROR]: Timedout\n";
        break;
      }
    case QProcess::WriteError: {
        std::cout << "[PackageWorker ERROR]: WriteError\n";
        break;
      }
    case QProcess::ReadError: {
        std::cout << "[PackageWorker ERROR]: ReadError\n";
        break;
      }
    case QProcess::UnknownError: {
        std::cout << "[PackageWorker ERROR]: UnknownError\n";
        break;
      }
    default:
      break;
    }
}

void PackageWorker::slotReadStdErr()
{
  // call helper with pointer
  std::cout << "std err ready\n";
}

void PackageWorker::slotReadStdOut()
{
  // call helper with pointer
  std::cout << "std out ready\n";
}

void PackageWorker::processReadyReadSTDerr(QProcess &toRead)
{
  QString stdErr = toRead.readAllStandardError();
  std::cout << stdErr.toStdString() << std::endl;
  emit readyReadSTDerr(stdErr);
}

void PackageWorker::processReadyReadSTDout(QProcess &toRead)
{
  QString stdOut = toRead.readAllStandardOutput();
  std::cout << stdOut.toStdString() << std::endl;
  emit readyReadSTDout(stdOut);
}

void PackageWorker::processFinished(int exitCode)
{
  std::cout << "[PackageWorker] Process exited";
  std::cout << "[PackageWorker] Exit Code: " << exitCode << "\n";
  emit finished(exitCode);
}

void PackageWorker::processDestroyed()
{
  std::cout << "[QProcessWrapper] Process destroyed!";
}

void PackageWorker::processStateChanged(QProcess::ProcessState newState)
{
  std::cout << "[QProcessWrapper] Process stage changed!";

  switch (newState) {
    case QProcess::NotRunning: {
        std::cout << "[QProcessWrapper] Process is now NotRunning";
        break;
      }
    case QProcess::Starting: {
        std::cout << "[QProcessWrapper] Process is now Starting";
        break;
      }
    case QProcess::Running: {
        std::cout << "[QProcessWrapper] Process is now Running";
        break;
      }
    default:
      break;
    }
}

void PackageWorker::errorMessageBox(QString error)
{
    std::cout << "Error: " << error.toStdString() << std::endl;
}

void PackageWorker::Cancel(){
    if (workProcess && workProcess->state() == QProcess::Running) {
#ifdef Q_OS_WIN
        // Terminate process and its children on Windows
        QString command =
            QString("taskkill /F /T /PID %1")
                .arg(QString::number(process->processId()));
        QProcess::startDetached(command);
#else
        // Terminate process and its children on Unix-like systems (including macOS)
        qint64 pid = workProcess->pid();
        if (pid != -1) {
            QString command = QString("pkill -TERM -P %1").arg(pid);
            QProcess::startDetached(command);
        }
#endif
    }

}
