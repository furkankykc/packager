#include "uproject.h"
#include "packageworker.h"
uProject::uProject(QString path, QObject *parent):
    QObject(parent)
{
    configure(path);
    RetrieveSettings();


}

void uProject::PrintPackageOptions()
{

    foreach (Options option, package_options) {
        qDebug() << "Level:" << level_paths[option.map % level_paths.size()]
                 << "Target:"
                 << build_targets[option.target % build_targets.size()]
                 << "Config:"
                 << package_configurations[option.config %
                                           package_configurations.size()];
    }


}

void uProject::package_job_finished(int v)
{

}



uProject::uProject(QObject *parent):QObject(parent)
{

}

QString uProject::getFlag(Options option)
{
    QString flags;
    if (build_targets[option.target] == "Server") {
        flags = QString(FLAGS_SERVER);
    } else if (build_targets[option.target] == "Client") {
        flags = QString(FLAGS_CLIENT);
    } else {
        flags = QString(FLAGS_DEFAULT);
    }

    return flags;
}

void uProject::Package(QList<Options> options,QString unreal_dir,QString output_path,int version)
{
    package_options = options;


    QString _exportPath = getExportName(output_path,false);



    Package(unreal_dir,_exportPath,version);
}

void uProject::Package(QString unreal_dir,QString export_path,int version)
{
    Options opt = package_options.takeFirst();
    changeLevel(opt);


    QString mapName =
        level_paths.at(opt.map).split("/").last().replace(".umap", "");

    QDir export_dir = QDir(export_path);

    QString specialExportName = QString("%1_%2_%3_%4")
                                    .arg(project_name, mapName,
                                         build_targets[opt.target],
                                         package_configurations[opt.config]);
    export_dir.mkdir(specialExportName);
    export_dir.cd(specialExportName);
    QString _export_path = export_dir.absolutePath();

    PackageWorker* package_Job = new PackageWorker(this);
    connect(package_Job,&PackageWorker::finished,this,&uProject::package_job_finished);
    connect(package_Job,&PackageWorker::readyReadSTDerr,this,&uProject::readyReadSTDerr);
    connect(package_Job,&PackageWorker::readyReadSTDout,this,&uProject::readyReadSTDout);
    connect(this,&uProject::cancel,package_Job,&PackageWorker::Cancel);


    QString flags = getFlag(opt);
    QString program =
        escapeSpaces(unreal_dir + QString(AUTOMATION_TOOL));
    QString automation_tool_parameters = QString(COMMAND).arg(
        uproject_path, package_configurations[opt.config], unreal_dir, flags,
        _export_path, QString::number(version));
    package_Job->setCommand(program,automation_tool_parameters);
    package_Job->process();

}

void uProject::configure(QString projectPath)
{
    bool bIsProjectDirValid = validateDirectory(projectPath);
    if(!bIsProjectDirValid){
        emit invalid_project_dir();
    }
    setConstants(projectPath);

}

bool uProject::validateDirectory(QString path)
{
    QDir project_dir = QDir(path);
    bool contains_source = project_dir.exists("Source");
    bool contains_content = project_dir.exists("Content");
    bool contains_uproject =
        !project_dir.entryList(QStringList() << "*.uproject").empty();
    bool result = contains_source && contains_content && contains_uproject;
    return result;
}

void uProject::setConstants(QString path)
{
    QDir _project_dir = QDir(path);
    QString _uproject_path = QDir(_project_dir)
                                 .entryInfoList(QStringList() << "*.uproject")
                                 .last()
                                 .absoluteFilePath();

    qDebug() << "Unreal project path (.uproject):" << _uproject_path;

    project_dir = _project_dir;
    uproject_path = _uproject_path;

    build_targets = getTargets();
    level_paths = getLevels();
    level_names = getLevelNames(level_paths);
    project_name = getProjectName();

}

QStringList uProject::getTargets() {
    QStringList filters = QStringList() << "*.Target.cs";
    QDir _project_dir = project_dir;
    _project_dir.cd("Source");
    QStringList targets = _project_dir.entryList(filters);
    QString moduleName;

    targets.replaceInStrings(".Target.cs", "");
    for (const QString &item : targets) {
        if (item.endsWith("Editor")) {
            moduleName = item;
            moduleName.replace("Editor", "");
            break;
        }
    }
    if (!moduleName.isEmpty()) {
        targets.replaceInStrings(moduleName, "");
    }
    targets.removeFirst();
    return targets;
}
QStringList uProject::getLevels() {
    QStringList nameFilters;
    QStringList levels;
    nameFilters << "*.umap"
                << "*.UMAP";
    QString _path;
    if (project_dir.cd("Content")) {
        _path = project_dir.absolutePath();
    }
    QDirIterator dirIterator(_path, nameFilters, QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        levels << dirIterator.next();
    }
    return levels;
}
QStringList uProject::getLevelNames(QStringList levels){
    QStringList level_names;
    for (const QString &level : qAsConst(level_paths)) {
        level_names << project_dir.relativeFilePath(level);
    }
    return level_names;
}

QString uProject::getProjectName() {
    QString projectName = "";
    QString _uproject_path = uproject_path;

    if (!_uproject_path.isEmpty()) {
        projectName = _uproject_path.replace(".uproject", "");
    }
    return projectName;
}
QString uProject::getConfigPath(){
    QDir configdir = QFileInfo(uproject_path).absoluteDir();
    configdir.cd("Config");
    QString _config_path = configdir.absoluteFilePath("DefaultEngine.ini");
    return _config_path;
}

QString uProject::changeLevel(Options opt){
    changeLevel(build_targets[opt.target],level_paths[opt.map]);
}

QString uProject::changeLevel(QString target, QString level)
{
    QString flags;
    QString _project_path = project_dir.absolutePath();
    if (target == "Server") {
        flags = QString(FLAGS_SERVER);

        writeIni(config_path, "ServerDefaultMap=",
                 convertConfigFormat(_project_path, level));
    } else if (target == "Client") {

        writeIni(config_path, "GameDefaultMap=",
                 convertConfigFormat(_project_path, level));
        flags = QString(FLAGS_CLIENT);
    } else {
        flags = QString(FLAGS_DEFAULT);

        writeIni(config_path, "GameDefaultMap=",
                 convertConfigFormat(_project_path, level));
    }
}
QString uProject::getExportName(QString output_path,bool current = false) {
    QDir outputFolder = QDir(output_path);
    if (outputFolder.isEmpty()) {
        outputFolder = QDir::currentPath();
    }
    QString exportName = project_name;
        int iter = 1;
        QString version_str = QString("_v%1");
        while (QDir(outputFolder)
                   .exists(QString("%1%2").arg(exportName, version_str.arg(iter)))) {
            iter += 1;
        }
    if(current && iter>1){
        iter--;
    }
    exportName.append(QString("_v%1").arg(QString::number(iter)));

    QDir(outputFolder).mkdir(exportName);
    QDir dir = QDir(outputFolder);
    dir.cd(exportName);
    return dir.absolutePath();
}
void uProject::SaveSettings(){
    QSettings settings("kykcbros", "packager");
    QString _project_dir_path = project_dir.absolutePath();
    QString projectKey = QString("packageOptions/%1").arg(_project_dir_path);
    settings.setValue("projectDir", _project_dir_path);
    QVariantList optionList;
    for (const Options &option : package_options) {
        QVariantMap optionMap;
        optionMap["map"] = option.map;
        optionMap["config"] = option.config;
        optionMap["target"] = option.target;
        optionList.append(optionMap);
    }
    settings.setValue(projectKey, optionList);
}
void uProject::RetrieveSettings(){
    QSettings settings("kykcbros", "packager");
    QString _project_dir_path = project_dir.absolutePath();
    QString projectKey = QString("packageOptions/%1").arg(_project_dir_path);

    QVariantList optionList = settings.value(projectKey).toList();

    QList<Options> options;
    for (const QVariant &variant : optionList) {
        QVariantMap optionMap = variant.toMap();
        Options option;
        option.map = optionMap["map"].toInt();
        option.config = optionMap["config"].toInt();
        option.target = optionMap["target"].toInt();
        options.append(option);
    }
    package_options = options;

    qDebug() << "Package option count:" << options.count();
}


