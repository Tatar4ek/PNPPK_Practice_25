#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeWidget>
#include <QScrollArea>
#include <QFormLayout>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMenu>
#include <QAction>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openMilStd();
    void saveMilStd();
    void openSerial();
    void saveSerial();
    void onTreeClicked(QTreeWidgetItem *item, int column);

    // Слоты обновления данных (onStringChanged теперь без аргументов!)
    void onStringChanged();
    void onIntChanged(int val);
    void onDoubleChanged(double val);
    void onBoolChanged(bool val);

private:
    void setupUi();
    void initDict();

    QVariantMap trimMap(const QVariantMap &map);
    QVariant trimVariant(const QVariant &v);

    void loadJsonToMap(const QString &filePath, QVariantMap &targetMap, QTreeWidget *tree);
    void saveMapToJson(const QString &filePath, const QVariantMap &sourceMap);

    void buildTree(QTreeWidget *tree, const QVariantMap &map, QTreeWidgetItem *parentItem, const QStringList &parentPath);
    void buildFormForPath(const QStringList &path, QScrollArea *scrollArea);

    QVariant getValueByPath(const QVariantMap &root, const QStringList &path);
    void setValueByPath(QVariantMap &root, const QStringList &path, const QVariant &value);
    void setNestedValueRecursive(QVariantMap &map, const QStringList &path, int index, const QVariant &value);

    QWidget* createEditor(const QString &key, const QVariant &val, const QStringList &fullPath);

    QTabWidget *tabWidget;
    QTreeWidget *treeMil, *treeSer;
    QScrollArea *scrollMil, *scrollSer;

    QVariantMap rootMilMap;
    QVariantMap rootSerMap;
    QVariantMap *activeMap = nullptr;

    QStringList varDict;
};

#endif // MAINWINDOW_H
