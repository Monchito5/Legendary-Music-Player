#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/qaudiooutput.h>
#include <qpushbutton.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mediaPlayer(new QMediaPlayer(this)),
    audioOutput(new QAudioOutput(this)),
    repeatMode(false),
    shuffleMode(false)
{
    ui->setupUi(this);

    // Configurar reproductor de audio
    mediaPlayer->setAudioOutput(audioOutput);

    // Inicializar widgets
    playlistWidget = new PlaylistWidget(this);
    playerControls = new PlayerControls(this);
    visualizer = new Visualizer(this);

    // Añadir widgets a la interfaz
    ui->playlistContainer->layout()->addWidget(playlistWidget);
    ui->controlsContainer->layout()->addWidget(playerControls);
    ui->visualizerContainer->layout()->addWidget(visualizer);

    // Configurar conexiones
    setupConnections();
    setupUI();
    // Configurar menús
    setupMenus();

    // Cargar configuración
    loadSettings();

    // Establecer título de la ventana
    setWindowTitle("Legendary Music Player");
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupConnections()
{
    // Conectar playlist con reproductor
    connect(playlistWidget, &PlaylistWidget::itemSelected,
            this, &MainWindow::onPlaylistItemSelected);

    // Conectar controles con reproductor
    connect(playerControls, &PlayerControls::play, mediaPlayer, &QMediaPlayer::play);
    connect(playerControls, &PlayerControls::pause, mediaPlayer, &QMediaPlayer::pause);
    connect(playerControls, &PlayerControls::stop, mediaPlayer, &QMediaPlayer::stop);
    connect(playerControls, &PlayerControls::next, playlistWidget, &PlaylistWidget::playNext);
    connect(playerControls, &PlayerControls::previous, playlistWidget, &PlaylistWidget::playPrevious);
    connect(playerControls, &PlayerControls::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(playerControls, &PlayerControls::positionChanged, mediaPlayer, &QMediaPlayer::setPosition);

    // Conectar reproductor con controles y visualizador
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onPlayerStateChanged);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &MainWindow::onPositionChanged);

    // Conectar eventos de fin de reproducción para avanzar automáticamente
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged,
            this, &MainWindow::onMediaStatusChanged);

    // Conectar señales de modo de repetición y aleatorio
    connect(playerControls, &PlayerControls::repeatModeChanged,
            this, &MainWindow::onRepeatModeChanged);
    connect(playerControls, &PlayerControls::shuffleModeChanged,
            this, &MainWindow::onShuffleModeChanged);

    // Sincronizar el estado de shuffle entre PlayerControls y PlaylistWidget
    connect(playerControls, &PlayerControls::shuffleModeChanged,
            [this](bool enabled) {
                // Buscar el botón shuffle en PlaylistWidget y sincronizar su estado
                QPushButton* shuffleButton = playlistWidget->findChild<QPushButton*>("shuffleButton");
                if (shuffleButton) {
                    shuffleButton->setChecked(enabled);
                }
            });
}

void MainWindow::setupMenus()
{
    // Menú Archivo
    connect(ui->actionOpen_File, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(ui->actionOpen_Folder, &QAction::triggered, this, &MainWindow::onOpenFolder);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);

    // Menú Herramientas
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onShowSettings);
    connect(ui->actionToggle_Visualization, &QAction::triggered, this, &MainWindow::onToggleVisualization);
}

void MainWindow::loadSettings()
{
    QSettings settings("LegendaryMusicPlayer", "Settings");

    // Restaurar geometría de la ventana
    restoreGeometry(settings.value("geometry").toByteArray());

    // Restaurar volumen
    int volume = settings.value("volume", 50).toInt();
    audioOutput->setVolume(volume / 100.0);
    playerControls->setVolume(volume);

    // Restaurar última carpeta de música
    QString lastDir = settings.value("lastDirectory",
                                     QStandardPaths::writableLocation(QStandardPaths::MusicLocation))
                          .toString();
    playlistWidget->setLastDirectory(lastDir);
}

void MainWindow::saveSettings()
{
    QSettings settings("LegendaryMusicPlayer", "Settings");

    // Guardar geometría de la ventana
    settings.setValue("geometry", saveGeometry());

    // Guardar volumen
    settings.setValue("volume", static_cast<int>(audioOutput->volume() * 100));

    // Guardar última carpeta de música
    settings.setValue("lastDirectory", playlistWidget->lastDirectory());
}

void MainWindow::onPlaylistItemSelected(const QUrl &url)
{
    mediaPlayer->setSource(url);
    mediaPlayer->play();
}

void MainWindow::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
    playerControls->updateState(state);

    // Activar/desactivar visualizador
    if (state == QMediaPlayer::PlayingState) {
        visualizer->start();
    } else {
        visualizer->stop();
    }
}

void MainWindow::onDurationChanged(qint64 duration)
{
    playerControls->setDuration(duration);
}

void MainWindow::onPositionChanged(qint64 position)
{
    playerControls->setPosition(position);
}

void MainWindow::onVolumeChanged(int volume)
{
    audioOutput->setVolume(volume / 100.0);
}

void MainWindow::onOpenFile()
{
    QString lastDir = playlistWidget->lastDirectory();
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open Music Files"),
                                                      lastDir,
                                                      tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a)"));
    if (!files.isEmpty()) {
        QFileInfo fileInfo(files.first());
        playlistWidget->setLastDirectory(fileInfo.absolutePath());
        playlistWidget->addFiles(files);
    }
}

void MainWindow::onOpenFolder()
{
    QString lastDir = playlistWidget->lastDirectory();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Abrir carpeta"),
                                                    lastDir,
                                                    QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        playlistWidget->setLastDirectory(dir);
        playlistWidget->addFolder(dir);
    }
}

void MainWindow::onShowSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Aplicar configuraciones
        loadSettings();
    }
}

void MainWindow::onToggleVisualization()
{
    visualizer->setVisible(!visualizer->isVisible());
    ui->actionToggle_Visualization->setChecked(visualizer->isVisible());
}

void MainWindow::onRepeatModeChanged(bool enabled)
{
    repeatMode = enabled;
}

void MainWindow::onShuffleModeChanged(bool enabled)
{
    shuffleMode = enabled;

    // Notificar al widget de playlist sobre el cambio en el modo aleatorio
    QPushButton* shuffleButton = playlistWidget->findChild<QPushButton*>("shuffleButton");
    if (shuffleButton && shuffleButton->isChecked() != enabled) {
        shuffleButton->setChecked(enabled);
    }
}

void MainWindow::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        if (repeatMode) {
            // Si está en modo repetición, reproducir la misma pista de nuevo
            mediaPlayer->setPosition(0);
            mediaPlayer->play();
        } else {
            // De lo contrario, pasar a la siguiente pista
            playlistWidget->playNext();
        }
    }
}

void MainWindow::setupUI() {
    // Establecer paleta de colores moderna
    QPalette bluePalette;
    bluePalette.setColor(QPalette::Window, QColor(45, 45, 45));
    bluePalette.setColor(QPalette::WindowText, QColor(230, 230, 230));
    bluePalette.setColor(QPalette::Base, QColor(25, 25, 25));
    bluePalette.setColor(QPalette::AlternateBase, QColor(35, 35, 35));
    bluePalette.setColor(QPalette::Text, QColor(230, 230, 230));
    bluePalette.setColor(QPalette::Button, QColor(45, 45, 45));
    bluePalette.setColor(QPalette::ButtonText, QColor(230, 230, 230));
    bluePalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    bluePalette.setColor(QPalette::HighlightedText, Qt::white);

    // Aplicar paleta
    qApp->setPalette(bluePalette);

    // Establecer estilo de hoja
    qApp->setStyleSheet(
        "QToolTip { color: #ffffff; background-color:rgb(42, 130, 218); border: 1px solid white; }"
        "QWidget { background-color: #1A2035; color: #E6E6E6; }"
        "QScrollBar:vertical { background-color: #2A2A2A; width: 10px; margin: 0px; }"
        "QScrollBar::handle:vertical { background-color: #5A5A5A; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QMainWindow { background-color: #1A2035; }"
        "QMenuBar { background-color: #2A3045; color: #E6E6E6; }"
        "QMenuBar::item:selected { background-color: rgb(218, 98, 42); color: #ffffff; }"
        "QPushButton { background-color: rgb(218, 98, 42); color: #ffffff; border: 1px solid #3A4055; }"
        "QPushButton:hover { background-color: rgb(198, 88, 38); }"
        "QPushButton:pressed { background-color: rgb(178, 78, 28); }"
    );
}
