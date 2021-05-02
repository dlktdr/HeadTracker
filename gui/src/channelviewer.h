#ifndef CHANNELVIEWER_H
#define CHANNELVIEWER_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QShowEvent>
#include "trackersettings.h"
#include "boardtype.h"

#define CHANNELS 16
#define BTCHANNELS 8

namespace Ui {
class ChannelViewer;
}

class ChannelViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ChannelViewer(TrackerSettings *trk, QWidget *parent = nullptr);
    ~ChannelViewer();
    void setBoard(BoardType *b) {board = b;}
private slots:
    void tabChanged(int);
    void closeClicked();
    void chDataUpdated();
    void noDataTimeout();
private:
    void addBars();
    bool staleData;

    Ui::ChannelViewer *ui;
    QVBoxLayout *layoutchout;
    QVBoxLayout *layoutppmin;
    QVBoxLayout *layoutsbusin;
    QVBoxLayout *layoutbtin;
    QLabel *lblPPMin;
    QLabel *lblSBUSin;
    QLabel *lblBTin;
    TrackerSettings *trkset;
    BoardType *board;
    QProgressBar *outbars[CHANNELS];
    QProgressBar *ppminbars[CHANNELS];
    QProgressBar *sbusinbars[CHANNELS];
    QProgressBar *btinbars[BTCHANNELS];
    QColor *colors[CHANNELS];
    QTimer nodataTimer;

protected:
    void showEvent(QShowEvent *event);
    void closeEvent(QCloseEvent *event);
};

#endif // CHANNELVIEWER_H
