#ifndef MICROCODEEDITOR_H
#define MICROCODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include "sim.h"


class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;

class MicrocodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    MicrocodeEditor(QWidget *parent = 0, bool highlightCurrentLine = true);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void highlightSimulatedLine();
    void highlightCurrentLine();
    void clearSimulationView();

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &, int);

private:
    QWidget *lineNumberArea;

    bool highlightCurLine;

    int getMicrocodeBlockNumbers();
};


class LineNumberArea : public QWidget
{
public:
    LineNumberArea(MicrocodeEditor *editor) : QWidget(editor) {
        microcodeEditor = editor;
    }

    QSize sizeHint() const {
        return QSize(microcodeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        microcodeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    MicrocodeEditor *microcodeEditor;
};

#endif // MICROCODEEDITOR_H
