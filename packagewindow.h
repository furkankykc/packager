#ifndef PACKAGEWINDOW_H
#define PACKAGEWINDOW_H


#include "qcombobox.h"
#include "qlayoutitem.h"
#include <QMainWindow>
#include <QProcess>
#include "uproject.h"
#include "constants.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class PackageWindow;
class MainWindow;

}
QT_END_NAMESPACE
class PackageWindow: public QMainWindow
{
    Q_OBJECT
public:
    PackageWindow(QWidget *parent = nullptr);
    ~PackageWindow();

    void InitializeProject(QString path);

    QList<Options> getOptionsFromLayout(QLayout *vLayout);

    void Package(QList<Options> options);

private:
    QStringList ListLevels(QString path);
    void addPackageJob(bool init, Options options);
    void saveSettings();
    void retrieveSettings();
    QString getExportName(bool current);
    void StartDeploy();
signals:
    void package_finished();

private:
    QLayout *mainSelectLayout = nullptr;
    Ui::MainWindow *ui;

    void deleteLayout(QLayout *vLayout, QLayout *parent);
    Options getCurrentOptions(QComboBox *target, QComboBox *configuration,
                              QComboBox *comboBox);
    QComboBox *createOptionComboBox(QStringList options, int optionIndex,
                                    QLayout *vLayout);
    QList<QLayout *> getChildLayouts(QLayout *vLayout);
    int getConfigFromComboBox(QLayoutItem *layoutItem);
    void deleteChildLayouts(QLayout *layout);
    void changeChildWidgetsBackgroundColor(QLayout *layout,
                                           const QColor &color);
    QString getLastCreatedFolder(const QString &directoryPath);

    QProcess *process;
    int progress = 0;
    int version = 1;
    QString unrealDir;
    QString deployerPath;
    QString outputFolder;
    QString exportPath;
    bool bAutoZip;
    bool bDeleteAfterZip;
    bool bAutoDeploy;

    uProject* project;

};

#endif // PACKAGEWINDOW_H
