#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <QWidget>
#include <QTimer>
#include <QtMultimedia/QAudioBuffer>
#include <QVector>
#include <QtMultimedia/QAudioDecoder>
#include <QRandomGenerator>

namespace Ui {
class Visualizer;
}

class Visualizer : public QWidget
{
    Q_OBJECT

public:
    explicit Visualizer(QWidget *parent = nullptr);
    ~Visualizer();

    void start();
    void stop();

public slots:
    void processBuffer(const QAudioBuffer &buffer);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onVisualizationTypeChanged(int index);
    void onTimerTimeout();
    void onBufferReady();

private:
    Ui::Visualizer *ui;
    QTimer *timer;
    QVector<float> samples;
    QVector<float> spectrum;
    int visualizationType;
    bool isActive;
    QAudioDecoder *audioDecoder;
    QRandomGenerator randomGenerator;

    void drawWaveform(QPainter &painter);
    void drawSpectrum(QPainter &painter);
    void drawCircular(QPainter &painter);
    void calculateSpectrum();
    void generateRandomSamples();
};

#endif // VISUALIZER_H
