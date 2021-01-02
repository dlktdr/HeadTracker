#ifndef DIAGNOSTICDISPLAY_H
#define DIAGNOSTICDISPLAY_H

#include <QWidget>
#include "trackersettings.h"

namespace Ui {
class DiagnosticDisplay;
}

class DiagnosticDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticDisplay(TrackerSettings *ts, QWidget *parent = nullptr);
    ~DiagnosticDisplay();

private:
    Ui::DiagnosticDisplay *ui;
     TrackerSettings *trkset;
private slots:
     void updated();
};

#endif // DIAGNOSTICDISPLAY_H
