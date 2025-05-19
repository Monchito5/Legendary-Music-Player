#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include "playlistwidget.h"
#include "playercontrols.h"
#include "visualizer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPlaylistItemSelected(const QUrl &url);
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);
    void onVolumeChanged(int volume);
    void onOpenFile();
    void onOpenFolder();
    void onShowSettings();
    void onToggleVisualization();
    void onRepeatModeChanged(bool enabled);
    void onShuffleModeChanged(bool enabled);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    Ui::MainWindow *ui;
    QMediaPlayer *mediaPlayer;
    QAudioOutput *audioOutput;
    PlaylistWidget *playlistWidget;
    PlayerControls *playerControls;
    Visualizer *visualizer;

    bool repeatMode;
    bool shuffleMode;

    void setupUI();
    void setupConnections();
    void setupMenus();
    void loadSettings();
    void saveSettings();
};

#endif // MAINWINDOW_H
