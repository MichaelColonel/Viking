#pragma once

#include <QMainWindow>

#include <map>
#include <vector>
#include <stdexcept>

#ifdef Q_OS_WIN32
#include <windef.h>
#include <winbase.h>
#include <ftd2xx.h>
#endif

namespace Ui {
class MainWindow;
}

struct Level;
struct Pulse;
struct Clock;
struct TimingParams;

class QListWidgetItem;
class QSocketNotifier;
class QSpinBox;

typedef std::map< int, std::vector<bool> > TimingMap;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    void saveSettings(const QString& filename = QString());
    void loadSettings(const QString& filename = QString());

    void form();
    void formAll();
    void setLevelParams(Level*);
    void setPulseParams(Pulse*);
    void setClockParams(Clock*);
    void initTimingMap() throw(std::range_error);
    unsigned char* formByteArray();

private slots:
    void signalCurrentItemChanged(QListWidgetItem*);
    void signalCurrentRowChanged(int);
    void signalTimingSettings(bool);

    void signalLevelChanged();
    void signalPulseOffsetChanged(int);
    void signalPulseLengthChanged(int);
    void signalClockOffsetChanged(int);
    void signalClockTimesChanged(int);
    void signalAddressChanged(int);

    void signalConnectButtonClicked();
    void signalDisconnectButtonClicked();
    void signalResetButtonClicked();
    void signalStartButtonClicked();
    void signalSyncRadioButtonToggled(bool);
    void signalWriteButtonClicked();
    void signalApplyButtonClicked();
    void signalNotifierActivated(int);
    void signalOpenSettingsClicked();
    void signalSaveSettingsClicked();
    void signalWriteLengthButtonClicked();
    void signalFullStartButtonClicked();

    void signalT2Changed(int);
    void signalT3Changed(int);
    void signalT4Changed(int);
    void signalT5Changed(int);
    void signalT6Changed(int);
    void signalT7Changed(int);
    void signalT8Changed(int);
    void signalT9Changed(int);
    void signalT10Changed(int);
    void signalT11Changed(int);
    void signalT12Changed(int);
    void signalT13Changed(int);
    void signalT14Changed(int);

private:
    void write(const char*);
    unsigned int getTnFromSignal(unsigned int n) const;
    void changeSignalsOffsets();
    void setTn(unsigned int n);

    Ui::MainWindow* ui;
    TimingParams* pTimingParams;
    TimingMap mTimingMap;
    QSocketNotifier* pFdNotifier;
    int currentChart;
    bool bIgnore;
#ifdef Q_OS_WIN32
    FT_HANDLE mHandle;
#elif defined(Q_OS_LINUX)
    int fd;
#endif
    std::vector<QSpinBox*> mTnSpinBoxes;
    double ratio;
    bool test;
};
