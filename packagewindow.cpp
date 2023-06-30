#include "packagewindow.h"
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

void PackageWindow::Package(QList<Options> _package_options) {


    if (_package_options.empty())
        return;
    if (progress == ui->progressBar->maximum() || progress == 0) {

        ui->progressBar->setRange(0, _package_options.length());
        progress = 0;
        ui->progressBar->setValue(progress);
    }
    if (_package_options.length() == 0) {
        ui->package_button->setEnabled(true);
        ui->cancel_button->setEnabled(false);
        return;
    }



    QObject::connect(
        project,&uProject::package_job_finished,
        [=](int exitCode) {
            qDebug() << "Process finished:" << progress << "/"
                     << ui->progressBar->value() << "/"
                     << ui->progressBar->maximum();
            progress += 1;
            ui->progressBar->setValue(progress);
            if (exitCode == 0) {
            }
            if (project->package_options.length() == 0) {
                ui->package_button->setEnabled(true);
                ui->cancel_button->setEnabled(false);
                emit package_finished();
            }
        });
    QObject::connect(project, &uProject::readyReadSTDerr,
                      [&](QString out) {
                         ui->textBrowser->append(out);
                         QTextStream(stdout) << out;
                     });
    QObject::connect(
        project, &uProject::readyReadSTDout,
        [&](QString err) {
            QMessageBox::critical(
                nullptr, "Packager", err);
            QTextStream(stdout) << err;

        });
    QObject::connect(QApplication::instance(), &QCoreApplication::aboutToQuit, project,&uProject::cancel);
    project->Package(_package_options,QString(unrealDir),outputFolder,version);

}

QString PackageWindow::getLastCreatedFolder(const QString &directoryPath) {
    QDir directory(directoryPath);
    QStringList entries = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Time | QDir::Reversed);

    if (!entries.isEmpty()) {
        QString lastCreatedFolder = entries.first();
        return QDir::cleanPath(directory.absoluteFilePath(lastCreatedFolder));
    }

    return QString(); // Return an empty string if no folders found
}
PackageWindow::PackageWindow(QWidget *parent)
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
    connect(this, &PackageWindow::package_finished, &PackageWindow::StartDeploy);
    connect(ui->actionAuto_Zip_Exports, &QAction::toggled,
            [&](bool checked) { bAutoZip = checked; });
    connect(ui->actionDelete_exports_after_zip, &QAction::toggled,
            [&](bool checked) { bDeleteAfterZip = checked; });
    connect(ui->actionDeploy_Client, &QAction::toggled,
            [&](bool checked) { bAutoDeploy = checked; });
    connect(ui->package_button, &QPushButton::clicked, this, [=] {
        QList<Options> options = getOptionsFromLayout(ui->packageLayout);
        Package(options);
        ui->package_button->setEnabled(false);
        ui->cancel_button->setEnabled(true);
    });

    retrieveSettings();

}

PackageWindow::~PackageWindow() { delete ui; }

void PackageWindow::StartDeploy() {
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
#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)

        process->setNativeArguments(
            QString("%1 %2").arg(escapeSpaces(deployerPath), clientPath));
#else
        process->setArguments(
            QStringList()<<escapeSpaces(deployerPath)<< clientPath);
#endif
        process->start("cmd.exe", QStringList{"/C"}, QIODevice::ReadOnly);
    }
};

void PackageWindow::deleteChildLayouts(QLayout *layout) {
    QList<QLayout *> childLayouts = getChildLayouts(layout);
    if (childLayouts.empty())
        return;
    for (QLayout *child : childLayouts) {
        deleteLayout(child, ui->packageLayout);
    }
}

void PackageWindow::InitializeProject(QString path) {

    if (!path.isEmpty()) {
        project = new uProject(path);
        setWindowTitle(project->Name()+ " | Packager");

        QString _project_path = project->Path();

        QList<Options> _package_options = project->package_options;

        ui->project_label->setText(_project_path);

        deleteChildLayouts(ui->packageLayout);

        Options options;
        addPackageJob(true, options);
        for (Options option : _package_options) {
            addPackageJob(false, option);
        }
    } else {
        QMessageBox::warning(
            parentWidget(), "Invalid Unreal Project Folder",
            "The selected folder is not a valid Unreal project folder.");
    }
}

QList<QLayout *> PackageWindow::getChildLayouts(QLayout *vLayout) {
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
void PackageWindow::changeChildWidgetsBackgroundColor(QLayout *layout,
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
int PackageWindow::getConfigFromComboBox(QLayoutItem *layoutItem) {
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

QList<Options> PackageWindow::getOptionsFromLayout(QLayout *vLayout) {
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



void PackageWindow::deleteLayout(QLayout *vLayout, QLayout *parent) {
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

Options PackageWindow::getCurrentOptions(QComboBox *target,
                                      QComboBox *configuration,
                                      QComboBox *comboBox) {
    qDebug() << "map:" << comboBox->currentIndex()
             << "config:" << configuration->currentIndex()
             << "target:" << target->currentIndex();
    Options _options{comboBox->currentIndex(), configuration->currentIndex(),
                     target->currentIndex()};

    return _options;
}

QComboBox *PackageWindow::createOptionComboBox(QStringList options,
                                            int optionIndex, QLayout *vLayout) {
    QComboBox *comboBox = new QComboBox;
    comboBox->addItems(options);
    comboBox->setCurrentIndex(optionIndex % options.length());
    vLayout->addWidget(comboBox);
    return comboBox;
}

void PackageWindow::addPackageJob(bool init, Options options) {

    QHBoxLayout *vLayout = new QHBoxLayout;

    if (!init) {
        QCheckBox *check = new QCheckBox;
        vLayout->addWidget(check);
        check->setCheckState(Qt::Checked);
    }

    QComboBox *levelBox = createOptionComboBox(project->Levels(), options.map, vLayout);
    QComboBox *configBox =
        createOptionComboBox(project->BuildConfigs(), options.config, vLayout);
    QComboBox *targetBox =
        createOptionComboBox(project->BuildTargets(), options.target, vLayout);

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


void PackageWindow::saveSettings() {
    // Create a QSettings object
    QSettings settings("kykcbros", "packager");
    // Save the checkbox state in QSettings
    if(project){
        project->SaveSettings();
    }
    settings.setValue("outputFolder", outputFolder);
    settings.setValue("unrealDir", unrealDir);
    settings.setValue("deployerPath", deployerPath);
    settings.setValue("autoZip", bAutoZip);
    settings.setValue("deleteAfterZip", bDeleteAfterZip);
    settings.setValue("autoDeploy", bAutoDeploy);


}

void PackageWindow::retrieveSettings() {
    QSettings settings("kykcbros", "packager");

    unrealDir = settings.value("unrealDir", "").toString();
    outputFolder = settings.value("outputFolder", "").toString();
    deployerPath = settings.value("deployerPath", "").toString();
    bAutoZip = settings.value("autoZip", false).toBool();
    bDeleteAfterZip = settings.value("deleteAfterZip", false).toBool();
    bAutoDeploy = settings.value("autoDeploy", false).toBool();


    QString project_dir = settings.value("projectDir", "").toString();
    // Create new instance according to last project_dir from settings
    InitializeProject(project_dir);

}

