
#include "channelviewer.h"
#include "ui_channelviewer.h"

ChannelViewer::ChannelViewer(TrackerSettings *trk, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChannelViewer)
{
    ui->setupUi(this);
    trkset = trk;
    board = nullptr;
    connect(ui->tabWidget,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));
    connect(trkset,SIGNAL(liveDataChanged()),this,SLOT(chDataUpdated()));
    connect(ui->cmdClose,SIGNAL(clicked()),this,SLOT(closeClicked()));
    connect(&nodataTimer,SIGNAL(timeout()),this,SLOT(noDataTimeout()));

    addBars();

    //Start disabled
    noDataTimeout();
    staleData = true;
    nodataTimer.setInterval(1000);
    nodataTimer.start();
    setWindowTitle("Channel Viewer");

    ui->tabsbusin->setVisible(false);
}

ChannelViewer::~ChannelViewer()
{
    delete ui;
}

void ChannelViewer::tabChanged(int index)
{
    if(board == nullptr)
        return;
    QMap<QString,bool> di;
    di["chout"] = false;
    di["btch"] = false;
    di["ppmch"] = false;
    di["uartch"] = false;

    switch(index) {
    case 0: // Channel Outputs
        di["chout"] = true;
        trkset->setDataItemSend(di);
        break;
    case 1: // PPM inputs
        di["ppmch"] = true;
        trkset->setDataItemSend(di);
        break;
    case 2: // SBUS inputs
        di["uartch"] = true;
        trkset->setDataItemSend(di);
        break;
    case 3: // BT inputs
        di["btch"] = true;
        trkset->setDataItemSend(di);
        break;
    }
}

void ChannelViewer::closeClicked()
{
    QMap<QString,bool> di;
    di["chout"] = false;
    di["btch"] = false;
    di["ppmch"] = false;
    di["uartch"] = false;
    if(board != nullptr)
        trkset->setDataItemSend(di);
    hide();
}

void ChannelViewer::chDataUpdated()
{
    nodataTimer.stop();
    nodataTimer.start();

    for(int i=0;i<CHANNELS;i++) {
        outbars[i]->setEnabled(true);
        ppminbars[i]->setEnabled(true);
        sbusinbars[i]->setEnabled(true);
    }
    for(int i=0;i<BTCHANNELS;i++) {
        btinbars[i]->setEnabled(true);
    }

    switch(ui->tabWidget->currentIndex()) {
    case 0: { // Channel Outputs
        for(int i=0;i<CHANNELS;i++) {
            int value = trkset->liveData(QString("chout[%1]").arg(i)).toInt();
            if(value == 0)
                value = TrackerSettings::PPM_CENTER;
            outbars[i]->setValue(value);
        }
        break;
    }
    case 1: { // PPM inputs
        bool lastch=true;
        for(int i=0;i<CHANNELS;i++) {
            int ppminv = trkset->liveData(QString("ppmch[%1]").arg(i)).toInt();
            if(ppminv == 0)  // On first zero this is how many ch's are available
                lastch = false;
            if(i==0) {
                if(lastch == false)
                    lblPPMin->setVisible(true);
                else
                    lblPPMin->setVisible(false);
            }
            ppminbars[i]->setVisible(lastch);
            ppminbars[i]->setValue(ppminv);
        }
        break;
    }
    case 2: { // SBUS inputs
        bool lastch=true;
        for(int i=0;i<CHANNELS;i++) {
            int sbusch = trkset->liveData(QString("uartch[%1]").arg(i)).toInt();
            if(sbusch == 0)
                lastch = false;
            if(i==0) {
                if(lastch == false)
                    lblSBUSin->setVisible(true);
                else
                    lblSBUSin->setVisible(false);
            }
            sbusinbars[i]->setVisible(lastch);
            sbusinbars[i]->setValue(sbusch);
        }
        break;
    }
    case 3: { // BT inputs
        bool btchsdetected=false;
        for(int i=0;i<BTCHANNELS;i++) {
            int btch = trkset->liveData(QString("btch[%1]").arg(i)).toInt();
            if(btch != 0) {
                btchsdetected = true;
            }
            btinbars[i]->setVisible(btch);
            btinbars[i]->setValue(btch);
        }
        if(!btchsdetected)
            lblBTin->setVisible(true);
        else
            lblBTin->setVisible(false);
    }
    }
}

void ChannelViewer::noDataTimeout()
{
    for(int i=0;i<CHANNELS;i++) {
        outbars[i]->setEnabled(false);
        ppminbars[i]->setEnabled(false);
        sbusinbars[i]->setEnabled(false);
    }
    for(int i=0;i<BTCHANNELS;i++) {
        btinbars[i]->setEnabled(false);
    }
}

void ChannelViewer::addBars()
{
    QString stylesheet = "QProgressBar {"
            "background-color: #dbdbdb;"
            "border-width: 2px;"
            "border-radius: 7px;}"
        "QProgressBar::chunk {"
            "border-top-right-radius: 7px;"
            "border-top-left-radius: 7px;"
            "border-bottom-right-radius: 7px;"
            "border-bottom-left-radius: 7px;"
            "background-color: %1;}";

    colors[0] = new QColor(255,0,0);
    colors[1] = new QColor(0,255,0);
    colors[2] = new QColor(138, 222, 255);
    colors[3] = new QColor(183, 0, 255);
    colors[4] = new QColor(255, 153, 0);
    colors[5] = new QColor(212, 63, 115);
    colors[6] = new QColor(70, 212, 63);
    colors[7] = new QColor(55, 219, 195);
    colors[8] = new QColor(219, 55, 156);
    colors[9] = new QColor(235, 231, 26);
    colors[10] = new QColor(45, 173, 19);
    colors[11] = new QColor(19, 127, 173);
    colors[12] = new QColor(56, 126, 255);
    colors[13] = new QColor(153, 42, 222);
    colors[14] = new QColor(86, 209, 199);
    colors[15] = new QColor(209, 112, 33);

    // Channel Outputs
    layoutchout = new QVBoxLayout(ui->tabchout);
    ui->tabchout->setLayout(layoutchout);
    for(int i=0;i<CHANNELS;i++) {
        outbars[i] = new QProgressBar;
        outbars[i]->setMinimum(TrackerSettings::MIN_PWM);
        outbars[i]->setMaximum(TrackerSettings::MAX_PWM);
        outbars[i]->setValue(TrackerSettings::PPM_CENTER);
        outbars[i]->setFormat(QString("Ch%1 %v").arg(i+1));
        outbars[i]->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        outbars[i]->setStyleSheet(stylesheet.arg(colors[i]->name()));
        layoutchout->addWidget(outbars[i]);
    }

    // PPM Inputs
    layoutppmin = new QVBoxLayout(ui->tabppmin);
    ui->tabppmin->setLayout(layoutppmin);
    for(int i=0;i<CHANNELS;i++) {
        ppminbars[i] = new QProgressBar;
        ppminbars[i]->setMinimum(TrackerSettings::MIN_PWM);
        ppminbars[i]->setMaximum(TrackerSettings::MAX_PWM);
        ppminbars[i]->setValue(TrackerSettings::PPM_CENTER);
        ppminbars[i]->setFormat(QString("Ch%1 %v").arg(i+1));
        ppminbars[i]->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        ppminbars[i]->setStyleSheet(stylesheet.arg(colors[i]->name()));
        layoutppmin->addWidget(ppminbars[i]);
    }
    lblPPMin = new QLabel();
    layoutppmin->addWidget(lblPPMin);
    lblPPMin->setVisible(false);
    lblPPMin->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    lblPPMin->setText("No PPM Input channels detected");

    // SBUS Inputs
    layoutsbusin = new QVBoxLayout(ui->tabsbusin);
    ui->tabsbusin->setLayout(layoutsbusin);
    for(int i=0;i<CHANNELS;i++) {
        sbusinbars[i] = new QProgressBar;
        sbusinbars[i]->setMinimum(TrackerSettings::MIN_PWM);
        sbusinbars[i]->setMaximum(TrackerSettings::MAX_PWM);
        sbusinbars[i]->setValue(TrackerSettings::PPM_CENTER);
        sbusinbars[i]->setFormat(QString("Ch%1 %v").arg(i+1));
        sbusinbars[i]->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        sbusinbars[i]->setStyleSheet(stylesheet.arg(colors[i]->name()));
        layoutsbusin->addWidget(sbusinbars[i]);
    }
    lblSBUSin = new QLabel();
    layoutsbusin->addWidget(lblSBUSin);
    lblSBUSin->setVisible(false);
    lblSBUSin->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    lblSBUSin->setText("No UART (Sbus/Crsf) input channels detected");

    // BT Inputs
    layoutbtin = new QVBoxLayout(ui->tabbtin);
    ui->tabbtin->setLayout(layoutbtin);
    for(int i=0;i<BTCHANNELS;i++) {
        btinbars[i] = new QProgressBar;
        btinbars[i]->setMinimum(TrackerSettings::MIN_PWM);
        btinbars[i]->setMaximum(TrackerSettings::MAX_PWM);
        btinbars[i]->setValue(TrackerSettings::PPM_CENTER);
        btinbars[i]->setFormat(QString("Ch%1 %v").arg(i+1));
        btinbars[i]->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        btinbars[i]->setStyleSheet(stylesheet.arg(colors[i]->name()));
        layoutbtin->addWidget(btinbars[i]);
    }
    lblBTin = new QLabel();
    layoutbtin->addWidget(lblBTin);
    lblBTin->setVisible(false);
    lblBTin->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    lblBTin->setText("No BT Input channels detected");
}

void ChannelViewer::showEvent(QShowEvent *event)
{
    tabChanged(ui->tabWidget->currentIndex());
    event->accept();
}

void ChannelViewer::closeEvent(QCloseEvent *event)
{
    closeClicked();
    event->accept();
}
