#ifndef MIL_EDITOR_H
#define MIL_EDITOR_H

#include <QWidget>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QComboBox;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;

struct MilDataField {
    QString name;
    QString type;
    int word;
    bool isSigned;
    int wordl, wordh;
    QString var;
    double cmr;
    QString scale;
    int length;
    int offset;
    QString values;
    QString value;
};

struct MilArray {
    QString name;
    int address;
    QString direction;
    int subaddr_A;
    int subaddr_B;
    int size;
    QList<MilDataField> dataFields;
};

struct MilConfig {
    QString mode;
    QString bus;
    bool slave_sends;
    QList<int> ptc_addr;
    int address;
    QList<MilArray> arrays;
};

class MilEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MilEditorWidget(QWidget *parent = nullptr);

    void loadFromJson(const QJsonObject &root);
    QString generateOrderedJson() const;
    void clear();

private slots:
    void onArraySelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onModeChanged(const QString &mode);
    void onAddField();
    void onEditField();
    void onDeleteField();

private:
    void updateArrayList();
    void updateArrayEditor();
    void saveCurrentArrayFromUi();
    void showFieldDialog(bool isEdit, int row = -1);

    MilConfig m_config;
    int m_currentArrayIndex;

    QComboBox *m_modeCombo;
    QComboBox *m_busCombo;
    QCheckBox *m_slaveSendsCheck;
    QLineEdit *m_ptcAddrEdit;
    QSpinBox *m_rtAddressSpin;

    QListWidget *m_arrayList;
    QSpinBox *m_arrAddressSpin;
    QComboBox *m_arrDirectionCombo;
    QSpinBox *m_arrSubaddrASpin;
    QSpinBox *m_arrSubaddrBSpin;
    QSpinBox *m_arrSizeSpin;
    QTableWidget *m_fieldsTable;
};

#endif // MIL_EDITOR_H
