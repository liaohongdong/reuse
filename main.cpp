#include <QCoreApplication>
#include <iostream>
#include <QCommandLineParser>
#include <QThread>
#include <QProcess>
#include <QDir>
#include <unistd.h>
#include <QRegExp>
#include <QFile>
#include <QTextStream>

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("reuse");
    QCoreApplication::setApplicationVersion("0.1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("============================ For Nextop ============================ \n"
                                     "This script relies on md-gamma-web as the base project to run in a subdirectory\n"
                                     "This command line is used to convert a single vue file into a signle javscript file.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file address"));
    parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination folder address"));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() == 4) {
        QProcess builder;
        builder.setProcessChannelMode(QProcess::SeparateChannels);

//        QDir sourceDir("/Users/hongdong.liao/Desktop/md-gamma-web/md-gamma-purchase"); // 源码项目所在
        QDir sourceDir(args[0] + "/" + args[1]); // 源码项目所在
        QDir sourceDistDir(sourceDir.path().toStdString().data() + QString("/dist")); // 源码打包目录地址
        QDir sourceNodeModuleDir(sourceDir.path().toStdString().data() + QString("/node_modules")); // 源码依赖目录地址
//        QString sourceCodeDir = sourceDir.relativeFilePath("src/views/components/orderDialog.vue"); // 源码目录地址
        QString sourceCodeDir = sourceDir.relativeFilePath(args[2]); // 源码目录地址

//        QDir targetDir("/Users/hongdong.liao/Desktop/md-gamma-web/md-gamma-main"); // 根目录地址
        QDir targetDir(args[0]); // 根目录地址
        QDir targetDistDir(targetDir.path() + "/dist");

        chdir(sourceDir.path().toStdString().data()); // 跳转至源码目录

        // 生成之前需要修改output的library
        // 字符串分割
        if (!args[3].contains(".js")) {
            qDebug("Invalid parameter");
            return -1;
        }
        QStringList spList = args[3].split(".");
        // 替换文件
        QFile vueConfigJs(sourceDir.path().toStdString().data() + QString("/vue.config.js"));
        QFile vueConfigOutJs(sourceDir.path().toStdString().data() + QString("/vue.config.js.bak"));
        if (!vueConfigJs.open(QIODevice::ReadWrite | QIODevice::Text)) { // 打开文件
            qDebug("File read and write error for vue.config.js");
            return -1;
        }
        if (!vueConfigOutJs.open(QIODevice::ReadWrite | QIODevice::Text)) { // 打开文件
            qDebug("File read and write error for vue.config.js");
            return -1;
        }
        // args[3] order-dialog.min.js       args[1] md-gamma-purchase
        while(!vueConfigJs.atEnd()) {
            QString line = vueConfigJs.readLine();
            line = line.remove('\n');
            line.replace(QRegExp(args[1]), spList[0]);
            QTextStream out(&vueConfigOutJs);
            out << line + "\n";
        }
        vueConfigJs.close();
        vueConfigOutJs.close();
        sourceDir.remove("vue.config.js");
        sourceDir.rename("vue.config.js.bak", "vue.config.js");
        // 替换文件

        QString command;
        command = QString("./node_modules/.bin/vue-cli-service build --target lib %1 %2")
                .arg(sourceCodeDir)
//                .arg(" --dest dist orderDialog.js")
                .arg(" --dest dist " + args[3] + " > /dev/null 2>&1");
        if (sourceNodeModuleDir.isEmpty()) { // 如果依赖目录为空
            qDebug("Please check the node_modules directory.");
            return -1;
        }
//        builder.execute(command); // 进行源码打包
        builder.start(command);
        builder.waitForFinished();

        QString currentPath = QDir::currentPath();
        chdir("dist");
        currentPath = QDir::currentPath();
        QDir dir(currentPath);

        if (!dir.exists()) {
            qDebug("soruce address not exist %s", currentPath.toStdString().data());
            return -1;
        }
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        dir.setSorting(QDir::DirsFirst);
        QFileInfoList fileList = dir.entryInfoList();
        if (fileList.size() < -1 || fileList.size() == 0) {
            qDebug("There are no files in this directory %s", currentPath.toStdString().data());
            return -1;
        }

        int index = 0;
        do {
            QFileInfo fileInfo = fileList.at(index);
//            cout << fileInfo.fileName().toStdString().data() << endl;
            QRegExp regJsFileName("(.*min.js$)");
            int jsResult = regJsFileName.indexIn(fileInfo.fileName());
            if (jsResult > -1) {
                // common js 文件夹
//                QString targetAddress = "/Users/hongdong.liao/Desktop/md-gamma-web/md-gamma-main/public/common/js/";
                QString targetJsAddress = args[0] + "/md-gamma-main/public/common/js/";
                QString targetJsFileAddress = targetJsAddress + args[3];
                QFile targetFile(targetJsFileAddress);
                if (targetFile.exists()) { // 文件存在删除
                    targetFile.remove();
                }
                QFile moveFile(fileInfo.filePath());
                QFile::copy(fileInfo.filePath(), targetJsFileAddress);
            }

            QRegExp regCssFileName("(.*.css$)");
            int cssResult = regCssFileName.indexIn(fileInfo.fileName());
            if (cssResult > -1) {
                // common js 文件夹
//                QString targetAddress = "/Users/hongdong.liao/Desktop/md-gamma-web/md-gamma-main/public/common/css/";
                QString targetCssAddress = args[0] + "/md-gamma-main/public/common/css/";
                QString targetCssFileAddress = targetCssAddress + spList[0] + QString(".css");
                qDebug("%s", targetCssFileAddress.toStdString().data());
                QFile targetFile(targetCssFileAddress);
                if (targetFile.exists()) { // 文件存在删除
                    targetFile.remove();
                }
                QFile moveFile(fileInfo.filePath());
                QFile::copy(fileInfo.filePath(), targetCssFileAddress);
            }
            ++index;
        } while(index < fileList.size());
        chdir(sourceDistDir.path().toStdString().data());
        QDir removeDist(sourceDistDir);
        removeDist.removeRecursively();
        qDebug("The generated code is successful, please go to md-gamma-main/public/common/js to view");

        // 还原文件
        QFile vueConfigRevert(sourceDir.path().toStdString().data() + QString("/vue.config.js"));
        QFile vueConfigOutRevert(sourceDir.path().toStdString().data() + QString("/vue.config.js.bak"));
        if (!vueConfigJs.open(QIODevice::ReadWrite | QIODevice::Text)) { // 打开文件
            qDebug("File read and write error for vue.config.js");
            return -1;
        }
        if (!vueConfigOutJs.open(QIODevice::ReadWrite | QIODevice::Text)) { // 打开文件
            qDebug("File read and write error for vue.config.js");
            return -1;
        }
        // args[3] order-dialog.min.js       args[1] md-gamma-purchase
        while(!vueConfigJs.atEnd()) {
            QString line = vueConfigJs.readLine();
            line = line.remove('\n');
            if (!line.contains(QString("'%1': '%2'").arg(spList[0]).arg(spList[0]))) {
                line.replace(QRegExp(spList[0]), args[1]);
            }
            QTextStream out(&vueConfigOutJs);
            out << line + "\n";
        }
        vueConfigJs.close();
        vueConfigOutJs.close();
        sourceDir.remove("vue.config.js");
        sourceDir.rename("vue.config.js.bak", "vue.config.js");
        // 还原文件

        QThread::msleep(300);
        exit(1);
    } else {
        qDebug("Incorrect parameter list");
        QThread::msleep(300);
        exit(1);
    }
    return app.exec();
}
