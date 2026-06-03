#include "nmea_editor.h"
#include "config_utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QListView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTabWidget>

NmeaEditorWidget::NmeaEditorWidget(QWidget *parent) : QWidget(parent), m_currentPortIndex(-1)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Левая панель
    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(new QLabel(QString::fromUtf8("<b>Список портов:</b>"), this));
    m_portList = new QListWidget(this);
    leftLayout->addWidget(m_portList);
    connect(m_portList, &QListWidget::currentItemChanged, this, &NmeaEditorWidget::onPortSelected);
    splitter->addWidget(leftPanel);

    // Правая панель
    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QGroupBox *portGroup = new QGroupBox(QString::fromUtf8("Настройки порта"), this);
    QFormLayout *portForm = new QFormLayout(portGroup);
    m_baudCombo = new QComboBox(this);
    m_baudCombo->setEditable(true);
    m_baudCombo->addItems(QStringList() << "4800" << "9600" << "19200" << "38400" << "57600" << "115200");
    portForm->addRow(QString::fromUtf8("Скорость (BAUD):"), m_baudCombo);
    connect(m_baudCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NmeaEditorWidget::onBaudChanged);

    m_functionsList = new QListWidget(this);
    m_functionsList->setViewMode(QListView::ListMode);
    QStringList funcs;
    funcs << "in_SNS" << "in_lag" << "in_IIB" << "in_INTR" << "out_INTR" << "in_PO5_new" << "out_PO5_new" << "out_tmt";
    for (const QString &f : funcs) {
        QListWidgetItem *item = new QListWidgetItem(f, m_functionsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    portForm->addRow(QString::fromUtf8("Активные функции:"), m_functionsList);
    connect(m_functionsList, &QListWidget::itemChanged, this, &NmeaEditorWidget::onFunctionItemChanged);
    rightLayout->addWidget(portGroup);

    m_messagesTabWidget = new QTabWidget(this);

    // OUT
    QWidget *outTab = new QWidget(this);
    QVBoxLayout *outLayout = new QVBoxLayout(outTab);
    m_outTable = new QTableWidget(0, 3, this);
    m_outTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("Имя") << QString::fromUtf8("Частота (Гц)") << QString::fromUtf8("Формат (MESSAGE)"));
    m_outTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_outTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_outTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    outLayout->addWidget(m_outTable);
    QHBoxLayout *outBtns = new QHBoxLayout();
    QPushButton *btnAdd = new QPushButton(QString::fromUtf8("+ Добавить"), this);
    QPushButton *btnEdit = new QPushButton(QString::fromUtf8("✎ Изменить"), this);
    QPushButton *btnDel = new QPushButton(QString::fromUtf8("✖ Удалить"), this);
    outBtns->addWidget(btnAdd); outBtns->addWidget(btnEdit); outBtns->addWidget(btnDel); outBtns->addStretch();
    outLayout->addLayout(outBtns);
    connect(btnAdd, &QPushButton::clicked, this, &NmeaEditorWidget::onAddOutMessage);
    connect(btnEdit, &QPushButton::clicked, this, &NmeaEditorWidget::onEditOutMessage);
    connect(btnDel, &QPushButton::clicked, this, &NmeaEditorWidget::onDeleteOutMessage);
    m_messagesTabWidget->addTab(outTab, QString::fromUtf8("Исходящие (OUT)"));

    // IN
    QWidget *inTab = new QWidget(this);
    QVBoxLayout *inLayout = new QVBoxLayout(inTab);
    m_inTable = new QTableWidget(0, 3, this);
    m_inTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("Имя") << QString::fromUtf8("Частота (Гц)") << QString::fromUtf8("Формат (MESSAGE)"));
    m_inTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_inTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_inTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inLayout->addWidget(m_inTable);
    QHBoxLayout *inBtns = new QHBoxLayout();
    QPushButton *btnAddIn = new QPushButton(QString::fromUtf8("+ Добавить"), this);
    QPushButton *btnEditIn = new QPushButton(QString::fromUtf8("✎ Изменить"), this);
    QPushButton *btnDelIn = new QPushButton(QString::fromUtf8("✖ Удалить"), this);
    inBtns->addWidget(btnAddIn); inBtns->addWidget(btnEditIn); inBtns->addWidget(btnDelIn); inBtns->addStretch();
    inLayout->addLayout(inBtns);
    connect(btnAddIn, &QPushButton::clicked, this, &NmeaEditorWidget::onAddInMessage);
    connect(btnEditIn, &QPushButton::clicked, this, &NmeaEditorWidget::onEditInMessage);
    connect(btnDelIn, &QPushButton::clicked, this, &NmeaEditorWidget::onDeleteInMessage);
    m_messagesTabWidget->addTab(inTab, QString::fromUtf8("Входящие (IN)"));

    rightLayout->addWidget(m_messagesTabWidget);
    rightLayout->addStretch();
    splitter->addWidget(rightPanel);
    splitter->setSizes(QList<int>() << 250 << 850);
    mainLayout->addWidget(splitter);
}

void NmeaEditorWidget::loadFromJson(const QJsonObject &root) {
    m_ports.clear();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        QString pName = ConfigUtils::cleanKey(it.key());
        QJsonObject pObj = it.value().toObject();
        NmeaPort port;
        port.name = pName;
        port.baud = pObj.value(ConfigUtils::cleanKey("BAUD")).toInt(4800);

        for (auto fIt = pObj.constBegin(); fIt != pObj.constEnd(); ++fIt) {
            QString key = ConfigUtils::cleanKey(fIt.key());
            if (key != "BAUD" && key != "OUT" && key != "IN" && fIt.value().toInt() == 1) {
                port.functions.append(key);
            }
        }
        if (pObj.contains(ConfigUtils::cleanKey("OUT"))) {
            QJsonObject outObj = pObj.value(ConfigUtils::cleanKey("OUT")).toObject();
            for (auto mIt = outObj.constBegin(); mIt != outObj.constEnd(); ++mIt) {
                NmeaMessage msg;
                msg.name = ConfigUtils::cleanKey(mIt.key());
                QJsonObject mObj = mIt.value().toObject();
                msg.freq = mObj.value(ConfigUtils::cleanKey("FREQ")).toInt(1);
                msg.format = mObj.value(ConfigUtils::cleanKey("MESSAGE")).toString();
                port.outMessages.append(msg);
            }
        }
        if (pObj.contains(ConfigUtils::cleanKey("IN"))) {
            QJsonObject inObj = pObj.value(ConfigUtils::cleanKey("IN")).toObject();
            for (auto mIt = inObj.constBegin(); mIt != inObj.constEnd(); ++mIt) {
                NmeaMessage msg;
                msg.name = ConfigUtils::cleanKey(mIt.key());
                QJsonObject mObj = mIt.value().toObject();
                msg.freq = mObj.value(ConfigUtils::cleanKey("FREQ")).toInt(1);
                msg.format = mObj.value(ConfigUtils::cleanKey("MESSAGE")).toString();
                port.inMessages.append(msg);
            }
        }
        m_ports.append(port);
    }
    m_currentPortIndex = -1;
    updatePortList();
}

void NmeaEditorWidget::saveToJson(QJsonObject &root) const {
    const_cast<NmeaEditorWidget*>(this)->saveCurrentPortFromUi();
    for (const NmeaPort &port : m_ports) {
        QJsonObject pObj;
        pObj["BAUD"] = port.baud;
        for (const QString &f : port.functions) pObj[f] = 1;

        if (!port.outMessages.isEmpty()) {
            QJsonObject outObj;
            for (const NmeaMessage &m : port.outMessages) {
                QJsonObject mObj; mObj["FREQ"] = m.freq; mObj["MESSAGE"] = m.format;
                outObj[m.name] = mObj;
            }
            pObj["OUT"] = outObj;
        }
        if (!port.inMessages.isEmpty()) {
            QJsonObject inObj;
            for (const NmeaMessage &m : port.inMessages) {
                QJsonObject mObj; mObj["FREQ"] = m.freq; mObj["MESSAGE"] = m.format;
                inObj[m.name] = mObj;
            }
            pObj["IN"] = inObj;
        }
        root[port.name] = pObj;
    }
}

void NmeaEditorWidget::clear() {
    m_ports.clear();
    m_currentPortIndex = -1;
    updatePortList();
}

void NmeaEditorWidget::updatePortList() {
    m_portList->clear();
    for (const NmeaPort &p : m_ports) m_portList->addItem(p.name);
    if (!m_ports.isEmpty()) {
        m_portList->setCurrentRow(0); // Это автоматически вызовет onPortSelected и обновит правую панель!
    } else {
        m_currentPortIndex = -1;
    }
}

void NmeaEditorWidget::onPortSelected(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous);
    saveCurrentPortFromUi();
    m_currentPortIndex = current ? m_portList->row(current) : -1;
    updatePortEditor();
}

void NmeaEditorWidget::updatePortEditor() {
    if (m_currentPortIndex < 0 || m_currentPortIndex >= m_ports.size()) return;
    const NmeaPort &p = m_ports.at(m_currentPortIndex);

    int idx = m_baudCombo->findText(QString::number(p.baud));
    m_baudCombo->setCurrentIndex(idx >= 0 ? idx : -1);
    if (idx < 0) m_baudCombo->setCurrentText(QString::number(p.baud));

    m_functionsList->blockSignals(true);
    for (int i = 0; i < m_functionsList->count(); ++i) {
        QListWidgetItem *item = m_functionsList->item(i);
        if (item) item->setCheckState(p.functions.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
    }
    m_functionsList->blockSignals(false);

    m_outTable->setRowCount(p.outMessages.size());
    for (int i = 0; i < p.outMessages.size(); ++i) {
        m_outTable->setItem(i, 0, new QTableWidgetItem(p.outMessages.at(i).name));
        m_outTable->setItem(i, 1, new QTableWidgetItem(QString::number(p.outMessages.at(i).freq)));
        m_outTable->setItem(i, 2, new QTableWidgetItem(p.outMessages.at(i).format));
    }
    m_inTable->setRowCount(p.inMessages.size());
    for (int i = 0; i < p.inMessages.size(); ++i) {
        m_inTable->setItem(i, 0, new QTableWidgetItem(p.inMessages.at(i).name));
        m_inTable->setItem(i, 1, new QTableWidgetItem(QString::number(p.inMessages.at(i).freq)));
        m_inTable->setItem(i, 2, new QTableWidgetItem(p.inMessages.at(i).format));
    }
}

void NmeaEditorWidget::saveCurrentPortFromUi() {
    if (m_currentPortIndex < 0 || m_currentPortIndex >= m_ports.size()) return;
    NmeaPort &p = m_ports[m_currentPortIndex];
    p.baud = m_baudCombo->currentText().toInt();
    p.functions.clear();
    for (int i = 0; i < m_functionsList->count(); ++i) {
        QListWidgetItem *item = m_functionsList->item(i);
        if (item && item->checkState() == Qt::Checked) p.functions.append(item->text());
    }
    p.outMessages.clear();
    for (int i = 0; i < m_outTable->rowCount(); ++i) {
        if (m_outTable->item(i, 0) && m_outTable->item(i, 1) && m_outTable->item(i, 2)) {
            NmeaMessage m; m.name = m_outTable->item(i, 0)->text().trimmed();
            m.freq = m_outTable->item(i, 1)->text().toInt(); m.format = m_outTable->item(i, 2)->text();
            p.outMessages.append(m);
        }
    }
    p.inMessages.clear();
    for (int i = 0; i < m_inTable->rowCount(); ++i) {
        if (m_inTable->item(i, 0) && m_inTable->item(i, 1) && m_inTable->item(i, 2)) {
            NmeaMessage m; m.name = m_inTable->item(i, 0)->text().trimmed();
            m.freq = m_inTable->item(i, 1)->text().toInt(); m.format = m_inTable->item(i, 2)->text();
            p.inMessages.append(m);
        }
    }
}

void NmeaEditorWidget::onBaudChanged(int) {}
void NmeaEditorWidget::onFunctionItemChanged(QListWidgetItem *) {}

void NmeaEditorWidget::showMessageDialog(bool isOut, int row) {
    if (m_currentPortIndex < 0) { QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8("Выберите порт.")); return; }
    bool ok = false;
    QString curName = "", curFreq = "1", curFmt = "&$";
    QTableWidget *tbl = isOut ? m_outTable : m_inTable;
    if (row >= 0) { curName = tbl->item(row, 0)->text(); curFreq = tbl->item(row, 1)->text(); curFmt = tbl->item(row, 2)->text(); }

    QString name = QInputDialog::getText(this, isOut ? QString::fromUtf8("Исходящее") : QString::fromUtf8("Входящее"), QString::fromUtf8("Имя:"), QLineEdit::Normal, curName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    int freq = QInputDialog::getInt(this, QString::fromUtf8("Частота"), QString::fromUtf8("Гц:"), curFreq.toInt(), 1, 100, 1, &ok);
    if (!ok) return;
    QString fmt = QInputDialog::getText(this, QString::fromUtf8("Формат"), QString::fromUtf8("Строка (напр. &$HEHPR...):"), QLineEdit::Normal, curFmt, &ok);
    if (!ok) return;

    if (row >= 0) {
        tbl->item(row, 0)->setText(name.trimmed()); tbl->item(row, 1)->setText(QString::number(freq)); tbl->item(row, 2)->setText(fmt);
    } else {
        int r = tbl->rowCount(); tbl->insertRow(r);
        tbl->setItem(r, 0, new QTableWidgetItem(name.trimmed())); tbl->setItem(r, 1, new QTableWidgetItem(QString::number(freq))); tbl->setItem(r, 2, new QTableWidgetItem(fmt));
        tbl->selectRow(r);
    }
}

void NmeaEditorWidget::onAddOutMessage() { showMessageDialog(true, -1); }
void NmeaEditorWidget::onEditOutMessage() { int r = m_outTable->currentRow(); if (r < 0) return; showMessageDialog(true, r); }
void NmeaEditorWidget::onDeleteOutMessage() { int r = m_outTable->currentRow(); if (r < 0) return; if (QMessageBox::question(this, QString::fromUtf8("Удалить?"), QString::fromUtf8("Удалить сообщение?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) m_outTable->removeRow(r); }
void NmeaEditorWidget::onAddInMessage() { showMessageDialog(false, -1); }
void NmeaEditorWidget::onEditInMessage() { int r = m_inTable->currentRow(); if (r < 0) return; showMessageDialog(false, r); }
void NmeaEditorWidget::onDeleteInMessage() { int r = m_inTable->currentRow(); if (r < 0) return; if (QMessageBox::question(this, QString::fromUtf8("Удалить?"), QString::fromUtf8("Удалить сообщение?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) m_inTable->removeRow(r); }
