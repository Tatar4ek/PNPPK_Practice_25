#include "mainwindow.h"
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QHeaderView>
#include <QRegularExpression>
#include <QFont>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    initDict();
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::initDict()
{
    varDict << "tr_out_PB" << "tr_out_TB" << "tr_out_KB" << "tr_out_PTB" << "tr_out_TTB" << "tr_out_KTB"
            << "tr_out_VNP" << "tr_out_VNB" << "tr_out_SNB" << "tr_out_VEP" << "tr_out_VEB" << "tr_out_SEB"
            << "tr_out_VZB" << "tr_out_SZB" << "tr_out_ROB" << "tr_out_FIM" << "tr_out_LAM" << "tr_out_CRS"
            << "tr_out_VLM" << "tr_out_A_M" << "tr_out_B_M" << "tr_out_T_M" << "tr_out_M_K"
            << "out_PB" << "out_TB" << "out_KB" << "out_PTB" << "out_TTB" << "out_KTB"
            << "out_VNP" << "out_VNB" << "out_SNB" << "out_VEP" << "out_VEB" << "out_SEB"
            << "out_VZB" << "out_SZB" << "out_ROB" << "out_FIM" << "out_LAM" << "out_CRSM"
            << "out_VLM" << "out_A_M" << "out_B_M" << "out_T_M" << "out_M_K" << "out_VT" << "out_KT" << "out_VP"
            << "out_IN" << "out_IE" << "out_FIq" << "out_LAq" << "out_VNq" << "out_VEq" << "out_Kq" << "out_CRSMq" << "out_Q"
            << "level_valid" << "k_valid" << "crs_valid" << "coord_valid" << "vo_valid" << "va_valid"
            << "rot_valid" << "stab_valid" << "dk_valid" << "integr_valid"
            << "fig" << "pis" << "pfk" << "pvz" << "prgk" << "rx" << "ry" << "rz" << "alg" << "pa1" << "pff"
            << "pas" << "pvl" << "vlzd" << "pvt" << "vtzd" << "ktzd" << "pvp" << "dfp" << "dlp" << "prgv" << "pgsv"
            << "fivk" << "lavk" << "vgps" << "crs" << "td20" << "pga" << "pkvz" << "prl" << "prls" << "kl" << "vl0" << "tr" << "pr8"
            << "sns_la" << "sns_fi" << "sns_speed" << "sns_hdop" << "sns_crs" << "sns_vn" << "sns_ve"
            << "sns_date_y" << "sns_date_m" << "sns_date_d" << "sns_time" << "sns_valid" << "sns_mode"
            << "lag_along" << "lag_across" << "lag_valid" << "gal_along" << "gal_across" << "gal_valid"
            << "pnk" << "TLMT" << "pbin" << "prts" << "pfault" << "qCoordsOut" << "k_prec" << "time_setup" << "snum" << "DtmIn" << "DtmOut" << "pobs"
            << "kbm" << "scorr_counter" << "scorr_fi" << "scorr_la" << "m_a" << "kc_a" << "cur_bus";

    varDict.removeDuplicates();
    varDict.sort();
}

QVariant MainWindow::trimVariant(const QVariant &v) {
    if (v.type() == QVariant::String) return v.toString().trimmed();
    if (v.type() == QVariant::Map) return QVariant(trimMap(v.toMap()));
    if (v.type() == QVariant::List) {
        QVariantList list = v.toList();
        for(int i=0; i<list.size(); ++i) list[i] = trimVariant(list[i]);
        return list;
    }
    return v;
}

QVariantMap MainWindow::trimMap(const QVariantMap &map) {
    QVariantMap res;
    for (auto it = map.begin(); it != map.end(); ++it) {
        res[it.key().trimmed()] = trimVariant(it.value());
    }
    return res;
}

void MainWindow::setupUi()
{
    setWindowTitle("UniKama Protocol Configurator (Qt 5.12)");
    resize(1200, 800);

    QMenu *fileMenu = menuBar()->addMenu("Файл");
    fileMenu->addAction("Открыть MIL-STD...", this, &MainWindow::openMilStd);
    fileMenu->addAction("Сохранить MIL-STD...", this, &MainWindow::saveMilStd);
    fileMenu->addSeparator();
    fileMenu->addAction("Открыть Serial (NMEA)...", this, &MainWindow::openSerial);
    fileMenu->addAction("Сохранить Serial (NMEA)...", this, &MainWindow::saveSerial);

    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);

    // --- MIL-STD Tab ---
    QWidget *milWidget = new QWidget();
    QHBoxLayout *milLayout = new QHBoxLayout(milWidget);
    QSplitter *milSplitter = new QSplitter(Qt::Horizontal);
    treeMil = new QTreeWidget();
    treeMil->setHeaderLabel("MIL-STD-1553");
    treeMil->setMinimumWidth(300);
    scrollMil = new QScrollArea();
    scrollMil->setWidgetResizable(true);
    milSplitter->addWidget(treeMil);
    milSplitter->addWidget(scrollMil);
    milLayout->addWidget(milSplitter);
    tabWidget->addTab(milWidget, "MIL-STD-1553");
    connect(treeMil, &QTreeWidget::itemClicked, this, &MainWindow::onTreeClicked);

    // --- Serial Tab ---
    QWidget *serWidget = new QWidget();
    QHBoxLayout *serLayout = new QHBoxLayout(serWidget);
    QSplitter *serSplitter = new QSplitter(Qt::Horizontal);
    treeSer = new QTreeWidget();
    treeSer->setHeaderLabel("Serial (NMEA)");
    treeSer->setMinimumWidth(300);
    scrollSer = new QScrollArea();
    scrollSer->setWidgetResizable(true);
    serSplitter->addWidget(treeSer);
    serSplitter->addWidget(scrollSer);
    serLayout->addWidget(serSplitter);
    tabWidget->addTab(serWidget, "Serial (NMEA)");
    connect(treeSer, &QTreeWidget::itemClicked, this, &MainWindow::onTreeClicked);
}

QVariant MainWindow::getValueByPath(const QVariantMap &root, const QStringList &path) {
    QVariant current = root;
    for (const QString &key : path) {
        if (current.type() == QVariant::Map) {
            QVariantMap map = current.toMap();
            if (map.contains(key)) current = map.value(key);
            else return QVariant();
        } else return QVariant();
    }
    return current;
}

void MainWindow::setValueByPath(QVariantMap &root, const QStringList &path, const QVariant &value) {
    if (path.isEmpty()) return;
    setNestedValueRecursive(root, path, 0, value);
}

void MainWindow::setNestedValueRecursive(QVariantMap &map, const QStringList &path, int index, const QVariant &value) {
    if (index == path.size() - 1) {
        map[path.last()] = value;
        return;
    }
    QVariant &val = map[path[index]];
    if (val.type() != QVariant::Map) val = QVariantMap();

    QVariantMap subMap = val.toMap();
    setNestedValueRecursive(subMap, path, index + 1, value);
    val = subMap;
}

void MainWindow::loadJsonToMap(const QString &filePath, QVariantMap &targetMap, QTreeWidget *tree)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    QString text = file.readAll();
    // Удаляем висячие запятые перед } или ]
    text.replace(QRegularExpression(",(\\s*[}\\]])"), "\\1");

    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (doc.isObject()) {
        targetMap = trimMap(doc.object().toVariantMap());
        tree->clear();
        buildTree(tree, targetMap, nullptr, QStringList());
        if (tree->topLevelItemCount() > 0) tree->setCurrentItem(tree->topLevelItem(0));
    } else {
        QMessageBox::critical(this, "Ошибка", "Файл не является валидным JSON");
    }
}

void MainWindow::saveMapToJson(const QString &filePath, const QVariantMap &sourceMap)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj = QJsonObject::fromVariantMap(sourceMap);
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        QMessageBox::information(this, "Успех", "Конфигурация успешно сохранена!");
    }
}

void MainWindow::buildTree(QTreeWidget *tree, const QVariantMap &map, QTreeWidgetItem *parentItem, const QStringList &parentPath)
{
    for (auto it = map.begin(); it != map.end(); ++it) {
        QStringList currentPath = parentPath;
        currentPath.append(it.key());

        QTreeWidgetItem *item;
        if (parentItem) item = new QTreeWidgetItem(parentItem, {it.key()});
        else item = new QTreeWidgetItem(tree, {it.key()});

        // Используем прямой конструктор QVariant для QStringList
        item->setData(0, Qt::UserRole, QVariant(currentPath));

        if (it.value().type() == QVariant::Map) {
            buildTree(tree, it.value().toMap(), item, currentPath);
        }
        if (!parentItem) item->setExpanded(true);
    }
}

void MainWindow::onTreeClicked(QTreeWidgetItem *item, int)
{
    // Извлекаем путь через toStringList()
    QStringList path = item->data(0, Qt::UserRole).toStringList();
    if (path.isEmpty()) return;

    QTreeWidget *senderTree = qobject_cast<QTreeWidget*>(sender());
    QScrollArea *targetScroll = nullptr;

    if (senderTree == treeMil) {
        targetScroll = scrollMil; activeMap = &rootMilMap;
    } else {
        targetScroll = scrollSer; activeMap = &rootSerMap;
    }
    buildFormForPath(path, targetScroll);
}

void MainWindow::buildFormForPath(const QStringList &path, QScrollArea *scrollArea)
{
    QWidget *oldWidget = scrollArea->takeWidget();
    if (oldWidget) oldWidget->deleteLater();

    QWidget *container = new QWidget();
    QFormLayout *layout = new QFormLayout(container);
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    layout->setLabelAlignment(Qt::AlignRight);

    QVariant targetData = getValueByPath(*activeMap, path);

    if (targetData.type() == QVariant::Map) {
        QVariantMap map = targetData.toMap();
        for (auto it = map.begin(); it != map.end(); ++it) {
            QStringList fullPath = path;
            fullPath.append(it.key());

            QLabel *lbl = new QLabel("<b>" + it.key() + "</b>:");
            QWidget *editor = createEditor(it.key(), it.value(), fullPath);
            layout->addRow(lbl, editor);
        }
    } else {
        layout->addRow(new QLabel("Значение:"));
        layout->addRow(new QLabel(targetData.toString()));
    }
    scrollArea->setWidget(container);
}

QWidget* MainWindow::createEditor(const QString &key, const QVariant &val, const QStringList &fullPath)
{
    auto setupWidget = [&](QWidget *w) {
        w->setProperty("path", QVariant(fullPath));
        return w;
    };

    if (key == "type") {
        QComboBox *cmb = new QComboBox();
        cmb->addItems({"word", "dword", "bits", "const", "counter", "checksum"});
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }
    if (key == "var") {
        QComboBox *cmb = new QComboBox();
        cmb->setEditable(true);
        cmb->addItems(varDict);
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }
    if (key == "scale") {
        QComboBox *cmb = new QComboBox();
        cmb->setEditable(true);
        cmb->addItems({"", "RAD_TO_DEG", "MS_KN"});
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }
    if (key == "direction") {
        QComboBox *cmb = new QComboBox();
        cmb->addItems({"send", "receive"});
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }
    if (key == "mode") {
        QComboBox *cmb = new QComboBox();
        cmb->addItems({"BC", "RT"});
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }
    if (key == "bus") {
        QComboBox *cmb = new QComboBox();
        cmb->addItems({"A", "B", "AB"});
        cmb->setCurrentText(val.toString());
        connect(cmb, &QComboBox::currentTextChanged, this, &MainWindow::onStringChanged);
        return setupWidget(cmb);
    }

    if (key == "MESSAGE") {
        QTextEdit *te = new QTextEdit();
        te->setFont(QFont("Consolas", 10));
        te->setPlainText(val.toString());
        te->setMinimumHeight(150);
        te->setWordWrapMode(QTextOption::NoWrap);
        // textChanged не передает аргументов, слот onStringChanged сам возьмет текст
        connect(te, &QTextEdit::textChanged, this, &MainWindow::onStringChanged);
        return setupWidget(te);
    }

    if (val.type() == QVariant::Bool) {
        QCheckBox *cb = new QCheckBox();
        cb->setChecked(val.toBool());
        connect(cb, &QCheckBox::toggled, this, &MainWindow::onBoolChanged);
        return setupWidget(cb);
    }
    if (val.type() == QVariant::Double) {
        QDoubleSpinBox *sb = new QDoubleSpinBox();
        sb->setDecimals(6);
        sb->setRange(-99999999, 99999999);
        sb->setValue(val.toDouble());
        connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onDoubleChanged);
        return setupWidget(sb);
    }
    if (val.type() == QVariant::Int || val.type() == QVariant::LongLong) {
        QSpinBox *sb = new QSpinBox();
        sb->setRange(-99999999, 99999999);
        sb->setValue(val.toInt());
        connect(sb, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onIntChanged);
        return setupWidget(sb);
    }
    if (val.type() == QVariant::List) {
        QLineEdit *le = new QLineEdit();
        le->setText(QJsonDocument::fromVariant(val).toJson(QJsonDocument::Compact));
        le->setToolTip("Формат JSON массива, например: [1, 2]");
        // editingFinished не передает аргументов
        connect(le, &QLineEdit::editingFinished, this, &MainWindow::onStringChanged);
        return setupWidget(le);
    }

    QLineEdit *le = new QLineEdit();
    le->setText(val.toString());
    connect(le, &QLineEdit::editingFinished, this, &MainWindow::onStringChanged);
    return setupWidget(le);
}

// ==================== УНИВЕРСАЛЬНЫЕ СЛОТЫ ====================
void MainWindow::onStringChanged()
{
    QWidget *w = qobject_cast<QWidget*>(sender());
    if (!w || !activeMap) return;

    QStringList path = w->property("path").toStringList();
    if (path.isEmpty()) return;

    // Определяем текст в зависимости от типа виджета
    QString text;
    if (QLineEdit *le = qobject_cast<QLineEdit*>(w)) text = le->text();
    else if (QTextEdit *te = qobject_cast<QTextEdit*>(w)) text = te->toPlainText();
    else if (QComboBox *cmb = qobject_cast<QComboBox*>(w)) text = cmb->currentText();
    else return;

    QVariant oldVal = getValueByPath(*activeMap, path);
    if (oldVal.type() == QVariant::List) {
        QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
        if (doc.isArray()) setValueByPath(*activeMap, path, doc.array().toVariantList());
    } else {
        setValueByPath(*activeMap, path, text);
    }
}

void MainWindow::onIntChanged(int val) {
    QWidget *w = qobject_cast<QWidget*>(sender());
    if (w && activeMap) setValueByPath(*activeMap, w->property("path").toStringList(), val);
}

void MainWindow::onDoubleChanged(double val) {
    QWidget *w = qobject_cast<QWidget*>(sender());
    if (w && activeMap) setValueByPath(*activeMap, w->property("path").toStringList(), val);
}

void MainWindow::onBoolChanged(bool val) {
    QWidget *w = qobject_cast<QWidget*>(sender());
    if (w && activeMap) setValueByPath(*activeMap, w->property("path").toStringList(), val);
}

void MainWindow::openMilStd() {
    QString f = QFileDialog::getOpenFileName(this, "Open MIL-STD", "", "JSON (*.json)");
    if(!f.isEmpty()) loadJsonToMap(f, rootMilMap, treeMil);
}
void MainWindow::saveMilStd() {
    QString f = QFileDialog::getSaveFileName(this, "Save MIL-STD", "", "JSON (*.json)");
    if(!f.isEmpty()) saveMapToJson(f, rootMilMap);
}
void MainWindow::openSerial() {
    QString f = QFileDialog::getOpenFileName(this, "Open Serial", "", "JSON (*.json)");
    if(!f.isEmpty()) loadJsonToMap(f, rootSerMap, treeSer);
}
void MainWindow::saveSerial() {
    QString f = QFileDialog::getSaveFileName(this, "Save Serial", "", "JSON (*.json)");
    if(!f.isEmpty()) saveMapToJson(f, rootSerMap);
}
