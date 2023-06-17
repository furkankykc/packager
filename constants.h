#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QDebug>
#include <QFile>
#define UNREAL_PATH "F:/Epic Games/UE_5.0"
#define AUTOMATION_TOOL                                                        \
    "/Engine/Binaries/DotNET/AutomationTool/AutomationTool.exe"
#define COMMAND                                                                \
    R"""(BuildCookRun -project="%1" -noP4 -clientconfig=%2 -serverconfig=%2 -nocompile -nocompileeditor -installed -unrealexe="%3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -utf8output %4 -build -cook -unversionedcookedcontent -ddc=InstalledDerivedDataBackendGraph -compressed -deploy -unattended -stage -pak -prereqs -package -archive -archivedirectory="%5" -createreleaseversion=%6 )"""
#define _                                                                      \
    R"""(
client ->Nogenre+ -client flag after platform
server -> Nogenre + platform<->serverplatform before and after -server flag also -noclient twice ==

server -> -server -serverplatform=Win64 -server -noclient -noclient
client -> -platform=Win64 -client
default-> -platform=Win64


)"""
#define FLAGS_DEFAULT "-platform=Win64"
#define FLAGS_CLIENT "-platform=Win64 -client"
#define FLAGS_SERVER "-server -serverplatform=Win64 -server -noclient -noclient"

QString convertConfigFormat(QString gameDir, QString map) {
    QString levelName = map.split("/").last().replace(".umap", "");
    QString convertedMapPath =
        map.replace(gameDir + "/Content", "/Game").replace("umap", levelName);
    return convertedMapPath;
}
QString escapeSpaces(const QString &path) {
    QStringList splited = path.split("/");
    QStringList new_str_list;
    foreach (QString slice, splited) {
        if (slice.contains(" ")) {
            new_str_list.append(QString("\"%1\"").arg(slice));
        } else {
            new_str_list.append(slice);
        }
    }
    qDebug() << "ESCAPED:" << new_str_list.join("/") << "::" << new_str_list;
    return new_str_list.join("/");
}
void writeAfterALine(QString path, QString keyBefore, QString key,
                     QString value) {
    QFile file(path);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream in(&file);
        QStringList lines;
        lines.clear();
        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.contains(keyBefore)) {
                QString newLine =
                    key + value; // Set the line with the new value
                lines.append(newLine);
            }

            lines.append(line);
        }

        file.resize(0); // Clear the file content
        QTextStream out(&file);
        for (const QString &line : lines) {
            out << line << "\n"; // Write the modified lines to the file
        }

        file.close();
    }
}
void writeIni(QString path, QString key, QString value) {

    QFile file(path);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream in(&file);
        QStringList lines;
        bool keyFound = false;

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.contains(key)) {
                keyFound = true;
                line = key + value; // Set the line with the new value
            }

            lines.append(line);
        }

        file.resize(0); // Clear the file content
        QTextStream out(&file);
        for (const QString &line : lines) {
            out << line << "\n"; // Write the modified lines to the file
        }

        file.close();
        if (!keyFound) {
            writeAfterALine(path, "GameDefaultMap=", key, value);
        }
    }
}

// 1->project path
// 2->config
// 3->unreal path
// 4->target flags
#endif // CONSTANTS_H
