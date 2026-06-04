#include "mainwindow.h"
#include "nmea_editor.h"
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_currentProtocol(Protocol::UNKNOWN)
{
    setupUi();
}

MainWindow::~MainWindow() {}

Protocol MainWindow::detectProtocol(const QJsonObject &root) {
    if (root.contains("mode") || root.contains("arrays")) return Protocol::MIL1553;
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key().trimmed().startsWith("PORT_")) return Protocol::NMEA;
    }
    if (root.contains("slave_mode")) return Protocol::NMEA;
    return Protocol::UNKNOWN;
}

void MainWindow::switchToProtocol(Protocol proto) {
    m_currentProtocol = proto;
    if (proto == Protocol::NMEA) {
        m_stackedWidget->setCurrentIndex(1);
        m_statusLabel->setText(QString::fromUtf8("Режим: NMEA | Файл: ") + (m_currentFilePath.isEmpty() ? QString::fromUtf8("Новый") : m_currentFilePath));
    } else if (proto == Protocol::MIL1553) {
        m_stackedWidget->setCurrentIndex(2);
        m_statusLabel->setText(QString::fromUtf8("Режим: MilStd1553 | Файл: ") + (m_currentFilePath.isEmpty() ? QString::fromUtf8("Новый") : m_currentFilePath));
    } else {
        m_stackedWidget->setCurrentIndex(0);
        m_statusLabel->setText(QString::fromUtf8("Готов к работе. Откройте или создайте файл конфигурации."));
    }
}

void MainWindow::setupUi() {
    setWindowTitle("Редактор конфигурации");
    resize(1100, 750);

    QMenu *fileMenu = menuBar()->addMenu(QString::fromUtf8("Файл"));
    fileMenu->addAction(QString::fromUtf8("Открыть конфигурацию..."), this, &MainWindow::onActionOpen);
    fileMenu->addAction(QString::fromUtf8("Сохранить конфигурацию..."), this, &MainWindow::onActionSave);
    fileMenu->addSeparator();
    fileMenu->addAction(QString::fromUtf8("Создать новый NMEA"), this, &MainWindow::onActionNewNmea);
    fileMenu->addAction(QString::fromUtf8("Создать новый MilStd1553"), this, &MainWindow::onActionNewMil);
    fileMenu->addSeparator();
    fileMenu->addAction(QString::fromUtf8("Выход"), this, &QWidget::close);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);
    QWidget *welcomePage = new QWidget(this);
    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomePage);
    welcomeLayout->addStretch();
    QLabel *welcomeLabel = new QLabel(QString::fromUtf8(
                                 "<p>Выберите <b>Файл -> Открыть конфигурацию...</b> для загрузки существующего файла,<br>"
                                 "или <b>Файл -> Создать новый...</b> для начала работы с нуля.</p>"), welcomePage);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(welcomeLabel);
    welcomeLayout->addStretch();
    m_stackedWidget->addWidget(welcomePage);

    m_nmeaEditor = new NmeaEditorWidget(this);
    m_stackedWidget->addWidget(m_nmeaEditor);

    m_milEditor = new MilEditorWidget(this);
    m_stackedWidget->addWidget(m_milEditor);

    m_statusLabel = new QLabel(QString::fromUtf8("Готов к работе."), this);
    statusBar()->addWidget(m_statusLabel);
}

void MainWindow::onActionOpen() {
    QString filePath = QFileDialog::getOpenFileName(this, QString::fromUtf8("Открыть конфигурацию"), "", QString::fromUtf8("JSON файлы (*.json);;Все файлы (*)"));
    if (filePath.isEmpty()) return;
    if (loadJson(filePath)) {
        m_currentFilePath = filePath;
        switchToProtocol(m_currentProtocol); // <--- ВОТ ЭТО ИСПРАВЛЯЕТ ПУСТОЕ ОКНО!
        QMessageBox::information(this, QString::fromUtf8("Успех"), QString::fromUtf8("Конфигурация успешно загружена!"));
    } else {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8("Не удалось прочитать файл или неизвестный формат."));
    }
}

void MainWindow::onActionSave() {
    if (m_currentProtocol == Protocol::UNKNOWN) {
        QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8("Нечего сохранять."));
        return;
    }
    QString filePath = QFileDialog::getSaveFileName(this, QString::fromUtf8("Сохранить конфигурацию"), m_currentFilePath, QString::fromUtf8("JSON файлы (*.json);;Все файлы (*)"));
    if (filePath.isEmpty()) return;
    if (saveJson(filePath)) {
        m_currentFilePath = filePath;
        switchToProtocol(m_currentProtocol);
        QMessageBox::information(this, QString::fromUtf8("Успех"), QString::fromUtf8("Конфигурация успешно сохранена!"));
    } else {
        QMessageBox::critical(this, QString::fromUtf8("Ошибка"), QString::fromUtf8("Не удалось сохранить файл."));
    }
}

void MainWindow::onActionNewNmea() {
    m_nmeaEditor->clear();
    m_currentFilePath = "";
    switchToProtocol(Protocol::NMEA);
}

void MainWindow::onActionNewMil() {
    m_milEditor->clear();
    m_currentFilePath = "";
    switchToProtocol(Protocol::MIL1553);
}

bool MainWindow::loadJson(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    m_currentProtocol = detectProtocol(root);
    if (m_currentProtocol == Protocol::UNKNOWN) return false;

    if (m_currentProtocol == Protocol::NMEA) {
        m_nmeaEditor->loadFromJson(root);
    } else if (m_currentProtocol == Protocol::MIL1553) {
        m_milEditor->loadFromJson(root);
    }
    return true;
}

bool MainWindow::saveJson(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out.setCodec("UTF-8");

    if (m_currentProtocol == Protocol::NMEA) {
        QJsonObject root;
        m_nmeaEditor->saveToJson(root);
        out << QJsonDocument(root).toJson(QJsonDocument::Indented);
    }
    else if (m_currentProtocol == Protocol::MIL1553) {
        out << m_milEditor->generateOrderedJson();
    }
    else {
        file.close();
        return false;
    }

    file.close();
    return true;
}
