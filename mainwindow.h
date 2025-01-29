#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtBluetooth/QBluetoothServiceDiscoveryAgent>

#include "scanner.h"
#include "sensor.h"
#include "sessionlogdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void onConnect();
    void onDisconnect();
    void onApplySettings();
    void onResetSettings();
    void onDownloadData();
    void onSelectDevice();
    void onSettingsEdited();

    void onOpenSessionLogs();
    void onCloseSessionLogs();

    void onSensorStateChanged(Sensor::State state);
    void onSensorError(Sensor::Error error);
    void onSensorConfigChanged(const SensorConfig& config);

    void onUpdateDeviceList(const QList<QBluetoothDeviceInfo>& devices);
    void onScannerStateChanged(Scanner::State state);

private:
    QWidget* createMeasurementSettingsItem(
        const QString& name,
        const QList<uint16_t>& sampleRates,
        uint16_t current,
        std::function<void(uint16_t)> onValueChanged);

    QWidget* createToggle(
        const QString& name,
        bool current,
        std::function<void(bool)> onValueChanged);

    QWidget* createDeviceSettingsItem();

    Ui::MainWindow *ui;
    Scanner scanner;
    QSharedPointer<Sensor> sensor;
    SensorConfig config;

    SessionLogDialog* sessionDialog;
};
#endif // MAINWINDOW_H
