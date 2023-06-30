#ifndef UPROJECT_H
#define UPROJECT_H

#include <QString>
#include <QList>
#include <QObject>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QSettings>
#include "constants.h"

class uProject:public QObject
{
    Q_OBJECT
public:
    void Package(QList<Options> options,QString unreal_dir,QString export_dir,int version);
    explicit uProject(QObject *parent = 0);
    explicit uProject(QString path, QObject *parent = 0);
    QList<Options> package_options;
    QString Path() {return uproject_path;}
    QString Name() {return project_name;}
    QStringList Levels() {return level_names;}
    QStringList BuildConfigs() {return package_configurations;}
    QStringList BuildTargets(){return build_targets;}


    void PrintPackageOptions();
    void SaveSettings();
    void RetrieveSettings();
signals:
    void invalid_project_dir();
    void packaging_finished();
    void readyReadSTDout(QString out);
    void readyReadSTDerr(QString err);
    void cancel();

public slots:
    void package_job_finished(int v);

private:
    void Package(QString unreal_dir,QString export_dir,int version);
    void configure(QString path);
    bool validateDirectory(QString path);
    void setConstants(QString path);
    QStringList getTargets();
    QStringList getLevels();
    QStringList getLevelNames();
    QStringList getLevelNames(QStringList levels);
    QString getProjectName();
    QString getConfigPath();
    QString changeLevel(Options opt);
    QString changeLevel(QString target,QString level);

    QStringList build_targets;
    QStringList level_paths;
    QStringList level_names;
    QDir project_dir;
    QString uproject_path;
    QString project_name;
    QString config_path;
    QStringList package_configurations={"DebugGame", "Development", "Shipping"};

    QString getFlag(Options opt);
    QString getExportName(QString output_path, bool current);
};

#endif // UPROJECT_H
