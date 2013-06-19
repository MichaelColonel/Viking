#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QByteArray>
#include <QSocketNotifier>
#include <QSettings>

#include <iostream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "signals.h"
#include "signal_parameters.h"

#define NOF_TIMINGS  8
#define BUFFER_SIZE 72
#define BUFFER_DATA 64
#define HEADER_SIZE  8

namespace {

const char* const labels[] = {
    QT_TRANSLATE_NOOP( "viking", "sync_ext"),
    QT_TRANSLATE_NOOP( "viking", "sync_test"),
    QT_TRANSLATE_NOOP( "viking", "holdb / dummy holdb"),
    QT_TRANSLATE_NOOP( "viking", "dreset / dummy dreset"),
    QT_TRANSLATE_NOOP( "viking", "shift_in_b"),
    QT_TRANSLATE_NOOP( "viking", "CKb / dummy CKb"),
    QT_TRANSLATE_NOOP( "viking", "TEST_on"),
    QT_TRANSLATE_NOOP( "viking", "ADC"),
    0
};

struct TimingParams signal_params[] = {
    { PULSE, new Pulse( false, 100, 150),        0, labels[0] }, // 2
    { PULSE, new Pulse( false, 300, 200),        3, labels[1] }, // 3
    { PULSE, new Pulse( true, 400, 5300),        6, labels[2] }, // 4-1 / 4-2
    { PULSE, new Pulse( false, 600, 150),        5, labels[3] }, // 5-1 / 5-2
    { PULSE, new Pulse( true, 800, 50),          4, labels[4] }, // 6
    { CLOCK, new Clock( true, 830, 5, 25, 129),  7, labels[5] }, // 7-1 / 7-2
    { LEVEL, new Level(true),                    1, labels[6] }, // 8
    { CLOCK, new Clock( false, 830, 23, 7, 129), 2, labels[7] }, // 9
    { NONE,  0,                                 -1, labels[8] }, // terminating value
};

char buffer[BUFFER_SIZE] = {};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    pTimingParams(signal_params),
    pFdNotifier(0),
    currentChart(-1),
    bIgnore(false),
#ifdef Q_OS_LINUX
    fd(-1),
#endif
    ratio(1.0),
    test(false)
{
    ui->setupUi(this);
    connect( ui->timingListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(signalCurrentRowChanged(int)));
    connect( ui->timingListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(signalCurrentItemChanged(QListWidgetItem*)));

    connect( ui->invertedCheckBox, SIGNAL(clicked()), this, SLOT(signalLevelChanged()));
    connect( ui->offsetPulseSpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalPulseOffsetChanged(int)));
    connect( ui->lengthPulseSpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalPulseLengthChanged(int)));
    connect( ui->offsetClockSpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalClockOffsetChanged(int)));
    connect( ui->timesClockSpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalClockTimesChanged(int)));
    connect( ui->addressSpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalAddressChanged(int)));

    connect( ui->connectPushButton, SIGNAL(clicked()), this, SLOT(signalConnectButtonClicked()));
    connect( ui->disconnectPushButton, SIGNAL(clicked()), this, SLOT(signalDisconnectButtonClicked()));

    connect( ui->resetPushButton, SIGNAL(clicked()), this, SLOT(signalResetButtonClicked()));
    connect( ui->startPushButton, SIGNAL(clicked()), this, SLOT(signalStartButtonClicked()));
    connect( ui->extSyncRadioButton, SIGNAL(toggled(bool)), this, SLOT(signalSyncRadioButtonToggled(bool)));
    connect( ui->intSyncRadioButton, SIGNAL(toggled(bool)), this, SLOT(signalSyncRadioButtonToggled(bool)));

    connect( ui->writePushButton, SIGNAL(clicked()), this, SLOT(signalWriteButtonClicked()));
    connect( ui->totalLenghtComboBox, SIGNAL(activated(QString)), this, SLOT(signalTotalLenghtActivated(QString)));

    connect( ui->openPushButton, SIGNAL(clicked()), this, SLOT(signalOpenSettingsClicked()));
    connect( ui->savePushButton, SIGNAL(clicked()), this, SLOT(signalSaveSettingsClicked()));
    connect( ui->writeLengthPushButton, SIGNAL(clicked()), this, SLOT(signalWriteLengthButtonClicked()));

    connect( ui->t2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT2Changed(int)));
    connect( ui->t3SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT3Changed(int)));
    connect( ui->t4SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT4Changed(int)));
    connect( ui->t5SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT5Changed(int)));
    connect( ui->t6SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT6Changed(int)));
    connect( ui->t7SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT7Changed(int)));
    connect( ui->t8SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT8Changed(int)));
    connect( ui->t9SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT9Changed(int)));
    connect( ui->t10SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT10Changed(int)));
    connect( ui->t11SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT11Changed(int)));
    connect( ui->t12SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT12Changed(int)));
    connect( ui->t13SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT13Changed(int)));
    connect( ui->t14SpinBox, SIGNAL(valueChanged(int)), this, SLOT(signalT14Changed(int)));

    loadSettings();
    formAll();

    // Here comes a lot of noob code and other elements
    mTnSpinBoxes.push_back(0);
    mTnSpinBoxes.push_back(0);
    mTnSpinBoxes.push_back(ui->t2SpinBox);
    mTnSpinBoxes.push_back(ui->t3SpinBox);
    mTnSpinBoxes.push_back(ui->t4SpinBox);
    mTnSpinBoxes.push_back(ui->t5SpinBox);
    mTnSpinBoxes.push_back(ui->t6SpinBox);
    mTnSpinBoxes.push_back(ui->t7SpinBox);
    mTnSpinBoxes.push_back(ui->t8SpinBox);
    mTnSpinBoxes.push_back(ui->t9SpinBox);
    mTnSpinBoxes.push_back(ui->t10SpinBox);
    mTnSpinBoxes.push_back(ui->t11SpinBox);
    mTnSpinBoxes.push_back(ui->t12SpinBox);
    mTnSpinBoxes.push_back(ui->t13SpinBox);
    mTnSpinBoxes.push_back(ui->t14SpinBox);

    for ( int i = 2; i < 15; ++i) {
        setTn(i);
    }
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void
MainWindow::loadSettings(const QString& filename)
{
    QSettings::Format format = QSettings::NativeFormat;
#ifdef Q_OS_WIN32
    format = QSettings::IniFormat;
#endif

    QSettings* settings = (filename.isEmpty())
            ? new QSettings( "Viking", "configure")
            : new QSettings( filename, format);

    if (!settings->contains("total-length")) {
        delete settings;
        return;
    }
    else
        Signal::total_length = settings->value( "total-length", Signal::total_length).toInt();

    int i = 0;
    while (pTimingParams[i].signal) {
        QString timing = QString("Timing%1").arg(i);
        settings->beginGroup(timing);
        pTimingParams[i].address = settings->value( "bit-address", 0).toInt();

        Signal* signal = 0;
        switch (pTimingParams[i].type) {
        case LEVEL:
            signal = dynamic_cast<Level*>(pTimingParams[i].signal);
            break;
        case PULSE:
            signal = dynamic_cast<Pulse*>(pTimingParams[i].signal);
            break;
        case CLOCK:
            signal = dynamic_cast<Clock*>(pTimingParams[i].signal);
            break;
        case NONE:
        default:
            break;
        }
        if (signal)
            signal->loadSettings(settings);

        settings->endGroup();
        ++i;
    }
    delete settings;

    int index = ui->totalLenghtComboBox->findText(QString::number(int(Signal::total_length)));
    if (index != -1) {
        ui->totalLenghtComboBox->setCurrentIndex(index);
        ui->plotterWidget->setLength(Signal::total_length);
    }
    ui->lengthPulseSpinBox->setMaximum(Signal::total_length);
    ui->offsetPulseSpinBox->setMaximum(Signal::total_length);
    ui->offsetClockSpinBox->setMaximum(Signal::total_length);
}

void
MainWindow::saveSettings(const QString& filename)
{
    QSettings::Format format = QSettings::NativeFormat;
#ifdef Q_OS_WIN32
    format = QSettings::IniFormat;
#endif

    QSettings* settings = (filename.isEmpty())
            ? new QSettings( "Viking", "configure")
            : new QSettings( filename, format);

    settings->setValue( "total-length", Signal::total_length);

    int i = 0;
    while (pTimingParams[i].signal) {
        QString timing = QString("Timing%1").arg(i);
        settings->beginGroup(timing);
        settings->setValue( "bit-address", pTimingParams[i].address);

        Signal* signal = 0;
        Pulse* pulse = 0;
        switch (pTimingParams[i].type) {
        case LEVEL:
            signal = dynamic_cast<Level*>(pTimingParams[i].signal);
            break;
        case PULSE:
            signal = pulse = dynamic_cast<Pulse*>(pTimingParams[i].signal);
            break;
        case CLOCK:
            signal = pulse = dynamic_cast<Clock*>(pTimingParams[i].signal);
            break;
        case NONE:
        default:
            break;
        }

        if (signal)
            signal->saveSettings(settings);

        settings->endGroup();
        ++i;
    }
    delete settings;
}

void
MainWindow::formAll()
{
    try {
        initTimingMap();
        form();
    }
    catch (std::overflow_error) {
        QMessageBox msgBox(this);
        msgBox.setModal(true);
        msgBox.setWindowTitle(tr("Viking"));
        msgBox.setText(tr("Wrong parameter value or range."));
        msgBox.exec();
    }
    catch (std::runtime_error) {
    }
    catch (...) {
    }
}

void
MainWindow::initTimingMap() throw(std::range_error)
{
    std::vector<bool> vec( Level::total_length, 0);

    for ( int i = 0; i < NOF_TIMINGS; ++i) {
        Signal* signal = 0;
        switch (pTimingParams[i].type) {
        case LEVEL:
            signal = dynamic_cast<Level*>(pTimingParams[i].signal);
            break;
        case PULSE:
            signal = dynamic_cast<Pulse*>(pTimingParams[i].signal);
            break;
        case CLOCK:
            signal = dynamic_cast<Clock*>(pTimingParams[i].signal);
            break;
        case NONE:
        default:
            break;
        }
        if (signal) {
            try {
                signal->form(vec);
            }
            catch (const std::overflow_error& err) {
                std::cerr << err.what() << std::endl;
                throw;
            }

            mTimingMap[NOF_TIMINGS - i - 1] = vec;
        }
        std::fill( vec.begin(), vec.end(), 0);
    }
}

void
MainWindow::form()
{
    if (bIgnore)
        return;

    if (test)
        return;
    for ( TimingMap::const_iterator iter = mTimingMap.begin(); iter != mTimingMap.end(); ++iter) {
        QVector<QPointF> vec(Signal::total_length);

        for ( unsigned int j = 0; j < Signal::total_length; ++j) {
            vec[j] = QPointF( j, 2 * iter->first + iter->second[j]);
        }
        ui->plotterWidget->setCurveData( iter->first, vec);
    }
}

void
MainWindow::signalCurrentItemChanged(QListWidgetItem* item)
{
    int row = ui->timingListWidget->row(item);

    ui->statusBar->showMessage(qApp->translate( "viking", pTimingParams[row].label));

    switch (pTimingParams[row].type) {
    case LEVEL:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(false);
        ui->clockGroupBox->setEnabled(false);
        break;
    case PULSE:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(true);
        ui->clockGroupBox->setEnabled(false);
        break;
    case CLOCK:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(true);
        ui->clockGroupBox->setEnabled(true);
        break;
    default:
        break;
    }
}

void
MainWindow::signalCurrentRowChanged(int i)
{
    currentChart = i;

    test = true;
    ui->addressSpinBox->setValue(pTimingParams[i].address);

    switch (pTimingParams[i].type) {
    case LEVEL:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(false);
        ui->clockGroupBox->setEnabled(false);
        {
            Level* level = dynamic_cast<Level*>(pTimingParams[i].signal);
            setLevelParams(level);
        }
        break;
    case PULSE:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(true);
        ui->clockGroupBox->setEnabled(false);
        {
            Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[i].signal);
            setLevelParams(pulse);
            setPulseParams(pulse);
        }
        break;
    case CLOCK:
        ui->levelGroupBox->setEnabled(true);
        ui->pulseGroupBox->setEnabled(true);
        ui->clockGroupBox->setEnabled(true);
        {
            Clock* clock = dynamic_cast<Clock*>(pTimingParams[i].signal);
            setLevelParams(clock);
            setPulseParams(&clock->pulse);
            setClockParams(clock);
        }
        break;
    default:
        break;
    }
      test = false;
}

void
MainWindow::signalLevelChanged()
{
    if (currentChart != -1) {
        Level* level = dynamic_cast<Level*>(pTimingParams[currentChart].signal);
        level->inverted = (ui->invertedCheckBox->checkState() == Qt::Checked) ? true : false;
        Signal::inverse(mTimingMap.at(NOF_TIMINGS - currentChart - 1));
        form();
    }
}

void
MainWindow::signalAddressChanged(int bit)
{
    if (currentChart != -1)
        pTimingParams[currentChart].address = bit;
}

void
MainWindow::signalPulseOffsetChanged(int value)
{
    if (currentChart == -1)
        return;
    switch (pTimingParams[currentChart].type) {
    case PULSE:
        {
            Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[currentChart].signal);
            pulse->offset = value;
        }
        break;
    case CLOCK:
        {
            Clock* clock = dynamic_cast<Clock*>(pTimingParams[currentChart].signal);
            clock->pulse.offset = value;
        }
        break;
    default:
        return;
    }

    for ( int i = 2; i < 15; ++i) {
        setTn(i);
    }
    changeSignalsOffsets();
    formAll();
}

void
MainWindow::signalPulseLengthChanged(int value)
{
    if (currentChart == -1)
        return;
    switch (pTimingParams[currentChart].type) {
    case PULSE:
        {
            Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[currentChart].signal);
            pulse->length = value;
        }
        break;
    case CLOCK:
        {
            Clock* clock = dynamic_cast<Clock*>(pTimingParams[currentChart].signal);
            clock->pulse.length = value;
        }
        break;
    default:
        return;
    }

    for ( int i = 2; i < 15; ++i) {
        setTn(i);
    }
    changeSignalsOffsets();
    formAll();
}

void
MainWindow::signalClockOffsetChanged(int value)
{
    if (currentChart == -1)
        return;

    Clock* clock = dynamic_cast<Clock*>(pTimingParams[currentChart].signal);
    clock->offset = value;

    for ( unsigned int i = 2; i < 15; ++i) {
        setTn(i);
    }
    changeSignalsOffsets();
    formAll();
}

void
MainWindow::signalClockTimesChanged(int value)
{
    if (currentChart == -1)
        return;

    Clock* clock = dynamic_cast<Clock*>(pTimingParams[currentChart].signal);
    clock->times = value;

    for ( unsigned int i = 2; i < 15; ++i) {
        setTn(i);
    }
    changeSignalsOffsets();
    formAll();
}

void
MainWindow::setLevelParams(Level* level)
{
    bIgnore = true;

    if (level->inverted)
        ui->invertedCheckBox->setCheckState(Qt::Checked);
    else
        ui->invertedCheckBox->setCheckState(Qt::Unchecked);

    bIgnore = false;

}

void
MainWindow::setPulseParams(Pulse* pulse)
{
    bIgnore = true;

    ui->lengthPulseSpinBox->setValue(pulse->length);
    ui->offsetPulseSpinBox->setValue(pulse->offset);

    bIgnore = false;
}

void
MainWindow::setClockParams(Clock* clock)
{
    bIgnore = true;

    ui->timesClockSpinBox->setValue(clock->times);
    ui->offsetClockSpinBox->setValue(clock->offset);

    bIgnore = false;
}

unsigned char*
MainWindow::formByteArray()
{
    unsigned char* buffer = new unsigned char[Signal::total_length];
    std::fill( buffer, buffer + Signal::total_length, 0);

    for ( unsigned int i = 0; i < Signal::total_length; ++i) {
        unsigned int j = 0;
        while (pTimingParams[j].signal) {
            buffer[i] |= (mTimingMap[NOF_TIMINGS - j - 1][i] << pTimingParams[j].address);
            ++j;
        }
    }

    return buffer;
}

void
MainWindow::signalConnectButtonClicked()
{
#ifdef Q_OS_LINUX
    fd = ::open( "/dev/viking", O_RDWR);
#elif defined(Q_OS_WIN32)
    FT_STATUS status;
    int iport = 0;
#endif

#ifdef Q_OS_LINUX
    if (fd != -1) {
        pFdNotifier = new QSocketNotifier( fd, QSocketNotifier::Exception, this);
        connect( pFdNotifier, SIGNAL(activated(int)), this, SLOT(signalNotifierActivated(int)));
#elif defined(Q_OS_WIN32)
    if ((status = FT_Open( iport, &mHandle)) == FT_OK) {
#endif
        ui->connectPushButton->setEnabled(false);
        ui->disconnectPushButton->setEnabled(true);
        ui->writePushButton->setEnabled(true);
        ui->extSyncRadioButton->setEnabled(true);
        ui->intSyncRadioButton->setEnabled(true);
        ui->resetPushButton->setEnabled(true);
        ui->startPushButton->setEnabled(true);
        ui->writeLengthPushButton->setEnabled(true);
        ui->statusBar->showMessage(tr("Device successfully connected"));
    }
    else {
        ui->connectPushButton->setEnabled(true);
        ui->disconnectPushButton->setEnabled(false);
        ui->writePushButton->setEnabled(false);
        ui->extSyncRadioButton->setEnabled(false);
        ui->intSyncRadioButton->setEnabled(false);
        ui->resetPushButton->setEnabled(false);
        ui->startPushButton->setEnabled(false);
        ui->writeLengthPushButton->setEnabled(false);
        ui->statusBar->showMessage( tr("Can't connect device"), 3000);
    }
}

void
MainWindow::signalTotalLenghtActivated(const QString& str)
{
    bool ok  = false;
    int value = str.toInt(&ok);
    if (ok) {
        ratio = double(Signal::total_length) / double(value);
        Signal::total_length = value;
        Signal::ratio = ratio;

        ui->plotterWidget->setLength(value);

        int i = 0;
        while (pTimingParams[i].signal) {
            Signal* signal = 0;
            switch (pTimingParams[i].type) {
            case PULSE:
                signal = dynamic_cast<Pulse*>(pTimingParams[i].signal);
                break;
            case CLOCK:
                signal = dynamic_cast<Clock*>(pTimingParams[i].signal);
                break;
            case LEVEL:
            case NONE:
            default:
                break;
            }
            if (signal)
                signal->changeRatio();
            ++i;
        }

        for ( int i = 2; i < 15; ++i) {
            setTn(i);
        }
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalWriteLengthButtonClicked()
{
    QString str = ui->totalLenghtComboBox->currentText();
    bool ok  = false;
    int value = str.toInt(&ok);
    if (ok) {
        value--;
        char buf[3] = { 'L' };
        buf[1] = static_cast<char>((value >> 8) & 0xFF);
        buf[2] = static_cast<char>(value & 0xFF);
        write(buf);
    }
}

void
MainWindow::signalDisconnectButtonClicked()
{
#ifdef Q_OS_WIN32
    FT_Close(mHandle);
#elif defined(Q_OS_LINUX)
    ::close(fd);
    fd = -1;
    delete pFdNotifier;
#endif

    ui->statusBar->showMessage(tr("Device successfully disconnected"));
    ui->connectPushButton->setEnabled(true);
    ui->disconnectPushButton->setEnabled(false);
    ui->writePushButton->setEnabled(false);
    ui->extSyncRadioButton->setEnabled(false);
    ui->intSyncRadioButton->setEnabled(false);
    ui->resetPushButton->setEnabled(false);
    ui->startPushButton->setEnabled(false);
    ui->writeLengthPushButton->setEnabled(false);
}

void
MainWindow::signalResetButtonClicked()
{
    write("R");
}

void
MainWindow::signalStartButtonClicked()
{
    write("S");
}

void
MainWindow::signalSyncRadioButtonToggled(bool checked)
{
    if (checked) {
        const char* sync = ui->intSyncRadioButton->isChecked() ? "C" : "c";
        write(sync);
    }
}

void
MainWindow::signalOpenSettingsClicked()
{
    QString filename = QFileDialog::getOpenFileName( this,
                                                     tr("Open Settings"),
                                                     QString(),
                                                     tr("Settings Files (*.conf)"));
    if (!filename.isEmpty()) {
        loadSettings(filename);

        for ( unsigned int i = 2; i < 15; ++i) {
            setTn(i);
        }
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalSaveSettingsClicked()
{
    QString filename = QFileDialog::getSaveFileName( this,
                                                     tr("Save Settings"),
                                                     QString(),
                                                     tr("Settings Files (*.conf)"));

    QString binary_filename = filename;
    QFileInfo fi(filename);
    if (fi.completeSuffix() != QString("conf")) {
        filename += QString(".conf");
        binary_filename += QString(".bin");
    }
    else {
        QDir dir = QFileInfo(binary_filename).dir();
        binary_filename = QFileInfo(binary_filename).completeBaseName();
        binary_filename += QString(".bin");
        binary_filename = dir.absolutePath() + QDir::separator() + binary_filename;
    }

    if (!filename.isEmpty()) {

        std::ofstream file(binary_filename.toStdString().c_str());
        unsigned char* buf = formByteArray();
        file.write( (char *)buf, Signal::total_length);
        delete [] buf;
        file.close();

        saveSettings(filename);

    }
}

void
MainWindow::signalWriteButtonClicked()
{
    unsigned char* buf = formByteArray();

#ifdef Q_OS_LINUX
    QFile file;
    qint64 result = -1;
#endif

    int prog = (Signal::total_length / BUFFER_DATA);
    QProgressDialog progress( tr("Writing protocol..."), tr("Abort"), 0, prog, this);
    progress.setWindowTitle(tr("Viking"));
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
#ifdef Q_OS_LINUX
    if (file.open( fd, QIODevice::WriteOnly)) {
#endif
        for ( int i = 0; i < prog; ++i) {
            progress.setValue(i);
            if (progress.wasCanceled()) {
                write("R");
                break;
            }

            ::memset( buffer, 0, BUFFER_SIZE);
            buffer[0] = 'D';
            ::memcpy( buffer + HEADER_SIZE, buf + i * BUFFER_DATA, BUFFER_DATA);
#ifdef Q_OS_LINUX
            result = file.write( buffer, BUFFER_SIZE);
            if (result != BUFFER_SIZE) {
                std::cerr << "Write error: " << result << std::endl;
            }
            file.flush();
#elif defined(Q_OS_WIN32)
            DWORD written;
            FT_STATUS status = FT_Write( mHandle, buffer, BUFFER_SIZE, &written);
            if (status != FT_OK) {
                std::cerr << "Write error." << std::endl;
            }
            if (written != static_cast<DWORD>(BUFFER_SIZE)) {
                std::cerr << "Write not full." << std::endl;
            }
#endif
            ::usleep(20000);
        }
#ifdef Q_OS_LINUX
    }
    file.close();
#endif
    progress.setValue(prog);

    delete [] buf;
}

void
MainWindow::write(const char* command)
{
    if (command) {
#ifdef Q_OS_LINUX
        QFile file;
        qint64 result = -1;
#endif
        ::memset( buffer, 0, BUFFER_SIZE);
        ::memcpy( buffer, command, qstrlen(command));
        std::cout << buffer << std::endl;

#ifdef Q_OS_LINUX
        if (file.open( fd, QIODevice::ReadWrite)) {
            result = file.write( buffer, BUFFER_SIZE);
            file.flush();
        }
        if (result != BUFFER_SIZE) {
            std::cerr << "Write error: " << result << std::endl;
        }
        file.close();
#elif defined(Q_OS_WIN32)
        DWORD written;
        FT_STATUS status = FT_Write( mHandle, buffer, BUFFER_SIZE, &written);
        if (status != FT_OK) {
            std::cerr << "Write error." << std::endl;
        }
        if (written != static_cast<DWORD>(BUFFER_SIZE)) {
            std::cerr << "Write not full." << std::endl;
        }
#endif
    }
}

void
MainWindow::signalNotifierActivated(int)
{
    switch (pFdNotifier->type()) {
    case QSocketNotifier::Exception:
        signalDisconnectButtonClicked();
        break;
    default:
        break;
    }
}

void
MainWindow::signalT2Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[0].signal);
        pulse->length = value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT3Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse_prev = dynamic_cast<Pulse*>(pTimingParams[0].signal);
        Pulse* pulse_next = dynamic_cast<Pulse*>(pTimingParams[1].signal);
        pulse_next->offset = pulse_prev->offset + pulse_prev->length + value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT4Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[1].signal);
        pulse->length = value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT5Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse_prev = dynamic_cast<Pulse*>(pTimingParams[1].signal);
        Pulse* pulse_next = dynamic_cast<Pulse*>(pTimingParams[2].signal);
        pulse_next->offset = pulse_prev->offset + value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT6Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse_prev = dynamic_cast<Pulse*>(pTimingParams[2].signal);
        Pulse* pulse_next = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        pulse_next->offset = pulse_prev->offset + value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT7Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        pulse->length = value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT8Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse_prev = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        Pulse* pulse_next = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        pulse_next->offset = pulse_prev->offset + pulse_prev->length + value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT9Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        pulse->length = value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT10Changed(int value)
{
    if (!bIgnore) {
        Pulse* pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        Clock* clock_prev = dynamic_cast<Clock*>(pTimingParams[5].signal);
        Clock* clock_next = dynamic_cast<Clock*>(pTimingParams[7].signal);
        clock_next->offset = clock_prev->offset = pulse->offset + value - clock_prev->pulse.offset;
        clock_next->offset -= mTnSpinBoxes[13]->value();
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT11Changed(int value)
{
    if (!bIgnore) {
        Clock* clock = dynamic_cast<Clock*>(pTimingParams[5].signal);
        clock->pulse.offset = clock->pulse.offset + clock->pulse.length - value;
        clock->pulse.length = value;
        changeSignalsOffsets();
        formAll();
    }
    mTnSpinBoxes[12]->setMinimum(value);
}

void
MainWindow::signalT12Changed(int value)
{
    if (!bIgnore) {
        Clock* clock_prev = dynamic_cast<Clock*>(pTimingParams[5].signal);
        Clock* clock_next = dynamic_cast<Clock*>(pTimingParams[7].signal);
        clock_prev->pulse.offset = value - clock_prev->pulse.length;
        clock_next->pulse.offset = value - clock_next->pulse.length;
        changeSignalsOffsets();
        formAll();
    }
    mTnSpinBoxes[11]->setMaximum(value);
}

void
MainWindow::signalT13Changed(int value)
{
    if (!bIgnore) {
        Clock* clock_prev = dynamic_cast<Clock*>(pTimingParams[5].signal);
        Clock* clock_next = dynamic_cast<Clock*>(pTimingParams[7].signal);
        clock_next->offset = clock_prev->offset - value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::signalT14Changed(int value)
{
    if (!bIgnore) {
        Clock* clock = dynamic_cast<Clock*>(pTimingParams[7].signal);
        clock->pulse.offset += clock->pulse.length - value;
        clock->pulse.length = value;
        changeSignalsOffsets();
        formAll();
    }
}

void
MainWindow::setTn(unsigned int n)
{
    if (n > 1) {
        bIgnore = true;
        int value = getTnFromSignal(n);
        mTnSpinBoxes[n]->setValue(value);
        bIgnore = false;
    }
}

/**
 * @brief MainWindow::getTnFromSignal
 * @param n - Tn (tn) tee number from timing chart
 * @return offset
 */
unsigned int
MainWindow::getTnFromSignal(unsigned int n) const
{
    unsigned int res = 0;
    // previous and next pulses
    Pulse* pulse = 0;
    Pulse* pulse_prev = 0;
    Pulse* pulse_next = 0;
    // previous and next clocks
    Clock* clock = 0;
    Clock* clock_prev = 0;
    Clock* clock_next = 0;

    switch (n) {
    case 2:
        pulse = dynamic_cast<Pulse*>(pTimingParams[0].signal);
        res = pulse->length;
        break;
    case 3:
        pulse_prev = dynamic_cast<Pulse*>(pTimingParams[0].signal);
        pulse_next = dynamic_cast<Pulse*>(pTimingParams[1].signal);
        res = pulse_next->offset - (pulse_prev->offset + pulse_prev->length);
        break;
    case 4:
        pulse = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        res = pulse->length;
        break;
    case 5:
        pulse_prev = dynamic_cast<Pulse*>(pTimingParams[1].signal);
        pulse_next = dynamic_cast<Pulse*>(pTimingParams[2].signal);
        res = pulse_next->offset - pulse_prev->offset;
        break;
    case 6:
        pulse_prev = dynamic_cast<Pulse*>(pTimingParams[2].signal);
        pulse_next = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        res = pulse_next->offset - pulse_prev->offset;
        break;
    case 7:
        pulse = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        res = pulse->length;
        break;
    case 8:
        pulse_prev = dynamic_cast<Pulse*>(pTimingParams[3].signal);
        pulse_next = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        res = pulse_next->offset - (pulse_prev->offset + pulse_prev->length);
        break;
    case 9:
        pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        res = pulse->length;
        break;
    case 10:
        pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
        clock = dynamic_cast<Clock*>(pTimingParams[5].signal);
        res = clock->offset + clock->pulse.offset - pulse->offset;
        break;
    case 11:
        clock = dynamic_cast<Clock*>(pTimingParams[5].signal);
        res = clock->pulse.length;
        break;
    case 12:
        clock = dynamic_cast<Clock*>(pTimingParams[5].signal);
        res = clock->pulse.length + clock->pulse.offset;
        break;
    case 13:
        clock_prev = dynamic_cast<Clock*>(pTimingParams[5].signal);
        clock_next = dynamic_cast<Clock*>(pTimingParams[7].signal);
        res = clock_prev->offset - clock_next->offset;
        break;
    case 14:
        clock = dynamic_cast<Clock*>(pTimingParams[7].signal);
        res = clock->pulse.length;
        break;
    default:
        break;
    }
    return res;
}


void
MainWindow::changeSignalsOffsets()
{
    unsigned int offset;
    Pulse* pulse = 0;
    Clock* clock = 0;
    Clock* adc = 0;

    // Change for 3
    pulse = dynamic_cast<Pulse*>(pTimingParams[0].signal);
    offset = pulse->offset + mTnSpinBoxes[2]->value() + mTnSpinBoxes[3]->value();
    pulse = dynamic_cast<Pulse*>(pTimingParams[1].signal);
    pulse->offset = offset;

    // Change for 4-1 / 4-2
    pulse = dynamic_cast<Pulse*>(pTimingParams[1].signal);
    offset = pulse->offset + mTnSpinBoxes[5]->value();
    pulse = dynamic_cast<Pulse*>(pTimingParams[2].signal);
    pulse->offset = offset;

    // Change for 5-1 / 5-2
    pulse = dynamic_cast<Pulse*>(pTimingParams[2].signal);
    offset = pulse->offset + mTnSpinBoxes[6]->value();
    pulse = dynamic_cast<Pulse*>(pTimingParams[3].signal);
    pulse->offset = offset;

    // Change for 6
    pulse = dynamic_cast<Pulse*>(pTimingParams[3].signal);
    offset = pulse->offset + mTnSpinBoxes[7]->value() + mTnSpinBoxes[8]->value();
    pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
    pulse->offset = offset;

    // Change for 7
    pulse = dynamic_cast<Pulse*>(pTimingParams[4].signal);
    clock = dynamic_cast<Clock*>(pTimingParams[5].signal);
    clock->offset = pulse->offset - clock->pulse.offset + mTnSpinBoxes[10]->value();

    // Change for 9
    adc = dynamic_cast<Clock*>(pTimingParams[7].signal);
    adc->offset = clock->offset - mTnSpinBoxes[13]->value();
}
