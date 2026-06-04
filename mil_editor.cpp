#include "mil_editor.h"
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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QStatusBar>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QDialog>
#include <QDialogButtonBox>

MilEditorWidget::MilEditorWidget(QWidget *parent) : QWidget(parent), m_currentArrayIndex(-1)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    QWidget *leftPanel = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->addWidget(new QLabel(QString::fromUtf8("<b>Список массивов (Arrays):</b>"), this));
    m_arrayList = new QListWidget(this);
    leftLayout->addWidget(m_arrayList);
    connect(m_arrayList, &QListWidget::currentItemChanged, this, &MilEditorWidget::onArraySelected);
    splitter->addWidget(leftPanel);

    QWidget *rightPanel = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);

    QGroupBox *globalGroup = new QGroupBox(QString::fromUtf8("Глобальные настройки"), this);
    QFormLayout *globalForm = new QFormLayout(globalGroup);
    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItems(QStringList() << "BC" << "RT");
    globalForm->addRow(QString::fromUtf8("Режим (mode):"), m_modeCombo);
    connect(m_modeCombo, &QComboBox::currentTextChanged, this, &MilEditorWidget::onModeChanged);

    m_busCombo = new QComboBox(this);
    m_busCombo->addItems(QStringList() << "A" << "B" << "AB");
    globalForm->addRow(QString::fromUtf8("Шина (bus):"), m_busCombo);

    m_slaveSendsCheck = new QCheckBox(QString::fromUtf8("Ведомый канал ведет обмен (slave_sends)"), this);
    globalForm->addRow(m_slaveSendsCheck);

    m_ptcAddrEdit = new QLineEdit(this);
    globalForm->addRow(QString::fromUtf8("Адреса ПТЦ (ptc_addr, через запятую):"), m_ptcAddrEdit);

    m_rtAddressSpin = new QSpinBox(this);
    m_rtAddressSpin->setRange(0, 31);
    globalForm->addRow(QString::fromUtf8("Адрес ОУ (address, для RT):"), m_rtAddressSpin);
    rightLayout->addWidget(globalGroup);

    QGroupBox *arrGroup = new QGroupBox(QString::fromUtf8("Настройки массива"), this);
    QFormLayout *arrForm = new QFormLayout(arrGroup);
    m_arrAddressSpin = new QSpinBox(this); m_arrAddressSpin->setRange(0, 31);
    arrForm->addRow(QString::fromUtf8("Адрес (address):"), m_arrAddressSpin);
    m_arrDirectionCombo = new QComboBox(this);
    m_arrDirectionCombo->addItems(QStringList() << "send" << "receive");
    arrForm->addRow(QString::fromUtf8("Направление (direction):"), m_arrDirectionCombo);
    m_arrSubaddrASpin = new QSpinBox(this); m_arrSubaddrASpin->setRange(0, 31);
    arrForm->addRow(QString::fromUtf8("Подадрес А (subaddr_A):"), m_arrSubaddrASpin);
    m_arrSubaddrBSpin = new QSpinBox(this); m_arrSubaddrBSpin->setRange(0, 31);
    arrForm->addRow(QString::fromUtf8("Подадрес В (subaddr_B):"), m_arrSubaddrBSpin);
    m_arrSizeSpin = new QSpinBox(this); m_arrSizeSpin->setRange(1, 32);
    arrForm->addRow(QString::fromUtf8("Размер (size, слов):"), m_arrSizeSpin);
    rightLayout->addWidget(arrGroup);

    QGroupBox *fieldsGroup = new QGroupBox(QString::fromUtf8("Поля данных (data)"), this);
    QVBoxLayout *fieldsLayout = new QVBoxLayout(fieldsGroup);
    m_fieldsTable = new QTableWidget(0, 4, this);
    m_fieldsTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("Имя") << QString::fromUtf8("Тип") << QString::fromUtf8("Слово(а)") << QString::fromUtf8("Детали"));
    m_fieldsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_fieldsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fieldsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fieldsLayout->addWidget(m_fieldsTable);

    QHBoxLayout *fieldBtns = new QHBoxLayout();
    QPushButton *btnAdd = new QPushButton(QString::fromUtf8("+ Добавить поле"), this);
    QPushButton *btnEdit = new QPushButton(QString::fromUtf8("✎ Изменить"), this);
    QPushButton *btnDel = new QPushButton(QString::fromUtf8("✖ Удалить"), this);
    fieldBtns->addWidget(btnAdd); fieldBtns->addWidget(btnEdit); fieldBtns->addWidget(btnDel); fieldBtns->addStretch();
    fieldsLayout->addLayout(fieldBtns);
    connect(btnAdd, &QPushButton::clicked, this, &MilEditorWidget::onAddField);
    connect(btnEdit, &QPushButton::clicked, this, &MilEditorWidget::onEditField);
    connect(btnDel, &QPushButton::clicked, this, &MilEditorWidget::onDeleteField);

    rightLayout->addWidget(fieldsGroup);
    rightLayout->addStretch();
    splitter->addWidget(rightPanel);
    splitter->setSizes(QList<int>() << 250 << 850);
    mainLayout->addWidget(splitter);

    onModeChanged("BC");
}

void MilEditorWidget::onModeChanged(const QString &mode) {
    bool isBC = (mode == "BC");
    m_busCombo->setEnabled(isBC);
    m_ptcAddrEdit->setEnabled(isBC);
    m_rtAddressSpin->setEnabled(!isBC);
}

void MilEditorWidget::loadFromJson(const QJsonObject &root) {
    m_config.mode = root.value(ConfigUtils::cleanKey("mode")).toString("BC");
    m_config.bus = root.value(ConfigUtils::cleanKey("bus")).toString("AB");
    m_config.slave_sends = root.value(ConfigUtils::cleanKey("slave_sends")).toBool(false);
    m_config.address = root.value(ConfigUtils::cleanKey("address")).toInt(1);
    m_config.ptc_addr.clear();
    if (root.contains(ConfigUtils::cleanKey("ptc_addr"))) {
        QJsonArray ptcArr = root.value(ConfigUtils::cleanKey("ptc_addr")).toArray();
        for (int i = 0; i < ptcArr.size(); ++i) m_config.ptc_addr.append(ptcArr.at(i).toInt());
    }
    m_config.arrays.clear();
    if (root.contains(ConfigUtils::cleanKey("arrays"))) {
        QJsonObject arrsObj = root.value(ConfigUtils::cleanKey("arrays")).toObject();
        for (auto it = arrsObj.constBegin(); it != arrsObj.constEnd(); ++it) {
            MilArray arr;
            arr.name = ConfigUtils::cleanKey(it.key());
            QJsonObject aObj = it.value().toObject();
            arr.address = aObj.value(ConfigUtils::cleanKey("address")).toInt(31);
            arr.direction = aObj.value(ConfigUtils::cleanKey("direction")).toString("send");
            arr.subaddr_A = aObj.value(ConfigUtils::cleanKey("subaddr_A")).toInt(0);
            arr.subaddr_B = aObj.value(ConfigUtils::cleanKey("subaddr_B")).toInt(0);
            arr.size = aObj.value(ConfigUtils::cleanKey("size")).toInt(1);
            if (aObj.contains(ConfigUtils::cleanKey("data"))) {
                QJsonObject dObj = aObj.value(ConfigUtils::cleanKey("data")).toObject();
                for (auto dIt = dObj.constBegin(); dIt != dObj.constEnd(); ++dIt) {
                    MilDataField field;
                    field.name = ConfigUtils::cleanKey(dIt.key());
                    QJsonObject fObj = dIt.value().toObject();
                    field.type = fObj.value(ConfigUtils::cleanKey("type")).toString("word");
                    field.word = fObj.value(ConfigUtils::cleanKey("word")).toInt(1);
                    field.isSigned = fObj.value(ConfigUtils::cleanKey("signed")).toBool(false);
                    field.wordl = fObj.value(ConfigUtils::cleanKey("wordl")).toInt(1);
                    field.wordh = fObj.value(ConfigUtils::cleanKey("wordh")).toInt(1);
                    field.var = fObj.value(ConfigUtils::cleanKey("var")).toString("");
                    field.cmr = fObj.value(ConfigUtils::cleanKey("cmr")).toDouble(0.0);
                    field.scale = fObj.value(ConfigUtils::cleanKey("scale")).toString("");
                    field.length = fObj.value(ConfigUtils::cleanKey("length")).toInt(1);
                    field.offset = fObj.value(ConfigUtils::cleanKey("offset")).toInt(4);
                    field.values = fObj.value(ConfigUtils::cleanKey("values")).toString("");
                    field.value = fObj.value(ConfigUtils::cleanKey("value")).toString("0");
                    arr.dataFields.append(field);
                }
            }
            m_config.arrays.append(arr);
        }
    }
    m_modeCombo->setCurrentText(m_config.mode);
    m_busCombo->setCurrentText(m_config.bus);
    m_slaveSendsCheck->setChecked(m_config.slave_sends);
    QStringList ptcStr;
    for (int p : m_config.ptc_addr) ptcStr.append(QString::number(p));
    m_ptcAddrEdit->setText(ptcStr.join(", "));
    m_rtAddressSpin->setValue(m_config.address);
    m_currentArrayIndex = -1;
    updateArrayList();
}

QString MilEditorWidget::generateOrderedJson() const {
    const_cast<MilEditorWidget*>(this)->saveCurrentArrayFromUi();

    QString json = "{\n";

    json += QString("  \"mode\": \"") + m_config.mode + "\",\n";
    json += QString("  \"bus\": \"") + m_config.bus + "\",\n";
    json += QString("  \"slave_sends\": ") + (m_config.slave_sends ? "true" : "false") + ",\n";
    json += QString("  \"address\": ") + QString::number(m_config.address) + ",\n";

    if (!m_config.ptc_addr.isEmpty()) {
        json += QString("  \"ptc_addr\": [");
        QStringList ptcStrList;
        for (int p : m_config.ptc_addr) ptcStrList.append(QString::number(p));
        json += ptcStrList.join(", ") + "],\n";
    }

    json += QString("  \"arrays\": {\n");
    for (int i = 0; i < m_config.arrays.size(); ++i) {
        const MilArray &arr = m_config.arrays.at(i);
        json += QString("    \"") + arr.name + "\": {\n";

        json += QString("      \"address\": ") + QString::number(arr.address) + ",\n";
        json += QString("      \"direction\": \"") + arr.direction + "\",\n";
        json += QString("      \"subaddr_A\": ") + QString::number(arr.subaddr_A) + ",\n";
        json += QString("      \"subaddr_B\": ") + QString::number(arr.subaddr_B) + ",\n";
        json += QString("      \"size\": ") + QString::number(arr.size) + ",\n";
        json += QString("      \"data\": {\n");

        for (int j = 0; j < arr.dataFields.size(); ++j) {
            const MilDataField &f = arr.dataFields.at(j);
            json += QString("        \"") + f.name + "\": {\n";

            json += QString("          \"type\": \"") + f.type + "\"";

            if (f.type == "word" || f.type == "bits" || f.type == "const" || f.type == "counter" || f.type == "checksum")
                json += QString(",\n          \"word\": ") + QString::number(f.word);

            if (f.type == "dword") {
                json += QString(",\n          \"wordl\": ") + QString::number(f.wordl);
                json += QString(",\n          \"wordh\": ") + QString::number(f.wordh);
            }

            if (f.type == "word" || f.type == "dword" || f.type == "bits")
                json += QString(",\n          \"var\": \"") + f.var + "\"";

            if (f.type == "word" || f.type == "dword") {
                json += QString(",\n          \"signed\": ") + (f.isSigned ? "true" : "false");
                json += QString(",\n          \"cmr\": ") + QString::number(f.cmr);
                if (!f.scale.isEmpty())
                    json += QString(",\n          \"scale\": \"") + f.scale + "\"";
            }

            if (f.type == "bits") {
                json += QString(",\n          \"length\": ") + QString::number(f.length);
                json += QString(",\n          \"offset\": ") + QString::number(f.offset);
                if (!f.values.isEmpty())
                    json += QString(",\n          \"values\": \"") + f.values + "\"";
            }

            if (f.type == "const")
                json += QString(",\n          \"value\": \"") + f.value + "\"";

            json += QString("\n        }") + (j < arr.dataFields.size() - 1 ? "," : "") + "\n";
        }
        json += QString("      }\n    }") + (i < m_config.arrays.size() - 1 ? "," : "") + "\n";
    }
    json += QString("  }\n}");

    return json;
}

void MilEditorWidget::clear() {
    m_config = MilConfig();
    m_config.mode = "BC"; m_config.bus = "AB"; m_config.slave_sends = false; m_config.address = 1;
    m_currentArrayIndex = -1;
    updateArrayList();
}

void MilEditorWidget::updateArrayList() {
    m_arrayList->clear();
    for (const MilArray &a : m_config.arrays) m_arrayList->addItem(a.name);
    if (!m_config.arrays.isEmpty()) {
        m_arrayList->setCurrentRow(0); // Вызовет onArraySelected и обновит UI
    } else {
        m_currentArrayIndex = -1;
    }
}

void MilEditorWidget::onArraySelected(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous);
    saveCurrentArrayFromUi();
    m_currentArrayIndex = current ? m_arrayList->row(current) : -1;
    updateArrayEditor();
}

void MilEditorWidget::updateArrayEditor() {
    if (m_currentArrayIndex < 0 || m_currentArrayIndex >= m_config.arrays.size()) return;
    const MilArray &a = m_config.arrays.at(m_currentArrayIndex);
    m_arrAddressSpin->setValue(a.address);
    m_arrDirectionCombo->setCurrentText(a.direction);
    m_arrSubaddrASpin->setValue(a.subaddr_A);
    m_arrSubaddrBSpin->setValue(a.subaddr_B);
    m_arrSizeSpin->setValue(a.size);

    m_fieldsTable->setRowCount(a.dataFields.size());
    for (int i = 0; i < a.dataFields.size(); ++i) {
        const MilDataField &f = a.dataFields.at(i);
        m_fieldsTable->setItem(i, 0, new QTableWidgetItem(f.name));
        m_fieldsTable->setItem(i, 1, new QTableWidgetItem(f.type));
        QString wStr = QString::number(f.word);
        if (f.type == "dword") wStr = QString("%1 (L), %2 (H)").arg(f.wordl).arg(f.wordh);
        m_fieldsTable->setItem(i, 2, new QTableWidgetItem(wStr));
        QString det;
        if (f.type == "word" || f.type == "dword") { det = QString("var: %1, cmr: %2").arg(f.var).arg(f.cmr); if (f.isSigned) det += " [Signed]"; }
        else if (f.type == "bits") { det = QString("var: %1, len: %2, off: %3").arg(f.var).arg(f.length).arg(f.offset); }
        else if (f.type == "const") { det = QString("val: %1").arg(f.value); }
        else { det = QString("word: %1").arg(f.word); }
        m_fieldsTable->setItem(i, 3, new QTableWidgetItem(det));
    }
}

void MilEditorWidget::saveCurrentArrayFromUi() {
    if (m_currentArrayIndex < 0 || m_currentArrayIndex >= m_config.arrays.size()) return;
    MilArray &a = m_config.arrays[m_currentArrayIndex];
    a.address = m_arrAddressSpin->value();
    a.direction = m_arrDirectionCombo->currentText();
    a.subaddr_A = m_arrSubaddrASpin->value();
    a.subaddr_B = m_arrSubaddrBSpin->value();
    a.size = m_arrSizeSpin->value();
}

void MilEditorWidget::showFieldDialog(bool isEdit, int row) {
    if (m_currentArrayIndex < 0) {
        QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8("Сначала выберите массив."));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? QString::fromUtf8("Изменить поле данных") : QString::fromUtf8("Добавить поле данных"));
    dialog.setMinimumWidth(450);

    QVBoxLayout *mainL = new QVBoxLayout(&dialog);
    QFormLayout *formL = new QFormLayout();

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    formL->addRow(QString::fromUtf8("Имя параметра:"), nameEdit);

    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems(QStringList() << "word" << "dword" << "bits" << "const" << "counter" << "checksum");
    formL->addRow(QString::fromUtf8("Тип (type):"), typeCombo);

    QStackedWidget *stack = new QStackedWidget(&dialog);

    QWidget *wWord = new QWidget(&dialog);
    QFormLayout *fWord = new QFormLayout(wWord);
    QSpinBox *spinWord = new QSpinBox(wWord); spinWord->setRange(1, 32);
    QLineEdit *editVarW = new QLineEdit(wWord);
    QCheckBox *chkSignedW = new QCheckBox(QString::fromUtf8("Знаковое (signed)"), wWord);
    QDoubleSpinBox *spinCmrW = new QDoubleSpinBox(wWord); spinCmrW->setRange(-99999, 99999); spinCmrW->setDecimals(4);
    QLineEdit *editScaleW = new QLineEdit(wWord);
    fWord->addRow(QString::fromUtf8("Номер слова (word):"), spinWord);
    fWord->addRow(QString::fromUtf8("Переменная (var):"), editVarW);
    fWord->addRow(chkSignedW);
    fWord->addRow(QString::fromUtf8("Цена предстаршего разряда (cmr):"), spinCmrW);
    fWord->addRow(QString::fromUtf8("Масштаб (scale, необяз.):"), editScaleW);
    stack->addWidget(wWord);

    QWidget *wDword = new QWidget(&dialog);
    QFormLayout *fDword = new QFormLayout(wDword);
    QSpinBox *spinWordL = new QSpinBox(wDword); spinWordL->setRange(1, 32);
    QSpinBox *spinWordH = new QSpinBox(wDword); spinWordH->setRange(1, 32);
    QLineEdit *editVarD = new QLineEdit(wDword);
    QCheckBox *chkSignedD = new QCheckBox(QString::fromUtf8("Знаковое (signed)"), wDword);
    QDoubleSpinBox *spinCmrD = new QDoubleSpinBox(wDword); spinCmrD->setRange(-99999, 99999); spinCmrD->setDecimals(4);
    QLineEdit *editScaleD = new QLineEdit(wDword);
    fDword->addRow(QString::fromUtf8("Младшее слово (wordl):"), spinWordL);
    fDword->addRow(QString::fromUtf8("Старшее слово (wordh):"), spinWordH);
    fDword->addRow(QString::fromUtf8("Переменная (var):"), editVarD);
    fDword->addRow(chkSignedD);    fDword->addRow(QString::fromUtf8("Цена предстаршего разряда (cmr):"), spinCmrD);
    fDword->addRow(QString::fromUtf8("Масштаб (scale, необяз.):"), editScaleD);
    stack->addWidget(wDword);

    QWidget *wBits = new QWidget(&dialog);
    QFormLayout *fBits = new QFormLayout(wBits);
    QSpinBox *spinWordB = new QSpinBox(wBits); spinWordB->setRange(1, 32);
    QLineEdit *editVarB = new QLineEdit(wBits);
    QSpinBox *spinLength = new QSpinBox(wBits); spinLength->setRange(1, 16);
    QSpinBox *spinOffset = new QSpinBox(wBits); spinOffset->setRange(4, 19);
    QLineEdit *editValues = new QLineEdit(wBits);
    fBits->addRow(QString::fromUtf8("Номер слова (word):"), spinWordB);
    fBits->addRow(QString::fromUtf8("Переменная (var):"), editVarB);
    fBits->addRow(QString::fromUtf8("Длина в битах (length):"), spinLength);
    fBits->addRow(QString::fromUtf8("Смещение (offset, 4..19):"), spinOffset);
    fBits->addRow(QString::fromUtf8("Таблица значений (values, необяз.):"), editValues);
    stack->addWidget(wBits);

    QWidget *wConst = new QWidget(&dialog);
    QFormLayout *fConst = new QFormLayout(wConst);
    QSpinBox *spinWordC = new QSpinBox(wConst); spinWordC->setRange(1, 32);
    QLineEdit *editValueC = new QLineEdit(wConst);
    fConst->addRow(QString::fromUtf8("Номер слова (word):"), spinWordC);
    fConst->addRow(QString::fromUtf8("Значение (value, напр. 0 или 0xFF):"), editValueC);
    stack->addWidget(wConst);

    QWidget *wCounter = new QWidget(&dialog);
    QFormLayout *fCounter = new QFormLayout(wCounter);
    QSpinBox *spinWordCnt = new QSpinBox(wCounter); spinWordCnt->setRange(1, 32);
    fCounter->addRow(QString::fromUtf8("Номер слова (word):"), spinWordCnt);
    stack->addWidget(wCounter);

    QWidget *wChecksum = new QWidget(&dialog);
    QFormLayout *fChecksum = new QFormLayout(wChecksum);
    QSpinBox *spinWordCsum = new QSpinBox(wChecksum); spinWordCsum->setRange(1, 32);
    fChecksum->addRow(QString::fromUtf8("Номер слова (word):"), spinWordCsum);
    stack->addWidget(wChecksum);

    mainL->addLayout(formL);
    mainL->addWidget(stack);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    mainL->addWidget(btnBox);

    if (isEdit && row >= 0) {
        const MilDataField &f = m_config.arrays[m_currentArrayIndex].dataFields.at(row);
        nameEdit->setText(f.name);
        typeCombo->setCurrentText(f.type);

        if (f.type == "word") {
            spinWord->setValue(f.word);
            editVarW->setText(f.var);
            chkSignedW->setChecked(f.isSigned);
            spinCmrW->setValue(f.cmr);
            editScaleW->setText(f.scale);
        } else if (f.type == "dword") {
            spinWordL->setValue(f.wordl);
            spinWordH->setValue(f.wordh);
            editVarD->setText(f.var);
            chkSignedD->setChecked(f.isSigned);
            spinCmrD->setValue(f.cmr);
            editScaleD->setText(f.scale);
        } else if (f.type == "bits") {
            spinWordB->setValue(f.word);
            editVarB->setText(f.var);
            spinLength->setValue(f.length);
            spinOffset->setValue(f.offset);
            editValues->setText(f.values);
        } else if (f.type == "const") {
            spinWordC->setValue(f.word);
            editValueC->setText(f.value);
        } else if (f.type == "counter") {
            spinWordCnt->setValue(f.word);
        } else if (f.type == "checksum") {
            spinWordCsum->setValue(f.word);
        }
    }

    auto updateVisibility = [&]() {
        stack->setCurrentIndex(typeCombo->currentIndex());
    };
    connect(typeCombo, &QComboBox::currentTextChanged, updateVisibility);
    updateVisibility(); // Вызываем сразу, чтобы показать нужную страницу

    if (dialog.exec() == QDialog::Accepted) {
        MilDataField newField;
        newField.name = nameEdit->text().trimmed();
        newField.type = typeCombo->currentText();

        if (newField.name.isEmpty()) {
            QMessageBox::warning(this, QString::fromUtf8("Ошибка"), QString::fromUtf8("Имя параметра не может быть пустым."));            return;
        }

        int typeIdx = typeCombo->currentIndex();
        if (typeIdx == 0) { // word
            newField.word = spinWord->value();
            newField.var = editVarW->text().trimmed();
            newField.isSigned = chkSignedW->isChecked();
            newField.cmr = spinCmrW->value();
            newField.scale = editScaleW->text().trimmed();
        } else if (typeIdx == 1) { // dword
            newField.wordl = spinWordL->value();
            newField.wordh = spinWordH->value();
            newField.var = editVarD->text().trimmed();
            newField.isSigned = chkSignedD->isChecked();
            newField.cmr = spinCmrD->value();
            newField.scale = editScaleD->text().trimmed();
        } else if (typeIdx == 2) { // bits
            newField.word = spinWordB->value();
            newField.var = editVarB->text().trimmed();
            newField.length = spinLength->value();
            newField.offset = spinOffset->value();
            newField.values = editValues->text().trimmed();
        } else if (typeIdx == 3) { // const
            newField.word = spinWordC->value();
            newField.value = editValueC->text().trimmed();
        } else if (typeIdx == 4) { // counter
            newField.word = spinWordCnt->value();
        } else if (typeIdx == 5) { // checksum
            newField.word = spinWordCsum->value();
        }


        if (isEdit && row >= 0) {
            m_config.arrays[m_currentArrayIndex].dataFields[row] = newField;
        } else {
            m_config.arrays[m_currentArrayIndex].dataFields.append(newField);
        }
        updateArrayEditor();
    }
}

void MilEditorWidget::onAddField() { showFieldDialog(false, -1); }
void MilEditorWidget::onEditField() { int r = m_fieldsTable->currentRow(); if (r < 0) return; showFieldDialog(true, r); }
void MilEditorWidget::onDeleteField() {
    int r = m_fieldsTable->currentRow(); if (r < 0) return;
    if (QMessageBox::question(this, QString::fromUtf8("Удалить?"), QString::fromUtf8("Удалить поле?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_config.arrays[m_currentArrayIndex].dataFields.removeAt(r);
        updateArrayEditor();
    }
}
