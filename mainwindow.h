#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>

class QStackedWidget;
class QLabel;
class NmeaEditorWidget;
class MilEditorWidget;

enum class Protocol { UNKNOWN, NMEA, MIL1553 };

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onActionOpen();
    void onActionSave();
    void onActionNewNmea();
    void onActionNewMil();

private:
    void setupUi();
    Protocol detectProtocol(const QJsonObject &root);
    void switchToProtocol(Protocol proto);
    bool loadJson(const QString &filePath);
    bool saveJson(const QString &filePath);

    Protocol m_currentProtocol;
    QString m_currentFilePath;

    QStackedWidget *m_stackedWidget;
    QLabel *m_statusLabel;

    NmeaEditorWidget *m_nmeaEditor;
    MilEditorWidget *m_milEditor;
};

#endif // MAINWINDOW_H
