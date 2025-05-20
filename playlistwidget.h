#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QWidget>
#include <QUrl>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QSharedPointer>
#include <QtMultimedia/QMediaPlayer>
#include <QFileInfo>
#include <memory>
#include <functional>

// Declaración anticipada para reducir dependencias
namespace Ui {
class PlaylistWidget;
}

// Estructura para representar una pista de audio
struct TrackInfo {
    QUrl url;
    QString title;
    QString artist;
    QString album;
    qint64 duration;
    QString filePath;

    TrackInfo() : duration(0) {}

    TrackInfo(const QUrl& url, const QString& title = QString())
        : url(url), title(title), duration(0) {
        filePath = url.toLocalFile();
        // Extraer el nombre del archivo si no se proporciona un título
        if (title.isEmpty()) {
            QFileInfo fileInfo(filePath);
            this->title = fileInfo.baseName();
        }
    }

    bool isValid() const {
        return !url.isEmpty();
    }
};

class BSTNode {
public:
    TrackInfo track;
    BSTNode* left;
    BSTNode* right;

    BSTNode(const TrackInfo &track);
};

// Clase para gestionar el Árbol de Búsqueda
class BSTManager {
public:
    BSTManager();
    ~BSTManager();

    void insert(const TrackInfo &track);
    TrackInfo search(const QString &title) const;
    void clear();

private:
    BSTNode* root;

    BSTNode* insertRecursive(BSTNode* node, const TrackInfo &track);
    BSTNode* searchRecursive(BSTNode* node, const QString &title) const;
    void clearRecursive(BSTNode* node);
    QString normalizeKey(const QString &key) const;
};

// Clase para gestionar la lista de reproducción
class PlaylistManager {
public:
    PlaylistManager();
    ~PlaylistManager();

    // Operaciones básicas
    void addTrack(const TrackInfo& track);
    void removeTrack(int index);
    void clear();

    // Navegación
    int nextIndex(bool shuffle, int currentIndex) const;
    int previousIndex(bool shuffle, int currentIndex) const;

    // Acceso a datos
    TrackInfo trackAt(int index) const;
    int count() const;
    bool isEmpty() const;

    // Gestión de reproducción aleatoria
    void updateShuffleIndices(int currentIndex);
    void setShuffleMode(bool enabled);
    bool isShuffleModeEnabled() const;

    // Guardar/cargar
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);

    // Búsqueda
    TrackInfo searchTrack(const QString &title) const;
private:
    QVector<TrackInfo> tracks;
    QVector<int> shuffleIndices;
    bool shuffleMode;

    BSTManager bst;

    // Métodos auxiliares
    void rebuildShuffleIndices(int currentIndex = -1);
};

class PlaylistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);
    ~PlaylistWidget();

    // Métodos públicos
    void addFiles(const QStringList &files);
    void addFolder(const QString &folder);
    void playNext();
    void playPrevious();
    QString lastDirectory() const;
    void setLastDirectory(const QString &directory);
    bool setCurrentIndex(int index);
    int getCurrentIndex() const;
    bool isEmpty() const;
    int count() const;

    // Métodos para acceder a la información de las pistas
    QUrl getCurrentTrackUrl() const;
    TrackInfo getCurrentTrackInfo() const;

signals:
    void itemSelected(const QUrl &url);
    void playlistChanged();
    void trackInfoChanged(const TrackInfo &info);

private slots:
    void onItemDoubleClicked(int row);
    void onAddFilesClicked();
    void onRemoveSelectedClicked();
    void onClearPlaylistClicked();
    void onSavePlaylistClicked();
    void onLoadPlaylistClicked();
    void onShuffleClicked(bool checked);
    void updatePlaylistView();
    // Búsqueda
    void onSearchTextChanged(const QString &query);

private:
    Ui::PlaylistWidget *ui;
    std::unique_ptr<PlaylistManager> playlistManager;
    QString currentDirectory;
    int currentIndex;

    // Métodos auxiliares
    void setupUI();
    QStringList supportedAudioFormats() const;
    void scanFolderForMusic(const QString &folder, QStringList &files);
    void extractMetadata(TrackInfo &track);
    void initializeConnections();
    void updateCurrentTrackDisplay();
};

#endif // PLAYLISTWIDGET_H
