#include "tristatelabel.h"
#include <QMouseEvent>

TristateLabel::TristateLabel(QWidget *parent, ToggleMode mode) :
    QLabel(parent)
{
    toggleMode = mode;
}
void TristateLabel::toggle()
{
    if (toggleMode == Tristate) {
        if (text() == "") {
            setText("0");
        }
        else if (text() == "0") {
            setText("1");
        }
        else if (text() == "1") {
            setText("");
        }
    }
    else if (toggleMode == ZeroOne) {
        if (text() == "0") {
            setText("1");
        }
        else if (text() == "1") {
            setText("0");
        }
    }
    else if (toggleMode == OneUndefined) {
        if (text() == "") {
            setText("1");
        }
        else if (text() == "1") {
            setText("");
        }
    }

}

void TristateLabel::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        emit clicked();
    }
}