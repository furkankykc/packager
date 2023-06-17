#include "mainwindow.h"
#include "constants.h"
#include "qcheckbox.h"
#include "qdiriterator.h"
#include "qprocess.h"
#include "qsettings.h"
#include "ui_mainwindow.h"
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>

void MainWindow::Package() {
    if (progress == ui->progressBar->maximum() || progress == 0) {

        ui->progressBar->setRange(0, packageOptions.length());
        progress = 0;
        ui->progressBar->setValue(progress);
    }
    if (packageOptions.length() == 0) {
        ui->package_button->setEnabled(true);
        ui->cancel_button->setEnabled(false);
        return;
    }
    QDir configdir = QFileInfo(uproject_path).absoluteDir();
    QString project_path = configdir.absolutePath();
    configdir.cd("Config");
    QString config_path = configdir.absoluteFilePath("DefaultEngine.ini");
    if (packageOptions.empty())
        return;
    Options option = packageOptions.takeFirst();
    QString flags;
    if (build_targets[option.target] == "Server") {
        flags = QString(FLAGS_SERVER);

        writeIni(config_path, "ServerDefaultMap=",
                 convertConfigFormat(project_path, level_paths[option.map]));
    } else if (build_targets[option.target] == "Client") {

        writeIni(config_path, "GameDefaultMap=",
                 convertConfigFormat(project_path, level_paths[option.map]));
        flags = QString(FLAGS_CLIENT);
    } else {
        flags = QString(FLAGS_DEFAULT);

        writeIni(config_path, "GameDefaultMap=",
                 convertConfigFormat(project_path, level_paths[option.map]));
    }
    //    for (QLayout *layout : getChildLayouts(ui->packageLayout)) {
    //        changeChildWidgetsBackgroundColor(layout, QColor(Qt::yellow));
    //    };
    //        QProcess process;
    process = new QProcess;
    QString Command;  // Contains the command to be executed
    QStringList args; // Contains arguments of the command
    int consoleResult = 0;
    QObject::connect(
        process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "Process finished:" << progress << "/"
                     << ui->progressBar->value() << "/"
                     << ui->progressBar->maximum();
            progress += 1;
            ui->progressBar->setValue(progress);
            if (exitCode == 0) {
            }
            if (packageOptions.length() > 0) {
                MainWindow::Package();
            } else {
                ui->package_button->setEnabled(true);
                ui->cancel_button->setEnabled(false);
                emit package_finished();
            }
        });
    QObject::connect(process, &QProcess::readyReadStandardOutput,
                     QApplication::instance(), [&]() {
                         QByteArray output = process->readAllStandardOutput();
                         ui->textBrowser->append(QString::fromUtf8(output));
                         QTextStream(stdout) << output;
                     });
    QObject::connect(
        process, &QProcess::readyReadStandardError, QApplication::instance(),
        [&]() {
            QTextStream(stdout) << "ERROR:" << process->readAllStandardError();
            consoleResult = QMessageBox::critical(
                nullptr, "Packager", process->readAllStandardError());
        });
    QObject::connect(
        QApplication::instance(), &QCoreApplication::aboutToQuit, [&]() {
            if (process && process->state() == QProcess::Running) {
#ifdef Q_OS_WIN
                // Terminate process and its children on Windows
                QString command =
                    QString("taskkill /F /T /PID %1")
                        .arg(QString::number(process->processId()));
                QProcess::startDetached(command);
#else
                // Terminate process and its children on Unix-like systems (including macOS)
                qint64 pid = process->pid();
                if (pid != -1) {
                    QString command = QString("pkill -TERM -P %1").arg(pid);
                    QProcess::startDetached(command);
                }
#endif
            }
        });

    QString mapName =
        levels.at(option.map).split("/").last().replace(".umap", "");
    QString program =
        escapeSpaces(QString(unrealDir) + QString(AUTOMATION_TOOL));
    QString _exportPath;
    if (exportPath.isEmpty()) {
        _exportPath = getExportName(false);
    } else {
        _exportPath = exportPath;
    }
    QDir exportDir = QDir(_exportPath);
    QString specialExportName = QString("%1_%2_%3_%4")
                                    .arg(getProjectName(projectDir), mapName,
                                         build_targets[option.target],
                                         package_configurations[option.config]);
    exportDir.mkdir(specialExportName);
    exportDir.cd(specialExportName);
    QString automation_tool_parameters = QString(COMMAND).arg(
        uproject_path, package_configurations[option.config], unrealDir, flags,
        exportDir.absolutePath(), QString::number(version));
    process->setNativeArguments(
        QString("%1 %2").arg(program, automation_tool_parameters));

    Command = "cmd.exe";
    args << "/C";

    process->start(Command, args, QIODevice::ReadOnly);
    //        process->waitForFinished(-1);
    // Waits for execution to complete process.close();
    qDebug() << "Console result:" << consoleResult;
}

QString MainWindow::getLastCreatedFolder(const QString &directoryPath) {
    QDir directory(directoryPath);
    QStringList entries = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Time | QDir::Reversed);

    if (!entries.isEmpty()) {
        QString lastCreatedFolder = entries.first();
        return QDir::cleanPath(directory.absoluteFilePath(lastCreatedFolder));
    }

    return QString(); // Return an empty string if no folders found
}
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->packageLayout->setAlignment(Qt::AlignTop);
    //    ui->tabWidget->
    connect(ui->cancel_button, &QPushButton::clicked, this, [&]() {
        if (process && process->state() == QProcess::Running) {
#ifdef Q_OS_WIN
            // Terminate process and its children on Windows
            QString command = QString("taskkill /F /T /PID %1")
                                  .arg(QString::number(process->processId()));
            QProcess::startDetached(command);
#else
            // Terminate process and its children on Unix-like systems (including macOS)
            qint64 pid = process->pid();
            if (pid != -1) {
                QString command = QString("pkill -TERM -P %1").arg(pid);
                QProcess::startDetached(command);
            }
#endif
        }
    });
    connect(ui->browse_button, &QPushButton::clicked, this, [=]() {
        QString dir = QFileDialog::getExistingDirectory(
            0, ("Select Project Folder"), QDir::currentPath());
        saveSettings();
        InitializeProject(dir);
    });
    QObject::connect(QApplication::instance(), &QApplication::aboutToQuit, this,
                     [&] { saveSettings(); });

    connect(ui->actionSet_UnrealPath, &QAction::triggered, [&] {
        QString dir = QFileDialog::getExistingDirectory(
            0, ("Select Unreal Engine Directory"), QDir::currentPath());
        if (!dir.isEmpty()) {
            unrealDir = dir;
        } else {
            QMessageBox::warning(
                parentWidget(), "Invalid Unreal Engine Folder",
                "The selected folder is not a valid Unreal Engine folder.");
        };
    });
    connect(ui->actionSet_Deployer_Path, &QAction::triggered, [&] {
        QString dir = QFileDialog::getOpenFileName(0, ("Select Deployer"),
                                                   QDir::currentPath());

        if (!dir.isEmpty()) {
            deployerPath = dir;
        } else {
            QMessageBox::warning(parentWidget(), "Invalid Deployer",
                                 "The selected file is not a valid Deployer.");
        };
    });
    connect(ui->actionOpen_Output_Directory, &QAction::triggered, [&] {
        if (!outputFolder.isEmpty()) {
            QDesktopServices::openUrl(outputFolder);
        }
    });
    connect(ui->actionSet_Output_Folder, &QAction::triggered, [&] {
        QString dir = QFileDialog::getExistingDirectory(
            0, ("Select Unreal Engine Directory"), QDir::currentPath());
        if (!dir.isEmpty()) {
            outputFolder = dir;
        } else {
            QMessageBox::warning(
                parentWidget(), "Invalid Output  Folder",
                "The selected folder is not a valid Output folder.");
        };
    });
    connect(this, &MainWindow::package_finished, &MainWindow::StartDeploy);
    connect(ui->actionAuto_Zip_Exports, &QAction::toggled,
            [&](bool checked) { bAutoZip = checked; });
    connect(ui->actionDelete_exports_after_zip, &QAction::toggled,
            [&](bool checked) { bDeleteAfterZip = checked; });
    connect(ui->actionDeploy_Client, &QAction::toggled,
            [&](bool checked) { bAutoDeploy = checked; });
    package_configurations << "DebugGame"
                           << "Development"
                           << "Shipping";
    //    build_targets << "Strait"
    //                  << "Client"
    //                  << "Server";
    retrieveSettings();
    InitializeProject(projectDir);
    connect(ui->package_button, &QPushButton::clicked, this, [=] {
        QList<Options> options = getOptionsFromLayout(ui->packageLayout);
        packageOptions = options;
        printPackageOptions();
        ui->package_button->setEnabled(false);
        ui->cancel_button->setEnabled(true);

        // Package commands
        Package();
    });
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::StartDeploy() {
    if (bAutoDeploy && !deployerPath.isEmpty()) {
        int consoleResult = 0;
        QString clientPath;
        clientPath =
            QDir(QDir(exportPath).entryList(QStringList() << "Client").first())
                .absolutePath();
        process = new QProcess;
        QObject::connect(process, &QProcess::readyReadStandardOutput,
                         QApplication::instance(), [&]() {
                             QByteArray output =
                                 process->readAllStandardOutput();
                             ui->textBrowser->append(QString::fromUtf8(output));
                             QTextStream(stdout) << output;
                         });
        QObject::connect(process, &QProcess::readyReadStandardError,
                         QApplication::instance(), [&]() {
                             QTextStream(stdout)
                                 << "ERROR:" << process->readAllStandardError();
                             consoleResult = QMessageBox::critical(
                                 nullptr, "Deployer",
                                 process->readAllStandardError());
                         });
        QObject::connect(
            QApplication::instance(), &QCoreApplication::aboutToQuit, [&]() {
                if (process && process->state() == QProcess::Running) {
#ifdef Q_OS_WIN
                    // Terminate process and its children on Windows
                    QString command =
                        QString("taskkill /F /T /PID %1")
                            .arg(QString::number(process->processId()));
                    QProcess::startDetached(command);
#else
                    // Terminate process and its children on Unix-like systems (including macOS)
                    qint64 pid = process->pid();
                    if (pid != -1) {
                        QString command = QString("pkill -TERM -P %1").arg(pid);
                        QProcess::startDetached(command);
                    }
#endif
                }
            });

        process->setNativeArguments(
            QString("%1 %2").arg(escapeSpaces(deployerPath), clientPath));

        process->start("cmd.exe", QStringList{"/C"}, QIODevice::ReadOnly);
    }
};

void MainWindow::deleteChildLayouts(QLayout *layout) {
    QList<QLayout *> childLayouts = getChildLayouts(layout);
    if (childLayouts.empty())
        return;
    for (QLayout *child : childLayouts) {
        deleteLayout(child, ui->packageLayout);
    }
}

void MainWindow::InitializeProject(QString path) {

    if (!path.isEmpty() && validateUEProject(path)) {
        setWindowTitle(getProjectName(path) + " | Packager");
        projectDir = path;
        retrieveSettings();
        projectDir = path;
        setUnrealProjectPath(path);
        ui->project_label->setText(path);
        deleteChildLayouts(ui->packageLayout);
        getProjectTargets(path);
        level_paths = ListLevels(path);
        levels.clear();
        for (const QString &level : qAsConst(level_paths)) {
            levels << QDir(path).relativeFilePath(level);
        }
        Options options;
        addPackageJob(true, options);
        for (Options option : packageOptions) {
            addPackageJob(false, option);
        }
        printPackageOptions();
    } else {
        QMessageBox::warning(
            parentWidget(), "Invalid Unreal Project Folder",
            "The selected folder is not a valid Unreal project folder.");
    }
}

QList<QLayout *> MainWindow::getChildLayouts(QLayout *vLayout) {
    QList<QLayout *> childLayouts;
    for (int i = 0; i < vLayout->count(); ++i) {
        QLayoutItem *layoutItem = ui->packageLayout->itemAt(i);
        if (layoutItem) {
            QLayout *childLayout =
                qobject_cast<QLayout *>(layoutItem->layout());
            if (childLayout) {
                childLayouts.append(childLayout);
            }
        }
    }

    return childLayouts;
}
void MainWindow::changeChildWidgetsBackgroundColor(QLayout *layout,
                                                   const QColor &color) {
    if (!layout)
        return;

    // Iterate through all items in the layout
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem *item = layout->itemAt(i);
        if (item->widget()) {
            // Change the background color of the widget
            item->widget()->setStyleSheet(
                QString("background-color: %1;").arg(color.name()));
        } else if (item->layout()) {
            // Recursively call the function for child layouts
            changeChildWidgetsBackgroundColor(item->layout(), color);
        }
    }
}
int MainWindow::getConfigFromComboBox(QLayoutItem *layoutItem) {
    int value = 0;
    QComboBox *comboBox = qobject_cast<QComboBox *>(layoutItem->widget());
    if (comboBox) {
        qDebug() << "Combobox value:" << comboBox->currentIndex();
        value = comboBox->currentIndex();
    } else {
        qDebug() << "Not a combobox";
    }
    return value;
}

QList<Options> MainWindow::getOptionsFromLayout(QLayout *vLayout) {
    QList<QLayout *> childLayouts = getChildLayouts(vLayout);
    QList<Options> options;
    // Use the child layouts as needed
    for (QLayout *childLayout : childLayouts) {
        // Do something with the child layout
        // ...
        Options option;
        bool enabled = false;
        QLayoutItem *layoutItem = childLayout->itemAt(0);
        QCheckBox *chckBox = qobject_cast<QCheckBox *>(layoutItem->widget());
        if (chckBox) {
            qDebug() << "Checkbox:" << chckBox->checkState();
            enabled = chckBox->checkState();
        } else {
            qDebug() << "Not a checkbox";
        }
        /////
        //            layoutItem = childLayout->itemAt(1);
        option.map = getConfigFromComboBox(childLayout->itemAt(1));
        option.config = getConfigFromComboBox(childLayout->itemAt(2));
        option.target = getConfigFromComboBox(childLayout->itemAt(3));
        ////

        if (enabled)
            options.append(option);
    }
    return options;
}

QStringList MainWindow::ListLevels(QString path) {
    QStringList nameFilters;
    QString _path = path;
    QStringList levels;
    nameFilters << "*.umap"
                << "*.UMAP";
    qDebug() << "ListLevels:" << path;
    QDir path_dir = QDir(path);
    if (path_dir.cd("Content")) {
        _path = path_dir.absolutePath();
    }
    QDirIterator dirIterator(_path, nameFilters, QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        levels << dirIterator.next();
    }
    foreach (QString filename, levels) {
        qDebug() << filename;
    }

    return levels;
}

void MainWindow::deleteLayout(QLayout *vLayout, QLayout *parent) {
    if (vLayout) {
        QLayoutItem *child;
        while ((child = vLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
    }
    parent->removeItem(vLayout);
    delete vLayout;
}

Options MainWindow::getCurrentOptions(QComboBox *target,
                                      QComboBox *configuration,
                                      QComboBox *comboBox) {
    qDebug() << "map:" << comboBox->currentIndex()
             << "config:" << configuration->currentIndex()
             << "target:" << target->currentIndex();
    Options _options{comboBox->currentIndex(), configuration->currentIndex(),
                     target->currentIndex()};

    return _options;
}

QComboBox *MainWindow::createOptionComboBox(QStringList options,
                                            int optionIndex, QLayout *vLayout) {
    QComboBox *comboBox = new QComboBox;
    comboBox->addItems(options);
    comboBox->setCurrentIndex(optionIndex % options.length());
    vLayout->addWidget(comboBox);
    return comboBox;
}

void MainWindow::addPackageJob(bool init, Options options) {

    QHBoxLayout *vLayout = new QHBoxLayout;

    if (!init) {
        QCheckBox *check = new QCheckBox;
        vLayout->addWidget(check);
        check->setCheckState(Qt::Checked);
    }

    QComboBox *levelBox = createOptionComboBox(levels, options.map, vLayout);
    QComboBox *configBox =
        createOptionComboBox(package_configurations, options.config, vLayout);
    QComboBox *targetBox =
        createOptionComboBox(build_targets, options.target, vLayout);

    if (init) {
        QPushButton *applyButton = new QPushButton;
        applyButton->setText("Add");
        connect(applyButton, &QPushButton::clicked, this, [=] {
            Options _options =
                getCurrentOptions(targetBox, configBox, levelBox);
            addPackageJob(false, _options);
        });
        vLayout->addWidget(applyButton);
        deleteLayout(mainSelectLayout, ui->mainLayout);
        ui->mainLayout->addLayout(vLayout);
        mainSelectLayout = vLayout;

    } else {
        QPushButton *deleteButton = new QPushButton;
        deleteButton->setText("Delete");

        connect(deleteButton, &QPushButton::clicked,
                [=] { deleteLayout(vLayout, ui->packageLayout); });

        vLayout->addWidget(deleteButton);
        ui->packageLayout->addLayout(vLayout);
    }
}

void MainWindow::printPackageOptions() {
    foreach (Options option, packageOptions) {
        qDebug() << "Level:" << level_paths[option.map % level_paths.size()]
                 << "Target:"
                 << build_targets[option.target % build_targets.size()]
                 << "Config:"
                 << package_configurations[option.config %
                                           package_configurations.size()];
    }
}

bool MainWindow::validateUEProject(QString path) {
    QDir project_dir = QDir(path);

    bool contains_source = project_dir.exists("Source");
    bool contains_content = project_dir.exists("Content");
    bool contains_uproject =
        !project_dir.entryList(QStringList() << "*.uproject").empty();
    bool result = contains_source && contains_content && contains_uproject;
    return result;
}

void MainWindow::getProjectTargets(QString dir) {
    QDir directory = QDir(dir);
    QStringList filters = QStringList() << "*.Target.cs";
    directory.cd("Source");
    QStringList targets = directory.entryList(filters);
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
    qDebug() << moduleName << "\t" << targets;
    build_targets = targets;
}

void MainWindow::setUnrealProjectPath(QString dir) {
    QString _uproject_path = QDir(dir)
                                 .entryInfoList(QStringList() << "*.uproject")
                                 .last()
                                 .absoluteFilePath();
    qDebug() << "Unreal project path (.uproject):" << _uproject_path;
    uproject_path = _uproject_path;
}

QString MainWindow::getProjectName(QString dir) {
    QString projectName = "";
    QString uprojectPath =
        QDir(dir).entryList(QStringList() << "*.uproject").constLast();
    if (!uprojectPath.isEmpty()) {
        projectName = uprojectPath.replace(".uproject", "");
    }
    return projectName;
}

void MainWindow::saveSettings() {
    // Create a QSettings object
    QSettings settings("kykcbros", "packager");

    // Save the checkbox state in QSettings
    settings.setValue("projectDir", projectDir);
    settings.setValue("outputFolder", outputFolder);
    qDebug() << "ProjectDir" << projectDir;
    settings.setValue("unrealDir", unrealDir);
    settings.setValue("deployerPath", deployerPath);
    settings.setValue("autoZip", bAutoZip);
    settings.setValue("deleteAfterZip", bDeleteAfterZip);
    settings.setValue("autoDeploy", bAutoDeploy);

    QString projectKey = QString("packageOptions/%1").arg(projectDir);
    QList<Options> options = getOptionsFromLayout(ui->packageLayout);
    qDebug() << "Package option count:" << options.count();

    QVariantList optionList;
    for (const Options &option : options) {
        QVariantMap optionMap;
        optionMap["map"] = option.map;
        optionMap["config"] = option.config;
        optionMap["target"] = option.target;
        optionList.append(optionMap);
    }

    settings.setValue(projectKey, optionList);
}

void MainWindow::retrieveSettings() {
    QSettings settings("kykcbros", "packager");

    unrealDir = settings.value("unrealDir", "").toString();
    outputFolder = settings.value("outputFolder", "").toString();
    deployerPath = settings.value("deployerPath", "").toString();
    bAutoZip = settings.value("autoZip", false).toBool();
    bDeleteAfterZip = settings.value("deleteAfterZip", false).toBool();
    bAutoDeploy = settings.value("autoDeploy", false).toBool();

    QString projectKey = QString("packageOptions/%1").arg(projectDir);

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
    packageOptions = options;
    projectDir = settings.value("projectDir", "").toString();

    qDebug() << "Package option count:" << options.count();
}

QString MainWindow::getExportName(bool current = false) {
    if (outputFolder.isEmpty()) {
        outputFolder = QDir::currentPath();
    }
    QString exportName = getProjectName(projectDir);

    if (!current) {
        int iter = 1;
        QString version = QString("_v%1").arg(QString::number(iter));
        while (QDir(outputFolder)
                   .exists(QString("%1%2").arg(exportName, version))) {
            iter += 1;
            version = QString("_v%1").arg(QString::number(iter));
        }

        this->version = iter;
    }
    exportName.append(QString("_v%1").arg(QString::number(version)));
    QDir(outputFolder).mkdir(exportName);
    QDir dir = QDir(outputFolder);
    dir.cd(exportName);
    exportPath = dir.absolutePath();
    return dir.absolutePath();
}
