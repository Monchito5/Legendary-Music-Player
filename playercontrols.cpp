#include "playercontrols.h"
#include "ui_playercontrols.h"
#include <QStyle>

PlayerControls::PlayerControls(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerControls),
    playerState(QMediaPlayer::StoppedState),
    duration(0),
    isMuted(false),
    isRepeating(false),
    isShuffling(false)
{
    ui->setupUi(this);

    // Conectar señales y slots
    connect(ui->playPauseButton, &QPushButton::clicked, this, &PlayerControls::onPlayPauseClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &PlayerControls::onStopClicked);
    connect(ui->nextButton, &QPushButton::clicked, this, &PlayerControls::onNextClicked);
    connect(ui->previousButton, &QPushButton::clicked, this, &PlayerControls::onPreviousClicked);
    connect(ui->positionSlider, &QSlider::sliderMoved, this, &PlayerControls::onSliderMoved);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &PlayerControls::onVolumeSliderMoved);
    connect(ui->muteButton, &QPushButton::toggled, this, &PlayerControls::onMuteToggled);
    connect(ui->repeatButton, &QPushButton::toggled, this, &PlayerControls::onRepeatToggled);
    connect(ui->shuffleButton, &QPushButton::toggled, this, &PlayerControls::onShuffleToggled);

    // Configurar valores iniciales
    ui->volumeSlider->setValue(50);
    updateState(QMediaPlayer::StoppedState);
}

PlayerControls::~PlayerControls()
{
    delete ui;
}

void PlayerControls::setDuration(qint64 duration)
{
    this->duration = duration;
    ui->positionSlider->setMaximum(duration);
    updatePositionInfo(ui->positionSlider->value());
}

void PlayerControls::setPosition(qint64 position)
{
    if (!ui->positionSlider->isSliderDown()) {
        ui->positionSlider->setValue(position);
        updatePositionInfo(position);
    }
}

void PlayerControls::updateState(QMediaPlayer::PlaybackState state)
{
    if (state != playerState) {
        playerState = state;

        switch (state) {
        case QMediaPlayer::PlayingState:
            ui->playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            ui->playPauseButton->setToolTip(tr("Pause"));
            break;
        case QMediaPlayer::PausedState:
        case QMediaPlayer::StoppedState:
            ui->playPauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            ui->playPauseButton->setToolTip(tr("Play"));
            break;
        }

        ui->stopButton->setEnabled(state != QMediaPlayer::StoppedState);
    }
}

void PlayerControls::setVolume(int volume)
{
    ui->volumeSlider->setValue(volume);
}

void PlayerControls::onPlayPauseClicked()
{
    switch (playerState) {
    case QMediaPlayer::StoppedState:
    case QMediaPlayer::PausedState:
        emit play();
        break;
    case QMediaPlayer::PlayingState:
        emit pause();
        break;
    }
}

void PlayerControls::onStopClicked()
{
    emit stop();
}

void PlayerControls::onNextClicked()
{
    emit next();
}

void PlayerControls::onPreviousClicked()
{
    emit previous();
}

void PlayerControls::onSliderMoved(int position)
{
    updatePositionInfo(position);
    emit positionChanged(position);
}

void PlayerControls::onVolumeSliderMoved(int position)
{
    emit volumeChanged(position);

    if (isMuted && position > 0) {
        ui->muteButton->setChecked(false);
    }
}

void PlayerControls::onMuteToggled(bool checked)
{
    isMuted = checked;

    if (checked) {
        emit volumeChanged(0);
    } else {
        emit volumeChanged(ui->volumeSlider->value());
    }
}

// Actualizar la función onRepeatToggled para manejar la repetición sin QMediaPlaylist
void PlayerControls::onRepeatToggled(bool checked)
{
    isRepeating = checked;
    // La lógica de repetición ahora se maneja en MainWindow cuando termina una pista
    emit repeatModeChanged(checked);
}

// Actualizar la función onShuffleToggled para manejar la reproducción aleatoria sin QMediaPlaylist
void PlayerControls::onShuffleToggled(bool checked)
{
    isShuffling = checked;
    // La lógica de reproducción aleatoria ahora se maneja en PlaylistWidget
    emit shuffleModeChanged(checked);
}

void PlayerControls::updatePositionInfo(qint64 position)
{
    ui->positionLabel->setText(formatTime(position) + " / " + formatTime(duration));
}

QString PlayerControls::formatTime(qint64 timeMilliseconds)
{
    qint64 seconds = timeMilliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;

    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
