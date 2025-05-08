#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QWidget>
#include <QUrl>
#include <QStringList>
#include <QVector>
#include <QtMultimedia/QMediaPlayer>
#include <QFileInfo>
#include <memory>
#include <random>

// Declaración anticipada para reducir dependencias
namespace Ui { class PlaylistWidget; }

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

struct MetaGroup {
    QString key;
    QVector<int> indices;    // índices de pistas en ese grupo
};

struct AVLNode {
    QString key;
    QVector<int> indices;
    AVLNode *left   = nullptr;
    AVLNode *right  = nullptr;
    int height      = 1;
};

// ——————————————
// —— PlaylistManager ——
// ——————————————

class PlaylistManager {
    // Lista circular de TrackInfo
    struct Nodo { TrackInfo pista; Nodo *siguiente; };
    Nodo   *primero = nullptr;
    Nodo   *ultimo   = nullptr;
    int     cantidad = 0;

    // AVL de metadatos
    AVLNode *metaRoot = nullptr;

    // Shuffle “smart”
    QVector<int>   shuffledIndices;
    int            shufflePos = -1;
    bool           shuffleMode = false;

    // Generador de números aleatorios
    std::mt19937 rng;

    // Historial de índices (undo), capacidad fija
    static constexpr int HISTORY_CAPACITY = 32;
    int history[HISTORY_CAPACITY];
    int historyTop = 0;  // próxima posición de push
    int historySize = 0; // cuántos elementos hay
public:
    PlaylistManager();
    ~PlaylistManager();

    // Operaciones básicas
    void agregarPista(const TrackInfo& pista);
    void eliminarPista(int indice);
    void vaciar();

    // Navegación secuencial
    int indiceSiguiente(int indiceActual) const;
    int indiceAnterior(int indiceActual) const;

    // Acceso a datos
    TrackInfo pistaEn(int indice) const;
    int       cantidadDePistas() const;
    bool      estaVacia() const;

    // “Smart shuffle”
    void buildMetadataTree();                // Reconstruye el AVL de grupos
    void generateSmartShuffle(int current);  // Rellena shuffledIndices

    // Modo aleatorio
    void  setShuffleMode(bool on);
    bool  isShuffleMode() const;

    // Guardar / cargar
    bool guardarEnArchivo(const QString& ruta) const;
    bool cargarDesdeArchivo(const QString& ruta);

    // Índice actual de reproducción
    void setCurrentIndex(int idx);
    int  getCurrentIndex() const;

    // Callback al terminar pista
    using EndCallback = std::function<void(int newIndex)>;
    void setEndCallback(EndCallback cb);

    // Métodos de historial
    bool canUndo() const;
    int  undo();  // devuelve índice anterior o -1

    // Reemplazamos next/previous para registrar historial
    int nextIndexSmart();
    int previousIndexSmart();

private:
    // Helpers AVL
    AVLNode* insertOrUpdate(AVLNode* node, const QString& key, int idx);
    void     collectInOrder(AVLNode* node, QVector<MetaGroup>& out);
    void     freeTree(AVLNode* node);

    // Utility para limpiar shuffle previo
    void clearShuffle();

    void pushHistory(int idx);
    EndCallback endCb;
};

// ——————————————
// —— PlaylistWidget (UI) ——
// ——————————————

class PlaylistWidget : public QWidget {
    Q_OBJECT

public:
    explicit PlaylistWidget(QWidget *parent = nullptr);
    ~PlaylistWidget();

    // Manipulación de archivos/pistas
    void addFiles(const QStringList &files);
    void addFolder(const QString &folder);

    // Controles de reproducción
    void playNext();
    void playPrevious();

    // Directorio
    QString lastDirectory() const;
    void    setLastDirectory(const QString &dir);

    // Índice actual
    bool setCurrentIndex(int index);
    int  getCurrentIndex() const;

    // Estado y conteo
    bool isEmpty() const;
    int  count() const;

    // Datos de la pista actual
    QUrl      getCurrentTrackUrl() const;
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
    void onShuffleModeToggled(bool on);
    void onUndoClicked();
    void onSearchTextChanged(const QString &text);
    void onBufferProgressChanged(float progress);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    Ui::PlaylistWidget *ui;
    std::unique_ptr<PlaylistManager> playlistManager;
    QMediaPlayer     *player;
    QString           currentDirectory;
    int               currentIndex;

    // Auxiliares UI/data
    QStringList supportedAudioFormats() const;
    void scanFolderForMusic(const QString &folder, QStringList &files);
    void extractMetadata(TrackInfo &track);
    void initializeConnections();
    void updatePlaylistView();
    void updateCurrentTrackDisplay();
};

#endif // PLAYLISTWIDGET_H
