#include "visualizer.h"
#include "ui_visualizer.h"
#include <QPainter>
#include <QDebug>
#include <cmath>
#include <QtMultimedia/QAudioDecoder>
#include <QtMultimedia/QAudioBuffer>
#include <QRandomGenerator>
#include <QTime>

Visualizer::Visualizer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Visualizer),
    timer(new QTimer(this)),
    visualizationType(0),
    isActive(false),
    audioDecoder(new QAudioDecoder(this)),
    randomGenerator(*QRandomGenerator::global())
{
    ui->setupUi(this);

    // Inicializar vectores
    samples.resize(1024);
    samples.fill(0);
    spectrum.resize(512);
    spectrum.fill(0);

    // Inicializar generador de números aleatorios
    QTime time = QTime::currentTime();
    randomGenerator.seed(time.msecsSinceStartOfDay());

    // Configurar timer para actualizar visualización
    connect(timer, &QTimer::timeout, this, &Visualizer::onTimerTimeout);
    timer->setInterval(16); // ~60 FPS

    // Conectar selector de tipo de visualización
    connect(ui->visualizationTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Visualizer::onVisualizationTypeChanged);

    // Configurar decodificador de audio
    connect(audioDecoder, &QAudioDecoder::bufferReady, this, &Visualizer::onBufferReady);

    // Establecer fondo negro
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
}

Visualizer::~Visualizer()
{
    stop();
    delete ui;
}

void Visualizer::start()
{
    isActive = true;
    timer->start();

    // En una implementación real, aquí conectaríamos con la fuente de audio actual
    // Para esta demo, generaremos datos aleatorios para simular la visualización
    generateRandomSamples();
}

void Visualizer::stop()
{
    isActive = false;
    timer->stop();

    // Limpiar datos
    samples.fill(0);
    spectrum.fill(0);
    update();
}

void Visualizer::processBuffer(const QAudioBuffer &buffer)
{
    if (!isActive || buffer.frameCount() <= 0) {
        return;
    }

    // Extraer muestras de audio
    const qint16 *data = buffer.constData<qint16>();
    int frameCount = buffer.frameCount();
    int channelCount = buffer.format().channelCount();

    // Tomar hasta 1024 muestras
    int sampleCount = qMin(frameCount, 1024);

    for (int i = 0; i < sampleCount; ++i) {
        // Promedio de todos los canales
        float sum = 0;
        for (int ch = 0; ch < channelCount; ++ch) {
            sum += data[i * channelCount + ch];
        }
        samples[i] = sum / (channelCount * 32768.0f); // Normalizar a [-1, 1]
    }

    // Calcular espectro de frecuencia
    calculateSpectrum();
}

void Visualizer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Limpiar fondo
    painter.fillRect(rect(), Qt::black);

    if (!isActive) {
        // Mostrar mensaje cuando está inactivo
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("Play music to see visualization"));
        return;
    }

    // Dibujar según el tipo de visualización seleccionado
    switch (visualizationType) {
    case 0: // Forma de onda
        drawWaveform(painter);
        break;
    case 1: // Espectro
        drawSpectrum(painter);
        break;
    case 2: // Circular
        drawCircular(painter);
        break;
    default:
        drawWaveform(painter);
        break;
    }
}

void Visualizer::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    update();
}

void Visualizer::onVisualizationTypeChanged(int index)
{
    visualizationType = index;
    update();
}

void Visualizer::onTimerTimeout()
{
    // Para la demo, generamos nuevos datos aleatorios en cada actualización
    if (isActive) {
        generateRandomSamples();
    }
    update();
}

void Visualizer::onBufferReady()
{
    QAudioBuffer buffer = audioDecoder->read();
    if (buffer.isValid()) {
        processBuffer(buffer);
    }
}

void Visualizer::generateRandomSamples()
{
    // Generar datos aleatorios para la visualización con más suavidad

    // Actualizar muestras con un poco de continuidad (para que no sea totalmente aleatorio)
    for (int i = 0; i < samples.size(); ++i) {
        // Añadir un pequeño cambio aleatorio a la muestra anterior
        float change = (randomGenerator.generateDouble() * 0.2f) - 0.1f;
        samples[i] = qBound(-1.0f, samples[i] + change, 1.0f);
    }

    // Calcular espectro basado en las nuevas muestras
    calculateSpectrum();
}

void Visualizer::drawWaveform(QPainter &painter)
{
    int width = this->width();
    int height = this->height();
    int midY = height / 2;

    painter.setPen(QPen(QColor(0, 255, 0), 2));

    QPointF prevPoint(0, midY);

    for (int i = 0; i < samples.size(); ++i) {
        float x = i * width / static_cast<float>(samples.size());
        float y = midY * (1 - samples[i]);

        QPointF currentPoint(x, y);
        painter.drawLine(prevPoint, currentPoint);
        prevPoint = currentPoint;
    }
}

void Visualizer::drawSpectrum(QPainter &painter)
{
    int width = this->width();
    int height = this->height();
    int barWidth = qMax(1, width / spectrum.size());

    for (int i = 0; i < spectrum.size(); ++i) {
        float amplitude = spectrum[i];
        int barHeight = static_cast<int>(amplitude * height);

        // Gradiente de color basado en la frecuencia
        int hue = (i * 240) / spectrum.size();
        QColor color = QColor::fromHsv(hue, 255, 255);

        painter.fillRect(i * barWidth, height - barHeight, barWidth - 1, barHeight, color);
    }
}

void Visualizer::drawCircular(QPainter &painter)
{
    int width = this->width();
    int height = this->height();
    int centerX = width / 2;
    int centerY = height / 2;
    int radius = qMin(width, height) / 3;

    painter.translate(centerX, centerY);

    // Dibujar círculos concéntricos
    painter.setPen(QPen(QColor(30, 30, 30), 1));
    for (int r = radius / 4; r <= radius; r += radius / 4) {
        painter.drawEllipse(-r, -r, r * 2, r * 2);
    }

    // Dibujar forma de onda circular
    painter.setPen(QPen(QColor(0, 255, 255), 2));

    const int numPoints = 72;
    QPointF points[numPoints];

    for (int i = 0; i < numPoints; ++i) {
        float angle = i * 2 * M_PI / numPoints;
        int sampleIndex = (i * samples.size()) / numPoints;
        float sampleValue = samples[sampleIndex];

        float r = radius * (1.0f + sampleValue * 0.5f);
        float x = r * cos(angle);
        float y = r * sin(angle);

        points[i] = QPointF(x, y);
    }

    for (int i = 0; i < numPoints; ++i) {
        int next = (i + 1) % numPoints;
        painter.drawLine(points[i], points[next]);
    }

    // Dibujar espectro como barras radiales
    painter.setPen(Qt::NoPen);

    const int numBars = 36;
    for (int i = 0; i < numBars; ++i) {
        float angle = i * 2 * M_PI / numBars;
        int spectrumIndex = (i * spectrum.size()) / numBars;
        float amplitude = spectrum[spectrumIndex] * 1.5f;

        float r = radius * 1.2f + amplitude * radius;
        float x = r * cos(angle);
        float y = r * sin(angle);

        // Gradiente de color basado en la amplitud
        int hue = static_cast<int>(amplitude * 240);
        QColor color = QColor::fromHsv(hue, 255, 255, 200);
        painter.setBrush(color);

        QPointF points[4] = {
            QPointF(radius * 1.2f * cos(angle), radius * 1.2f * sin(angle)),
            QPointF(r * cos(angle), r * sin(angle)),
            QPointF(r * cos(angle + 2 * M_PI / numBars), r * sin(angle + 2 * M_PI / numBars)),
            QPointF(radius * 1.2f * cos(angle + 2 * M_PI / numBars), radius * 1.2f * sin(angle + 2 * M_PI / numBars))
        };

        painter.drawPolygon(points, 4);
    }
}

void Visualizer::calculateSpectrum()
{
    // Implementación simplificada de FFT
    // En una implementación real, se usaría una biblioteca como FFTW

    // Simulación de espectro basado en las muestras
    for (int i = 0; i < spectrum.size(); ++i) {
        float sum = 0;
        int start = i * samples.size() / spectrum.size();
        int end = (i + 1) * samples.size() / spectrum.size();

        for (int j = start; j < end; ++j) {
            sum += fabs(samples[j]);
        }

        // Suavizado temporal
        spectrum[i] = spectrum[i] * 0.7f + (sum / (end - start)) * 0.3f;
    }
}
