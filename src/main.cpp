#include "MMTMainWindow.hpp"
#include "MMTSettings.hpp"

#include <QtWidgets/QApplication>
#include <QScreen>
#include <QtGlobal>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QDateTime>

#include <windows.h>

BOOL IsElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

static MMT::MainWindow* s_mainWindow = nullptr;

static const QtMessageHandler QT_DEFAULT_MESSAGE_HANDLER = qInstallMessageHandler(0);

static QFile s_logFile;

void MessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    bool log = false;
    const QStringList msgTypeStringList = {"Debug", "Info", "Warning", "Critical", "Fatal"};
    const QList<QtMsgType> msgTypeList = { QtDebugMsg, QtInfoMsg, QtWarningMsg, QtCriticalMsg, QtFatalMsg};
    auto currentLogLevelString = MMT::Settings::instance()->logLevel();

    if (msgTypeList.indexOf(type) >= msgTypeStringList.indexOf(currentLogLevelString))
    {
        if (s_mainWindow)
        {
            QMetaObject::invokeMethod(s_mainWindow, "appendMessage", Qt::AutoConnection, Q_ARG(const QString&, msg), Q_ARG(QtMsgType, type));
        }

        QTextStream out(&s_logFile);
        out << QDateTime::currentDateTime().toString("[yyyy-MM-dd_hh:mm:ss][%1]").arg(currentLogLevelString) << msg << "\n";
        out.flush();
    }

    (*QT_DEFAULT_MESSAGE_HANDLER)(type, context, msg);
}

int main(int argc, char *argv[])
{
    s_logFile.setFileName("log.txt");
    s_logFile.open(QIODevice::WriteOnly | QIODevice::Append);

    qInstallMessageHandler(MessageHandler);

    QApplication app(argc, argv);

    QProcess tasklist;
    tasklist.start("tasklist", QStringList() << "/NH" << "/FI" << QString("IMAGENAME eq %1").arg("MultiMonitorTools.exe"));
    tasklist.waitForFinished();
    QString output = tasklist.readAllStandardOutput();

    if (output.count("MultiMonitorTools") > 1)
    {
        QMessageBox messageBox;
        messageBox.setInformativeText("The program is already running.");
        messageBox.setWindowTitle("Error");
        messageBox.exec();
        return 1;
    }

    MMT::MainWindow w;
    s_mainWindow = &w;

    auto geom = qApp->primaryScreen()->geometry();
    w.setGeometry(geom.adjusted(geom.width() * 0.05, geom.height() * 0.05, -geom.width() * 0.05, -geom.height() * 0.05));

    w.show();
    auto result = app.exec();
    s_logFile.close();

    return result;
}
