#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qcombobox.h"
#include "qlayoutitem.h"
#include <QMainWindow>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct Options {
    int map = 0;
    int config = 0;
    int target = 0;
};
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void InitializeProject(QString path);

    QList<Options> getOptionsFromLayout(QLayout *vLayout);

    void Package();

private:
    QStringList ListLevels(QString path);
    void addPackageJob(bool init, Options options);
    QString getProjectName(QString dir);
    void saveSettings();
    void retrieveSettings();
    QString getExportName(bool current);
    void StartDeploy();
signals:
    void package_finished();

private:
    QStringList levels;
    QStringList level_paths;
    QStringList package_configurations;
    QStringList build_targets;
    QList<Options> packageOptions;
    QString uproject_path;
    QLayout *mainSelectLayout = nullptr;
    Ui::MainWindow *ui;
    bool validateUEProject(QString path);
    void getProjectTargets(QString dir);
    void printPackageOptions();
    void deleteLayout(QLayout *vLayout, QLayout *parent);
    Options getCurrentOptions(QComboBox *target, QComboBox *configuration,
                              QComboBox *comboBox);
    QComboBox *createOptionComboBox(QStringList options, int optionIndex,
                                    QLayout *vLayout);
    QList<QLayout *> getChildLayouts(QLayout *vLayout);
    int getConfigFromComboBox(QLayoutItem *layoutItem);
    void setUnrealProjectPath(QString dir);
    void deleteChildLayouts(QLayout *layout);
    void changeChildWidgetsBackgroundColor(QLayout *layout,
                                           const QColor &color);
    QString getLastCreatedFolder(const QString &directoryPath);

    QProcess *process;
    int progress = 0;
    int version = 1;
    QString projectDir;
    QString unrealDir;
    QString deployerPath;
    QString outputFolder;
    QString exportPath;
    bool bAutoZip;
    bool bDeleteAfterZip;
    bool bAutoDeploy;
};

#endif // MAINWINDOW_H
