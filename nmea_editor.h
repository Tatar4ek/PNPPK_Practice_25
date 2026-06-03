#ifndef NMEA_EDITOR_H
#define NMEA_EDITOR_H

#include <QWidget>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QComboBox;
class QTableWidget;
class QTabWidget;

struct NmeaMessage {
    QString name;
    int freq;
    QString format;
};

struct NmeaPort {
    QString name;
    int baud;
    QStringList functions;
    QList<NmeaMessage> outMessages;
    QList<NmeaMessage> inMessages;
};

class NmeaEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NmeaEditorWidget(QWidget *parent = nullptr);

    void loadFromJson(const QJsonObject &root);
    void saveToJson(QJsonObject &root) const;
    void clear();

private slots:
    void onPortSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onBaudChanged(int index);
    void onFunctionItemChanged(QListWidgetItem *item);
    void onAddOutMessage();
    void onEditOutMessage();
    void onDeleteOutMessage();
    void onAddInMessage();
    void onEditInMessage();
    void onDeleteInMessage();

private:
    void updatePortList();
    void updatePortEditor();
    void saveCurrentPortFromUi();
    void showMessageDialog(bool isOut, int row = -1);

    QList<NmeaPort> m_ports;
    int m_currentPortIndex;

    QListWidget *m_portList;
    QComboBox *m_baudCombo;
    QListWidget *m_functionsList;
    QTabWidget *m_messagesTabWidget;
    QTableWidget *m_outTable;
    QTableWidget *m_inTable;
};

#endif // NMEA_EDITOR_H
