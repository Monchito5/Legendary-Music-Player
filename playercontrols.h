#ifndef PLAYERCONTROLS_H
#define PLAYERCONTROLS_H

#include <QWidget>
#include <QtMultimedia/QMediaPlayer>

namespace Ui {
class PlayerControls;
}

class PlayerControls : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerControls(QWidget *parent = nullptr);
    ~PlayerControls();

    void setDuration(qint64 duration);
    void setPosition(qint64 position);
    void updateState(QMediaPlayer::PlaybackState state);
    void setVolume(int volume);

signals:
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void positionChanged(qint64 position);
    void volumeChanged(int volume);
    void repeatModeChanged(bool enabled);
    void shuffleModeChanged(bool enabled);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onNextClicked();
    void onPreviousClicked();
    void onSliderMoved(int position);
    void onVolumeSliderMoved(int position);
    void onMuteToggled(bool checked);
    void onRepeatToggled(bool checked);
    void onShuffleToggled(bool checked);

private:
    Ui::PlayerControls *ui;
    QMediaPlayer::PlaybackState playerState;
    qint64 duration;
    bool isMuted;
    bool isRepeating;
    bool isShuffling;

    void updatePositionInfo(qint64 position);
    QString formatTime(qint64 timeMilliseconds);
};

#endif // PLAYERCONTROLS_H
