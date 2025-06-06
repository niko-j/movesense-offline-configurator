#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "protocol/ProtocolConstants.hpp"

#include <QtLogging>
#include <QMessageBox>
#include <QLayout>
#include <QComboBox>
#include <QCheckBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , scanner(this)
    , ui(new Ui::MainWindow)
    , config({})
    , sessionDialog(new SessionLogDialog(this))
    , logStreamView(new LogStreamView(this))
{
    ui->setupUi(this);

    // Connect button actions
    connect(ui->startScanButton, &QPushButton::clicked, &scanner, &Scanner::start);
    connect(ui->stopScanButton, &QPushButton::clicked, &scanner, &Scanner::stop);

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnect);
    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::onApplySettings);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::onResetSettings);
    connect(ui->sessionLogsButton, &QPushButton::clicked, this, &MainWindow::onOpenSessionLogs);
    connect(ui->debugButton, &QPushButton::clicked, this, &MainWindow::onOpenDebugStream);

    connect(sessionDialog, &QDialog::finished, this, &MainWindow::onCloseSessionLogs);
    connect(logStreamView, &QDialog::finished, this, &MainWindow::onCloseDebugStream);

    // Connect device list actions
    connect(ui->deviceList, &QListWidget::itemSelectionChanged, this, &MainWindow::onSelectDevice);

    // Set widget states
    ui->stopScanButton->hide();
    ui->disconnectButton->hide();
    ui->connectButton->setEnabled(false);

    ui->resetButton->setEnabled(false);
    ui->applyButton->setEnabled(false);
    ui->sessionLogsButton->setEnabled(false);
    ui->debugButton->setEnabled(false);

    connect(&scanner, &Scanner::deviceListUpdated, this, &MainWindow::onUpdateDeviceList);
    connect(&scanner, &Scanner::stateChanged, this, &MainWindow::onScannerStateChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnect()
{
    scanner.stop();

    auto devices = scanner.listDevices();
    auto index = ui->deviceList->currentIndex();
    if(!index.isValid())
        return;
    auto& device = devices.at(index.row());

    ui->deviceList->setEnabled(false);
    ui->connectButton->hide();
    ui->disconnectButton->show();

    sensor = QSharedPointer<Sensor>(new Sensor(this, device));
    connect(sensor.get(), &Sensor::onStateChanged, this, &MainWindow::onSensorStateChanged);
    connect(sensor.get(), &Sensor::onError, this, &MainWindow::onSensorError);
    connect(sensor.get(), &Sensor::onConfigUpdated, this, &MainWindow::onSensorConfigChanged);
    connect(sensor.get(), &Sensor::onStatusResponse, this, &MainWindow::onSensorStatus);

    sensor->connectDevice();
}

void MainWindow::onDisconnect()
{
    sensor->disconnectDevice();
}

void MainWindow::onApplySettings()
{
    if(sensor)
    {
        sensor->sendConfig(config);
        ui->applyButton->setEnabled(false);
    }
}

void MainWindow::onResetSettings()
{
    if(sensor)
    {
        config = {};
        config.wakeUpBehavior = OfflineConfig::WakeUpConnector;
        config.sleepDelay = 30 * 60;
        onSensorConfigChanged(config);
        onApplySettings();
    }
}

void MainWindow::onSelectDevice()
{
    auto index = ui->deviceList->currentIndex();
    ui->connectButton->setEnabled(index.isValid());
}

void MainWindow::onSettingsEdited()
{
    ui->applyButton->setEnabled(true);
}

void MainWindow::onOpenSessionLogs()
{
    sessionDialog->show();
    sessionDialog->setSensorDevice(sensor);
}

void MainWindow::onCloseSessionLogs()
{
    sessionDialog->setSensorDevice(nullptr);
    sessionDialog->hide();
}

void MainWindow::onOpenDebugStream()
{
    logStreamView->show();
    logStreamView->setSensorDevice(sensor);
}

void MainWindow::onCloseDebugStream()
{
    logStreamView->setSensorDevice(nullptr);
    logStreamView->hide();
}

void MainWindow::onSensorStateChanged(Sensor::State state)
{
    switch(state)
    {
        case Sensor::Disconnected:
        {
            qInfo("Sensor disconnected!");
            onCloseSessionLogs();
            onCloseDebugStream();
            sensor.reset();

            ui->resetButton->setEnabled(false);
            ui->applyButton->setEnabled(false);
            ui->sessionLogsButton->setEnabled(false);
            ui->debugButton->setEnabled(false);

            ui->disconnectButton->hide();
            ui->connectButton->show();

            ui->deviceList->setEnabled(true);

            auto* settingsWidget = ui->settingsScrollArea->takeWidget();
            if(settingsWidget)
                delete settingsWidget;

            break;
        }
        case Sensor::Connecting:
        {
            qInfo("Sensor connecting...");
            break;
        }
        case Sensor::DiscoveringServices:
        {
            qInfo("Discovering sensor services...");
            break;
        }
        case Sensor::Connected:
        {
            qInfo("Sensor connected!");
            ui->sessionLogsButton->setEnabled(true);
            ui->debugButton->setEnabled(true);
            break;
        }
    }
}

void MainWindow::onSensorError(Sensor::Error error, QString msg)
{
    switch(error)
    {
    case Sensor::DeviceFault:
    {
        QString message = QString::asprintf("Sensor has encountered an error. Details:\n%s", msg.toStdString().c_str());
        QMessageBox::warning(this, "Sensor error", message);
        break;
    }
    default:
        QString message = QString::asprintf("Sensor reported an error: %u", error);
        QMessageBox::warning(this, "Sensor error", message);
        break;
    }
}

void MainWindow::onSensorConfigChanged(const OfflineConfig& config)
{
    this->config = config;

    ui->resetButton->setEnabled(true);

    QWidget* settings = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(settings);

    auto labelFormatOnOff = [](uint16_t value) {
        return value > 0 ? QString("On") : QString("Off");
    };

    auto labelFormatSampleRate = [](uint16_t value) {
        if(value == 0)
            return QString("Off");
        else
            return QString::asprintf("%d Hz", value);
    };

    auto labelFormatInterval = [](uint16_t value) {
        if (value == 0)
            return QString("Off");
        else
        {
            int h = value / (60 * 60);
            int m = (value - h * 3600) / 60;
            int s = value % 60;
            QString label = "";
            if(h > 0) label += QString::asprintf("%d h ", h);
            if(m > 0) label += QString::asprintf("%d min ", m);
            if(s > 0) label += QString::asprintf("%d s ", s);
            return label;
        }
    };

    QWidget* ecg = createDropmenu(
        "Single-lead ECG",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_SAMPLERATES_ECG),
        config.measurementParams.bySensor.ECG,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.ECG = val;
        }, labelFormatSampleRate);
    layout->addWidget(ecg);

    QWidget* ecgCompression = createToggle(
        "Use experimental ECG compression",
        !!(config.optionsFlags & OfflineConfig::OptionsCompressECG),
        [this](bool enable) {
            if(enable)
                this->config.optionsFlags |= OfflineConfig::OptionsCompressECG;
            else
                this->config.optionsFlags &= ~OfflineConfig::OptionsCompressECG;
        });
    layout->addWidget(ecgCompression);

    QWidget* hr = createDropmenu(
        "Heart rate (average bpm)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_TOGGLE),
        config.measurementParams.bySensor.HeartRate,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.HeartRate = val;
        }, labelFormatOnOff);
    layout->addWidget(hr);

    QWidget* rr = createDropmenu(
        "R-to-R intervals (ms)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_TOGGLE),
        config.measurementParams.bySensor.RtoR,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.RtoR = val;
        }, labelFormatOnOff);
    layout->addWidget(rr);

    QWidget* accel = createDropmenu(
        "Linear acceleration (m/s^2)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_SAMPLERATES_IMU),
        config.measurementParams.bySensor.Acc,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.Acc = val;
        }, labelFormatSampleRate);
    layout->addWidget(accel);

    QWidget* gyro = createDropmenu(
        "Gyroscope (dps)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_SAMPLERATES_IMU),
        config.measurementParams.bySensor.Gyro,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.Gyro = val;
        }, labelFormatSampleRate);
    layout->addWidget(gyro);

    QWidget* magn = createDropmenu(
        "Magnetometer (μT)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_SAMPLERATES_IMU),
        config.measurementParams.bySensor.Magn,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.Magn = val;
        }, labelFormatSampleRate);
    layout->addWidget(magn);

    QWidget* temp = createDropmenu(
        "Temperature (°C)",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_TOGGLE),
        config.measurementParams.bySensor.Temp,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.Temp = val;
        }, labelFormatOnOff);
    layout->addWidget(temp);

    QWidget* activity = createDropmenu(
        "Activity",
        QList<uint16_t>::fromReadOnlyData(SENSOR_MEAS_PRESETS_ACTIVITY_INTERVALS),
        config.measurementParams.bySensor.Activity,
        [this](uint16_t val) {
            this->config.measurementParams.bySensor.Activity = val;
        }, labelFormatInterval);
    layout->addWidget(activity);

    QWidget* tapDetection = createToggle(
        "Record tap detection events",
        !!(config.optionsFlags & OfflineConfig::OptionsLogTapGestures),
        [this](bool enable) {
            if(enable)
                this->config.optionsFlags |= OfflineConfig::OptionsLogTapGestures;
            else
                this->config.optionsFlags &= ~OfflineConfig::OptionsLogTapGestures;
        });
    layout->addWidget(tapDetection);

    QWidget* shakeDetection = createToggle(
        "Record shake detection events",
        !!(config.optionsFlags & OfflineConfig::OptionsLogShakeGestures),
        [this](bool enable) {
            if(enable)
                this->config.optionsFlags |= OfflineConfig::OptionsLogShakeGestures;
            else
                this->config.optionsFlags &= ~OfflineConfig::OptionsLogShakeGestures;
        });
    layout->addWidget(shakeDetection);

    QWidget* shakeToConnect = createToggle(
        "Shake to turn on BLE (turn off after 30 seconds)",
        !!(config.optionsFlags & OfflineConfig::OptionsShakeToConnect),
        [this](bool enable) {
            if(enable)
                this->config.optionsFlags |= OfflineConfig::OptionsShakeToConnect;
            else
                this->config.optionsFlags &= ~OfflineConfig::OptionsShakeToConnect;
        });
    layout->addWidget(shakeToConnect);

    QWidget* dev = createDeviceSettingsItem();
    layout->addWidget(dev);

    ui->settingsScrollArea->setWidget(settings);
}

void MainWindow::onSensorStatus(uint8_t ref, uint16_t status)
{
    if(status >= 300)
    {
        QString msg = QString::asprintf("Operation failed: %u", status);
        QMessageBox::warning(this, "Sensor error", msg);
    }
}

void MainWindow::onUpdateDeviceList(const QList<QBluetoothDeviceInfo>& devices)
{
    ui->deviceList->clear();
    for(const auto& dev : devices)
    {
        if(dev.name().isEmpty())
            ui->deviceList->addItem(dev.deviceUuid().toString());
        else
            ui->deviceList->addItem(dev.name());
    }
}

void MainWindow::onScannerStateChanged(Scanner::State state)
{
    switch(state)
    {
        case Scanner::Stopped:
        {
            ui->startScanButton->show();
            ui->stopScanButton->hide();
            break;
        }
        case Scanner::Scanning:
        {
            ui->startScanButton->hide();
            ui->stopScanButton->show();
            break;
        }
    }
}

QWidget* MainWindow::createDropmenu(
    const QString& name,
    const QList<uint16_t>& sampleRates,
    uint16_t current,
    std::function<void(uint16_t)> onValueChanged,
    std::function<QString(uint16_t)> labelFormatter)
{
    QWidget* item = new QWidget();
    QGridLayout* layout = new QGridLayout(item);
    {
        QLabel* label = new QLabel(name);
        layout->addWidget(label, 0, 0);

        QComboBox* dropdown = new QComboBox();
        int select = 0;
        for(auto i = 0; i < sampleRates.length(); i++)
        {
            uint16_t value = sampleRates[i];
            QString valueText = labelFormatter(value);
            dropdown->addItem(valueText, value);
            if(value == current)
                select = i;
        }
        dropdown->setCurrentIndex(select);

        connect(dropdown, &QComboBox::currentIndexChanged, this, [this, onValueChanged, dropdown](int index) {
            int value = dropdown->itemData(index).toInt();
            onValueChanged(value);
            this->onSettingsEdited();
        });

        layout->addWidget(dropdown, 0, 1);
    }
    return item;
}

QWidget* MainWindow::createToggle(
    const QString& name,
    bool current,
    std::function<void(bool)> onValueChanged)
{
    QWidget* item = new QWidget();
    QGridLayout* layout = new QGridLayout(item);
    {
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked(current);
        checkbox->setText(name);

        connect(checkbox, &QCheckBox::checkStateChanged, this, [this, onValueChanged, checkbox](Qt::CheckState state) {
            onValueChanged(state == Qt::Checked);
            this->onSettingsEdited();
        });

        layout->addWidget(checkbox, 0, 0);
    }
    return item;
}

QWidget* MainWindow::createDeviceSettingsItem()
{
    QWidget* item = new QWidget();
    QGridLayout* layout = new QGridLayout(item);
    {
        QLabel* wakeupLabel = new QLabel("Wake up device when");
        layout->addWidget(wakeupLabel, 0, 0);

        QList<QPair<QString, OfflineConfig::WakeUpBehavior>> wakeupOptionItems = {
            { "Always on", OfflineConfig::WakeUpAlwaysOn },
            { "Connectors", OfflineConfig::WakeUpConnector },
            { "Movement", OfflineConfig::WakeUpMovement },
            { "Double tap", OfflineConfig::WakeUpDoubleTap },
        };

        QList<QPair<QString, uint16_t>> sleepDelayItems = {
            { "Never (double tap to sleep)", 0 },
            { "30 seconds", 30 },
            { "1 minute", 60 },
            { "5 minutes", 5 * 60 },
            { "15 minutes", 15 * 60 },
            { "30 minutes", 30 * 60 },
            { "1 hour", 60 * 60 },
            { "2 hours", 2 * 60 * 60 },
            { "3 hours", 3 * 60 * 60 },
            { "6 hours", 6 * 60 * 60 },
            { "12 hours", 12 * 60 * 60 }, // Still fits unsigned 16-bit int
        };

        QComboBox* wakeupOptions = new QComboBox();
        {
            int i = 0;
            int select = 0;
            for(auto& item : wakeupOptionItems)
            {
                wakeupOptions->addItem(item.first, item.second);
                if(item.second == this->config.wakeUpBehavior)
                    select = i;
                i++;
            }
            wakeupOptions->setCurrentIndex(select);
        }
        layout->addWidget(wakeupOptions, 0, 1);

        QLabel* sleepDelayLabel = new QLabel("Automatic sleep after");
        layout->addWidget(sleepDelayLabel, 1, 0);

        QComboBox* sleepDelayOptions = new QComboBox();
        {
            int i = 0;
            int select = 0;
            for(auto& item : sleepDelayItems)
            {
                sleepDelayOptions->addItem(item.first, item.second);
                if(item.second == this->config.sleepDelay)
                    select = i;
                i++;
            }
            sleepDelayOptions->setCurrentIndex(select);
        }
        layout->addWidget(sleepDelayOptions, 1, 1);

        connect(wakeupOptions, &QComboBox::currentIndexChanged, this, [this, wakeupOptions](int index) {
            uint8_t value = wakeupOptions->itemData(index).toInt();
            this->config.wakeUpBehavior = (OfflineConfig::WakeUpBehavior) value;
            this->onSettingsEdited();
        });

        connect(sleepDelayOptions, &QComboBox::currentIndexChanged, this, [this, sleepDelayOptions](int index) {
            uint16_t value = sleepDelayOptions->itemData(index).toInt();
            this->config.sleepDelay = value;
            this->onSettingsEdited();
        });
    }
    return item;
}
