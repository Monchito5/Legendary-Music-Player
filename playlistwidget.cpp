#include "playlistwidget.h"
#include "ui_playlistwidget.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QtMultimedia/QMediaMetaData>
#include <algorithm>
#include <numeric>
#include <random>

// Altura y balance
static int heightOf(AVLNode* n) { return n ? n->height : 0; }
static void updateHeight(AVLNode* n) {
    n->height = 1 + std::max(heightOf(n->left), heightOf(n->right));
}
static int balanceFactor(AVLNode* n) {
    return n ? heightOf(n->left) - heightOf(n->right) : 0;
}

// Rotaciones
static AVLNode* rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    y->left = x->right;
    x->right = y;
    updateHeight(y);
    updateHeight(x);
    return x;
}
static AVLNode* rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    x->right = y->left;
    y->left = x;
    updateHeight(x);
    updateHeight(y);
    return y;
}

// Insertar o actualizar un nodo por clave
static AVLNode* insertOrUpdate(AVLNode* node, const QString& key, int idx) {
    if (!node) {
        AVLNode* n = new AVLNode{key, {idx}, nullptr,nullptr,1};
        return n;
    }
    if (key < node->key) {
        node->left = insertOrUpdate(node->left, key, idx);
    } else if (key > node->key) {
        node->right = insertOrUpdate(node->right, key, idx);
    } else {
        // Ya existe: añadir índice si no está
        if (!node->indices.contains(idx))
            node->indices.append(idx);
        return node;
    }
    updateHeight(node);
    int bf = balanceFactor(node);
    if (bf > 1 && key < node->left->key)           return rotateRight(node);
    if (bf < -1 && key > node->right->key)         return rotateLeft(node);
    if (bf > 1 && key > node->left->key) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }
    if (bf < -1 && key < node->right->key) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    return node;
}

// Recolectar grupos en orden
static void collectInOrder(AVLNode* node, QVector<MetaGroup>& out) {
    if (!node) return;
    collectInOrder(node->left, out);
    MetaGroup g; g.key = node->key; g.indices = node->indices;
    out.append(g);
    collectInOrder(node->right, out);
}

// Liberar árbol
static void freeTree(AVLNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}


// ——————————————
// —— PlaylistManager ——
// ——————————————

PlaylistManager::PlaylistManager()
    : primero(nullptr)
    , ultimo(nullptr)
    , cantidad(0)
    , metaRoot(nullptr)
    , shufflePos(-1)
    , shuffleMode(false)
    , rng(std::random_device{}())
    , historyTop(0)
    , historySize(0)
    , endCb(nullptr)
{}


PlaylistManager::~PlaylistManager() {
    vaciar();
    freeTree(metaRoot);
}

void PlaylistManager::agregarPista(const TrackInfo& pista) {
    if (!pista.isValid()) return;
    Nodo* n = new Nodo{pista, nullptr};
    if (!primero) {
        primero = n;
        n->siguiente = n;
    } else {
        n->siguiente      = primero;
        ultimo->siguiente = n;
    }
    ultimo = n;
    ++cantidad;
}

void PlaylistManager::eliminarPista(int indice) {
    if (cantidad == 0 || indice < 0 || indice >= cantidad) return;
    Nodo* prev   = ultimo;
    Nodo* actual = primero;
    for (int i = 0; i < indice; ++i) {
        prev   = actual;
        actual = actual->siguiente;
    }
    if (actual == primero) primero = primero->siguiente;
    if (actual == ultimo)  ultimo  = prev;
    prev->siguiente = actual->siguiente;
    delete actual;
    --cantidad;
    if (cantidad == 0) {
        primero = ultimo = nullptr;
    }
}

void PlaylistManager::vaciar() {
    if (!primero) return;
    Nodo* it = primero;
    do {
        Nodo* next = it->siguiente;
        delete it;
        it = next;
    } while (it != primero);
    primero = ultimo = nullptr;
    cantidad = 0;
}

// Navegación secuencial
int PlaylistManager::indiceSiguiente(int indiceActual) const {
    if (cantidad == 0) return -1;
    return (indiceActual + 1) % cantidad;
}
int PlaylistManager::indiceAnterior(int indiceActual) const {
    if (cantidad == 0) return -1;
    return (indiceActual - 1 + cantidad) % cantidad;
}

// Acceso datos
TrackInfo PlaylistManager::pistaEn(int indice) const {
    if (indice < 0 || indice >= cantidad) return {};
    Nodo* ptr = primero;
    for (int i = 0; i < indice; ++i) ptr = ptr->siguiente;
    return ptr->pista;
}
int  PlaylistManager::cantidadDePistas() const { return cantidad; }
bool PlaylistManager::estaVacia()   const { return cantidad == 0; }

bool PlaylistManager::guardarEnArchivo(const QString& rutaDelArchivo) const {
    QFile file(rutaDelArchivo);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&file);
    out << "#EXTM3U\n";
    Nodo *ptr = primero;
    for (int i = 0; i < cantidad; ++i) {
        const auto &t = ptr->pista;
        if (!t.artist.isEmpty() || !t.title.isEmpty() || t.duration > 0) {
            out << "#EXTINF:" << (t.duration/1000) << ","
                << (t.artist.isEmpty() ? "" : t.artist + " - ")
                << t.title << "\n";
        }
        out << t.url.toLocalFile() << "\n";
        ptr = ptr->siguiente;
    }
    return true;
}

bool PlaylistManager::cargarDesdeArchivo(const QString& rutaDelArchivo) {
    QFile file(rutaDelArchivo);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    vaciar();
    QTextStream in(&file);
    QString dir = QFileInfo(rutaDelArchivo).absolutePath();
    QString currTitle, currArtist;
    qint64 currDur = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("#EXTINF:")) {
            int comma = line.indexOf(',');
            currDur = line.mid(8, comma-8).toInt()*1000;
            QString rest = line.mid(comma+1);
            int dash = rest.indexOf(" - ");
            if (dash>0) {
                currArtist = rest.left(dash);
                currTitle  = rest.mid(dash+3);
            } else {
                currTitle.clear();
                currArtist.clear();
            }
        }
        else if (!line.isEmpty() && !line.startsWith('#')) {
            QString path = line;
            if (!QFileInfo(path).isAbsolute())
                path = QDir(dir).filePath(path);
            if (QFileInfo::exists(path)) {
                TrackInfo t(QUrl::fromLocalFile(path));
                t.title    = currTitle.isEmpty()
                              ? QFileInfo(path).baseName()
                              : currTitle;
                t.artist   = currArtist;
                t.duration = currDur;
                agregarPista(t);
                currTitle.clear();
                currArtist.clear();
                currDur = 0;
            }
        }
    }
    return true;
}


// ——————————————
// —— Smart Shuffle ——
// ——————————————

AVLNode* PlaylistManager::insertOrUpdate(AVLNode* node, const QString& key, int idx) {
    if (!node) {
        AVLNode* n = new AVLNode{ key, {idx}, nullptr, nullptr, 1 };
        return n;
    }
    if (key < node->key)
        node->left = insertOrUpdate(node->left, key, idx);
    else if (key > node->key)
        node->right = insertOrUpdate(node->right, key, idx);
    else {
        if (!node->indices.contains(idx))
            node->indices.append(idx);
        return node;
    }
    updateHeight(node);
    int bf = balanceFactor(node);
    // rotaciones tal cual
    // …
    return node;
}

void PlaylistManager::collectInOrder(AVLNode* node, QVector<MetaGroup>& out) {
    if (!node) return;
    collectInOrder(node->left, out);
    out.append({ node->key, node->indices });
    collectInOrder(node->right, out);
}

void PlaylistManager::freeTree(AVLNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}


// Construye el árbol AVL de metadatos
void PlaylistManager::buildMetadataTree() {
    freeTree(metaRoot);
    metaRoot = nullptr;
    Nodo* ptr = primero;
    for (int i = 0; i < cantidad; ++i) {
        auto &t = ptr->pista;
        auto addMeta = [&](const QString& field, const QString& val) {
            if (!val.isEmpty())
                metaRoot = insertOrUpdate(metaRoot, field + ":" + val, i);
        };
        addMeta("artist", t.artist);
        addMeta("album",  t.album);
        addMeta("year",   QString::number(t.duration > 0 ? (t.duration/1000) : 0));
        ptr = ptr->siguiente;
    }
}

// Genera el shuffle “inteligente”
void PlaylistManager::generateSmartShuffle(int currentIndex) {
    buildMetadataTree();
    QVector<MetaGroup> groups;
    collectInOrder(metaRoot, groups);
    std::shuffle(groups.begin(), groups.end(), rng);

    shuffledIndices.clear();
    for (auto &g : groups) {
        QVector<int> v = g.indices;
        std::shuffle(v.begin(), v.end(), rng);
        for (int idx : v)
            if (idx != currentIndex)
                shuffledIndices.append(idx);
    }
    shuffledIndices.append(currentIndex);
    shufflePos = 0;
    shuffleMode = true;
}

// Registrar callback
void PlaylistManager::setEndCallback(EndCallback cb) {
    endCb = std::move(cb);
}

// Empujar índice a la pila circular estática
void PlaylistManager::pushHistory(int idx) {
    history[historyTop] = idx;
    historyTop = (historyTop + 1) % HISTORY_CAPACITY;
    if (historySize < HISTORY_CAPACITY) ++historySize;
}

// Puede deshacer si hay al menos 2 entradas
bool PlaylistManager::canUndo() const {
    return historySize > 1;
}

// Deshacer: retrocede en el historial y retorna índice anterior
int PlaylistManager::undo() {
    if (!canUndo()) return -1;
    // La última acción está en historyTop-1
    int lastPos = (historyTop - 1 + HISTORY_CAPACITY) % HISTORY_CAPACITY;
    // La anterior está en lastPos-1
    int prevPos = (lastPos - 1 + HISTORY_CAPACITY) % HISTORY_CAPACITY;
    int prevIdx = history[prevPos];
    // Ajustar top y size para reflejar deshacer
    historyTop = lastPos;
    --historySize;
    return prevIdx;
}

// Sobrescribir next/previous para usar historial y callback
int PlaylistManager::nextIndexSmart() {
    int idx = (shufflePos + 1) % shuffledIndices.size();
    pushHistory(idx);
    if (endCb) endCb(idx);
    return idx;
}

int PlaylistManager::previousIndexSmart() {
    int idx = (shufflePos - 1 + shuffledIndices.size()) % shuffledIndices.size();
    pushHistory(idx);
    if (endCb) endCb(idx);
    return idx;
}

void PlaylistManager::setShuffleMode(bool on) {
    shuffleMode = on;
}
bool PlaylistManager::isShuffleMode() const {
    return shuffleMode;
}

void PlaylistManager::setCurrentIndex(int idx) {
    // Actualiza el índice interno y reinicia el historial
    if (idx < 0 || idx >= cantidad) return;
    // Guardar en historial si hace falta
    pushHistory(idx);
    // Ajustar shufflePos para que next/previous continúen desde aquí
    if (shuffleMode) {
        auto it = std::find(shuffledIndices.begin(), shuffledIndices.end(), idx);
        shufflePos = (it == shuffledIndices.end() ? 0 : std::distance(shuffledIndices.begin(), it));
    }
}

// ==================== PlaylistWidget ====================
PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PlaylistWidget)
    , playlistManager(std::make_unique<PlaylistManager>())
    , player(new QMediaPlayer(this))
    , currentDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation))
    , currentIndex(-1)
{
    playlistManager->setEndCallback(
        [this](int newIndex) {
            // Al terminar cada pista (callback):
            setCurrentIndex(newIndex);
            player->setSource(getCurrentTrackUrl());
            player->play();
        }
    );
    ui->setupUi(this);
    initializeConnections();
    updatePlaylistView();
}


PlaylistWidget::~PlaylistWidget() {
    delete ui;
}

// Conexiones
void PlaylistWidget::initializeConnections() {
    connect(ui->playlistView, &QListWidget::doubleClicked,
            this, [this](const QModelIndex &idx){ onItemDoubleClicked(idx.row()); });
    connect(ui->addButton, &QPushButton::clicked, this, &PlaylistWidget::onAddFilesClicked);
    connect(ui->removeButton, &QPushButton::clicked, this, &PlaylistWidget::onRemoveSelectedClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &PlaylistWidget::onClearPlaylistClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &PlaylistWidget::onSavePlaylistClicked);
    connect(ui->loadButton, &QPushButton::clicked, this, &PlaylistWidget::onLoadPlaylistClicked);
    connect(ui->shuffleButton, &QPushButton::toggled, this, &PlaylistWidget::onShuffleModeToggled);
    connect(ui->undoButton, &QPushButton::clicked, this, &PlaylistWidget::onUndoClicked);
    connect(ui->searchLineEdit, &QLineEdit::textChanged,
            this, &PlaylistWidget::onSearchTextChanged);
    connect(player, &QMediaPlayer::bufferProgressChanged,
            this, &PlaylistWidget::onBufferProgressChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged,
            this, &PlaylistWidget::onMediaStatusChanged);
    connect(player, &QMediaPlayer::mediaStatusChanged,
            this, &PlaylistWidget::handleMediaStatusChanged);
}

// Al cambiar a shuffle
void PlaylistWidget::onShuffleClicked(bool checked) {
    playlistManager->setShuffleMode(checked);
    if (checked) {
        playlistManager->generateSmartShuffle(currentIndex);
    }
}

void PlaylistWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadedMedia) {
        // Ya está cargado completamente, iniciamos
        player->play();
    }
    else if (status == QMediaPlayer::EndOfMedia) {
        // Al terminar, avanzamos
        playNext();
    }
}

void PlaylistWidget::onBufferProgressChanged(float progress) {
    if (progress >= 1.0f && player->playbackState() != QMediaPlayer::PlayingState) {
        // Buffer completo, lanzamos play
        player->play();
    }
}

void PlaylistWidget::onSearchTextChanged(const QString &text) {
    for (int i = 0; i < ui->playlistView->count(); ++i) {
        auto item = ui->playlistView->item(i);
        const bool match = item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

// Al terminar pista
void PlaylistWidget::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status != QMediaPlayer::EndOfMedia) return;
    int next = playlistManager->isShuffleMode()
                   ? playlistManager->nextIndexSmart()
                   : playlistManager->indiceSiguiente(currentIndex);
    if (setCurrentIndex(next)) {
        player->setSource(getCurrentTrackUrl());
        // player->play(); Evita fallos ocasionales
    }
}

// Al pulsar “siguiente”
void PlaylistWidget::playNext() {
    int next = playlistManager->isShuffleMode()
    ? playlistManager->nextIndexSmart()
    : playlistManager->indiceSiguiente(currentIndex);
    if (setCurrentIndex(next)) {
        player->setSource(getCurrentTrackUrl());
    }
}

// Al pulsar “anterior”
void PlaylistWidget::playPrevious() {
    int prev = playlistManager->isShuffleMode()
    ? playlistManager->previousIndexSmart()
    : playlistManager->indiceAnterior(currentIndex);
    if (setCurrentIndex(prev)) {
        player->setSource(getCurrentTrackUrl());
    }
}

bool PlaylistWidget::setCurrentIndex(int idx) {
    if (idx < 0 || idx >= playlistManager->cantidadDePistas()) return false;
    currentIndex = idx;
    playlistManager->setCurrentIndex(idx);
    // Si hay shuffle, regenerar
    if (playlistManager->isShuffleMode())
        playlistManager->generateSmartShuffle(currentIndex);
    ui->playlistView->setCurrentRow(idx);
    auto t = playlistManager->pistaEn(idx);
    emit itemSelected(t.url);
    emit trackInfoChanged(t);
    return true;
}

void PlaylistWidget::addFolder(const QString &folder) {
    QStringList files;
    scanFolderForMusic(folder, files);
    addFiles(files);
}

int PlaylistWidget::getCurrentIndex() const { return currentIndex; }
bool PlaylistWidget::isEmpty() const    { return playlistManager->estaVacia(); }
int  PlaylistWidget::count() const      { return playlistManager->cantidadDePistas(); }

QUrl PlaylistWidget::getCurrentTrackUrl() const {
    if (currentIndex<0 || currentIndex>=playlistManager->cantidadDePistas())
        return {};
    return playlistManager->pistaEn(currentIndex).url;
}

TrackInfo PlaylistWidget::getCurrentTrackInfo() const {
    if (currentIndex<0 || currentIndex>=playlistManager->cantidadDePistas())
        return {};
    return playlistManager->pistaEn(currentIndex);
}

// Actualizar vista
void PlaylistWidget::updatePlaylistView() {
    ui->playlistView->clear();
    for (int i = 0; i < playlistManager->cantidadDePistas(); ++i) {
        auto t = playlistManager->pistaEn(i);
        QString text = t.artist.isEmpty()
                           ? t.title
                           : QString("%1 - %2").arg(t.artist, t.title);
        ui->playlistView->addItem(text);
    }
}

// Auxiliares (sin cambios de firma)
QStringList PlaylistWidget::supportedAudioFormats() const {
    return { "mp3","wav","flac","ogg","m4a","aac","wma" };
}

void PlaylistWidget::scanFolderForMusic(const QString &folder, QStringList &files) {
    QDir dir(folder);
    QStringList filters;
    for (auto &fmt : supportedAudioFormats())
        filters << QString("*.%1").arg(fmt);
    for (auto &f : dir.entryList(filters, QDir::Files))
        files << dir.absoluteFilePath(f);
    for (auto &d : dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
        scanFolderForMusic(dir.absoluteFilePath(d), files);
}

void PlaylistWidget::extractMetadata(TrackInfo &track) {
    QFileInfo fi(track.url.toLocalFile());
    QString nm = fi.baseName();
    int dash = nm.indexOf(" - ");
    if (dash>0) {
        track.artist = nm.left(dash).trimmed();
        track.title  = nm.mid(dash+3).trimmed();
    } else {
        track.title = nm;
    }
}

void PlaylistWidget::updateCurrentTrackDisplay() {
    if (currentIndex>=0 && currentIndex<ui->playlistView->count())
        ui->playlistView->setCurrentRow(currentIndex);
}

// Devuelve el directorio actual
QString PlaylistWidget::lastDirectory() const {
    return currentDirectory;
}

void PlaylistWidget::setLastDirectory(const QString &directory) {
    currentDirectory = directory;
}

// Private slots
void PlaylistWidget::onUndoClicked() {
    int prevIdx = playlistManager->undo();
    if (prevIdx >= 0) {
        setCurrentIndex(prevIdx);
        player->setSource(getCurrentTrackUrl());
        player->play();
    } else {
        QMessageBox::information(this, tr("Deshacer"), tr("No hay más historial para deshacer."));
    }
}

void PlaylistWidget::onItemDoubleClicked(int row) {
    if (setCurrentIndex(row)) {
        player->setSource(getCurrentTrackUrl());
    }
}

void PlaylistWidget::addFiles(const QStringList &files) {
    bool wasEmpty = playlistManager->estaVacia();
    for (const QString &f : files) {
        if (!supportedAudioFormats().contains(QFileInfo(f).suffix(), Qt::CaseInsensitive)) {
            continue; // Skip unsupported formats
        }
        TrackInfo t(QUrl::fromLocalFile(f));
        extractMetadata(t);
        playlistManager->agregarPista(t);
    }
    updatePlaylistView();
    if (wasEmpty && playlistManager->cantidadDePistas() > 0) {
        setCurrentIndex(0);
    }
    emit playlistChanged();
}

void PlaylistWidget::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        tr("Añadir Archivos"),
        lastDirectory(),
        tr("Archivos de audio (%1)").arg(supportedAudioFormats().join(" "))
    );
    if (!files.isEmpty()) {
        setLastDirectory(QFileInfo(files.first()).absolutePath());
        addFiles(files);
    }
}

void PlaylistWidget::onRemoveSelectedClicked() {
    int row = ui->playlistView->currentRow();
    if (row >= 0 && row < playlistManager->cantidadDePistas()) {
        playlistManager->eliminarPista(row);
        updatePlaylistView();
        if (playlistManager->estaVacia()) {
            currentIndex = -1;
        } else if (row <= currentIndex) {
            setCurrentIndex(currentIndex - 1);
        }
        emit playlistChanged();
    }
}

void PlaylistWidget::onClearPlaylistClicked() {
    playlistManager->vaciar();
    ui->playlistView->clear();
    currentIndex = -1;
    emit playlistChanged();
}

void PlaylistWidget::onShuffleModeToggled(bool on) {
    playlistManager->setShuffleMode(on);
    if (on)
        playlistManager->generateSmartShuffle(currentIndex);
    updatePlaylistView();
}

void PlaylistWidget::onSavePlaylistClicked() {
    if (playlistManager->estaVacia()) {
        QMessageBox::information(this, tr("Guardar playlist"),
                                 tr("La lista está vacía, nada que guardar."));
        return;
    }

    QString fn = QFileDialog::getSaveFileName(
        this,
        tr("Guardar playlist"),
        currentDirectory,
        tr("Archivos de playlist (*.m3u *.m3u8)")
        );
    if (fn.isEmpty())
        return;

    // Actualizamos directorio
    currentDirectory = QFileInfo(fn).absolutePath();

    // Aseguramos extensión .m3u
    if (!fn.endsWith(".m3u", Qt::CaseInsensitive) &&
        !fn.endsWith(".m3u8", Qt::CaseInsensitive))
    {
        fn += ".m3u";
    }

    // Delegamos en PlaylistManager
    if (playlistManager->guardarEnArchivo(fn)) {
        QMessageBox::information(this, tr("Guardar playlist"),
                                 tr("Playlist guardada correctamente en '%1'.").arg(fn));
    } else {
        QMessageBox::critical(this, tr("Error al guardar"),
                              tr("No se pudo escribir el archivo '%1'.").arg(fn));
    }
}

void PlaylistWidget::onLoadPlaylistClicked() {
    QString fn = QFileDialog::getOpenFileName(
        this,
        tr("Cargar playlist"),
        currentDirectory,
        tr("Archivos de playlist (*.m3u *.m3u8)")
        );
    if (fn.isEmpty())
        return;

    // Actualizamos directorio
    currentDirectory = QFileInfo(fn).absolutePath();

    // Delegamos en PlaylistManager
    if (playlistManager->cargarDesdeArchivo(fn)) {
        updatePlaylistView();
        // Reiniciamos al primer elemento
        if (!playlistManager->estaVacia()) {
            setCurrentIndex(0);
        }
        QMessageBox::information(this, tr("Cargar playlist"),
                                 tr("Playlist cargada correctamente desde '%1'.").arg(fn));
    } else {
        QMessageBox::critical(this, tr("Error al cargar"),
                              tr("No se pudo leer el archivo '%1'.").arg(fn));
    }
}
