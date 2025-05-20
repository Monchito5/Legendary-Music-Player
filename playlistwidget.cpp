#include "playlistwidget.h"
#include "ui_playlistwidget.h"
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMessageBox>
#include <QUrl>
#include <QRandomGenerator>
#include <QTime>
#include <QtMultimedia/QMediaMetaData>
#include <QMimeDatabase>
#include <QDebug>
#include <algorithm>
#include <QLineEdit>
#include <random>

// =============== PlaylistManager Implementation ===============

PlaylistManager::PlaylistManager() : shuffleMode(false) {
}

PlaylistManager::~PlaylistManager() {
}

// Árbol BST - Constructor
BSTNode::BSTNode(const TrackInfo &track)
    : track(track), left(nullptr), right(nullptr) {}

// Funciones de Búsqueda con el Árbol BST
BSTManager::BSTManager() : root(nullptr) {}

BSTManager::~BSTManager() {
    clear();
}

void BSTManager::insert(const TrackInfo &track) {
    root = insertRecursive(root, track);
}

TrackInfo BSTManager::search(const QString &title) const {
    BSTNode* result = searchRecursive(root, normalizeKey(title));
    return result ? result->track : TrackInfo();
}

void BSTManager::clear() {
    clearRecursive(root);
    root = nullptr;
}

// Métodos auxiliares
BSTNode* BSTManager::insertRecursive(BSTNode* node, const TrackInfo &track) {
    if (!node) return new BSTNode(track);

    QString nodeKey = normalizeKey(node->track.title);
    QString newKey = normalizeKey(track.title);

    if (newKey < nodeKey) {
        node->left = insertRecursive(node->left, track);
    } else if (newKey > nodeKey) {
        node->right = insertRecursive(node->right, track);
    }

    return node;
}

BSTNode* BSTManager::searchRecursive(BSTNode* node, const QString &title) const {
    if (!node) return nullptr;

    QString nodeKey = normalizeKey(node->track.title);
    QString searchKey = normalizeKey(title);

    if (searchKey == nodeKey) return node;
    if (searchKey < nodeKey) return searchRecursive(node->left, title);
    return searchRecursive(node->right, title);
}

void BSTManager::clearRecursive(BSTNode* node) {
    if (node) {
        clearRecursive(node->left);
        clearRecursive(node->right);
        delete node;
    }
}

QString BSTManager::normalizeKey(const QString &key) const {
    return key.toLower().trimmed();
}

// Funciones del Gestor de la lista de Reproducción

void PlaylistManager::addTrack(const TrackInfo& track) {
    tracks.append(track);
    bst.insert(track); // Insertar en el BST
    // Verificar si la pista ya existe para evitar duplicados
    for (int i = 0; i < tracks.size(); ++i) {
        if (tracks[i].url == track.url) {
            // Actualizar la información si es necesario
            if (tracks[i].title.isEmpty() && !track.title.isEmpty()) {
                tracks[i].title = track.title;
            }
            if (tracks[i].artist.isEmpty() && !track.artist.isEmpty()) {
                tracks[i].artist = track.artist;
            }
            // No añadir duplicado
            return;
        }
    }

    // Añadir nueva pista
    int newIndex = tracks.size();
    tracks.append(track);

    // Actualizar índices
    rebuildShuffleIndices(newIndex);

    // Actualizar índices de reproducción aleatoria
    if (shuffleMode) {
        rebuildShuffleIndices();
    }
}

TrackInfo PlaylistManager::searchTrack(const QString &title) const {
    return bst.search(title);
}

void PlaylistManager::removeTrack(int index) {
    if (index >= 0 && index < tracks.size()) {
        tracks.removeAt(index);
        // Nota: Para una implementación completa, se necesita reconstruir el BST
        // Esto es una simplificación para el ejemplo
        rebuildShuffleIndices();
    }
}

void PlaylistManager::clear() {
    tracks.clear();
    shuffleIndices.clear();
}

// Optimizar la navegación en modo aleatorio
int PlaylistManager::nextIndex(bool shuffle, int currentIndex) const {
    if (tracks.isEmpty()) {
        return -1;
    }

    if (currentIndex < 0) {
        return 0;
    }

    if (shuffle && !shuffleIndices.isEmpty()) {
        // Encontrar la posición actual en la lista de reproducción aleatoria
        int currentPos = shuffleIndices.indexOf(currentIndex);
        if (currentPos >= 0 && currentPos < shuffleIndices.size() - 1) {
            return shuffleIndices[currentPos + 1];
        } else {
            // Volver al principio de la lista aleatoria
            return shuffleIndices.first();
        }
    } else {
        // Modo secuencial
        return (currentIndex + 1) % tracks.size();
    }
}

int PlaylistManager::previousIndex(bool shuffle, int currentIndex) const {
    if (tracks.isEmpty()) return -1;

    if (shuffle && !shuffleIndices.isEmpty()) {
        int currentPos = shuffleIndices.indexOf(currentIndex);
        if (currentPos >= 0) {
            int prevPos = (currentPos - 1 + shuffleIndices.size()) % shuffleIndices.size();
            return shuffleIndices[prevPos];
        }
    }

    // Comportamiento normal (no aleatorio)
    return (currentIndex - 1 + tracks.size()) % tracks.size();
}

TrackInfo PlaylistManager::trackAt(int index) const {
    if (index >= 0 && index < tracks.size()) {
        return tracks[index];
    }
    return TrackInfo();
}

int PlaylistManager::count() const {
    return tracks.size();
}

bool PlaylistManager::isEmpty() const {
    return tracks.isEmpty();
}

void PlaylistManager::updateShuffleIndices(int currentIndex) {
    rebuildShuffleIndices(currentIndex);
}

void PlaylistManager::setShuffleMode(bool enabled) {
    shuffleMode = enabled;
    if (shuffleMode) {
        rebuildShuffleIndices();
    }
}

bool PlaylistManager::isShuffleModeEnabled() const {
    return shuffleMode;
}

bool PlaylistManager::saveToFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "#EXTM3U\n";

    for (const TrackInfo& track : tracks) {
        // Escribir información extendida si está disponible
        if (!track.artist.isEmpty() || !track.title.isEmpty() || track.duration > 0) {
            out << "#EXTINF:" << (track.duration / 1000) << ","
                << (!track.artist.isEmpty() ? track.artist + " - " : "")
                << track.title << "\n";
        }
        out << track.url.toLocalFile() << "\n";
    }

    file.close();
    return true;
}

bool PlaylistManager::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    clear();

    QTextStream in(&file);
    QString currentTitle;
    QString currentArtist;
    qint64 currentDuration = 0;

    QFileInfo playlistFileInfo(filePath);
    QString playlistDir = playlistFileInfo.absolutePath();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("#EXTINF:")) {
            // Parsear información extendida
            int commaPos = line.indexOf(',');
            if (commaPos > 8) { // #EXTINF:xx,
                QString durationStr = line.mid(8, commaPos - 8);
                currentDuration = durationStr.toInt() * 1000; // Convertir a milisegundos

                QString titleArtist = line.mid(commaPos + 1);
                int dashPos = titleArtist.indexOf(" - ");
                if (dashPos > 0) {
                    currentArtist = titleArtist.left(dashPos);
                    currentTitle = titleArtist.mid(dashPos + 3);
                } else {
                    currentTitle = titleArtist;
                    currentArtist = "";
                }
            }
        } else if (!line.isEmpty() && !line.startsWith("#")) {
            // Línea de archivo
            QFileInfo fileInfo(line);
            QString filePath;

            if (fileInfo.isRelative()) {
                filePath = QDir(playlistDir).filePath(line);
            } else {
                filePath = line;
            }

            if (QFileInfo::exists(filePath)) {
                TrackInfo track(QUrl::fromLocalFile(filePath));
                track.title = currentTitle.isEmpty() ? fileInfo.baseName() : currentTitle;
                track.artist = currentArtist;
                track.duration = currentDuration;

                addTrack(track);

                // Resetear información para la siguiente pista
                currentTitle.clear();
                currentArtist.clear();
                currentDuration = 0;
            }
        }
    }

    file.close();
    return true;
}

void PlaylistManager::rebuildShuffleIndices(int currentIndex) {
    shuffleIndices.clear();

    // Crear índices secuenciales
    for (int i = 0; i < tracks.size(); ++i) {
        shuffleIndices.append(i);
    }

    if (tracks.size() <= 1) {
        return;
    }

    // Si hay un índice actual, mantenerlo en su posición
    int currentValue = -1;
    int currentPos = -1;

    if (currentIndex >= 0 && currentIndex < shuffleIndices.size()) {
        currentValue = currentIndex;
        currentPos = shuffleIndices.indexOf(currentValue);

        if (currentPos >= 0) {
            shuffleIndices.removeAt(currentPos);
        }
    }

    // Mezclar los índices restantes
    auto rng = QRandomGenerator::global();
    for (int i = shuffleIndices.size() - 1; i > 0; --i) {
        int j = rng->bounded(i + 1);
        if (i != j) {
            std::swap(shuffleIndices[i], shuffleIndices[j]);
        }
    }

    // Reinsertar el índice actual en su posición original si es necesario
    if (currentPos >= 0) {
        shuffleIndices.insert(currentPos, currentValue);
    }
}

// =============== PlaylistWidget Implementation ===============

PlaylistWidget::PlaylistWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlaylistWidget),
    playlistManager(std::make_unique<PlaylistManager>()),
    currentDirectory(QStandardPaths::writableLocation(QStandardPaths::MusicLocation)),
    currentIndex(-1)
{
    ui->setupUi(this);
    initializeConnections();
    QLineEdit *searchInput = new QLineEdit(this);
    searchInput->setPlaceholderText("Buscar por título...");
    connect(searchInput, &QLineEdit::textChanged, this, &PlaylistWidget::onSearchTextChanged);
    ui->verticalLayout->insertWidget(1, searchInput);
}

PlaylistWidget::~PlaylistWidget()
{
    delete ui;
}

void PlaylistWidget::initializeConnections() {
    // Conectar señales y slots
    connect(ui->playlistView, &QListWidget::doubleClicked,
            [this](const QModelIndex &index) { onItemDoubleClicked(index.row()); });
    connect(ui->addButton, &QPushButton::clicked, this, &PlaylistWidget::onAddFilesClicked);
    connect(ui->removeButton, &QPushButton::clicked, this, &PlaylistWidget::onRemoveSelectedClicked);
    connect(ui->clearButton, &QPushButton::clicked, this, &PlaylistWidget::onClearPlaylistClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &PlaylistWidget::onSavePlaylistClicked);
    connect(ui->loadButton, &QPushButton::clicked, this, &PlaylistWidget::onLoadPlaylistClicked);

    // Conectar el botón de reproducción aleatoria
    connect(ui->shuffleButton, &QPushButton::toggled, this, &PlaylistWidget::onShuffleClicked);
}

// Implementación del slot
    void PlaylistWidget::onSearchTextChanged(const QString &query) {
        TrackInfo result = playlistManager->searchTrack(query);

        ui->playlistView->clear();
        if (result.isValid()) {
            QString displayText = QString("%1 - %2").arg(result.artist, result.title);
            ui->playlistView->addItem(displayText);
        } else {
            updatePlaylistView(); // Mostrar toda la lista si no hay resultados
        }
    }

void PlaylistWidget::addFiles(const QStringList &files) {
    bool wasEmpty = playlistManager->isEmpty();
    int addedCount = 0;

    for (const QString &file : files) {
        QFileInfo fileInfo(file);
        if (fileInfo.exists() && supportedAudioFormats().contains(fileInfo.suffix().toLower())) {
            TrackInfo track(QUrl::fromLocalFile(file));
            extractMetadata(track);
            playlistManager->addTrack(track);
            addedCount++;
        }
    }

    if (addedCount > 0) {
        updatePlaylistView();

        if (wasEmpty) {
            currentIndex = 0;
            ui->playlistView->setCurrentRow(currentIndex);
        }

        emit playlistChanged();
    }
}

void PlaylistWidget::addFolder(const QString &folder) {
    QStringList files;
    scanFolderForMusic(folder, files);
    addFiles(files);
}

void PlaylistWidget::playNext() {
    if (playlistManager->isEmpty()) return;

    int nextIndex = playlistManager->nextIndex(playlistManager->isShuffleModeEnabled(), currentIndex);
    setCurrentIndex(nextIndex);
}

void PlaylistWidget::playPrevious() {
    if (playlistManager->isEmpty()) return;

    int prevIndex = playlistManager->previousIndex(playlistManager->isShuffleModeEnabled(), currentIndex);
    setCurrentIndex(prevIndex);
}

QString PlaylistWidget::lastDirectory() const {
    return currentDirectory;
}

void PlaylistWidget::setLastDirectory(const QString &directory) {
    currentDirectory = directory;
}

bool PlaylistWidget::setCurrentIndex(int index) {
    if (index >= 0 && index < playlistManager->count()) {
        currentIndex = index;
        ui->playlistView->setCurrentRow(currentIndex);

        TrackInfo track = playlistManager->trackAt(currentIndex);
        emit itemSelected(track.url);
        emit trackInfoChanged(track);

        return true;
    }
    return false;
}

int PlaylistWidget::getCurrentIndex() const {
    return currentIndex;
}

bool PlaylistWidget::isEmpty() const {
    return playlistManager->isEmpty();
}

int PlaylistWidget::count() const {
    return playlistManager->count();
}

QUrl PlaylistWidget::getCurrentTrackUrl() const {
    if (currentIndex >= 0 && currentIndex < playlistManager->count()) {
        return playlistManager->trackAt(currentIndex).url;
    }
    return QUrl();
}

TrackInfo PlaylistWidget::getCurrentTrackInfo() const {
    if (currentIndex >= 0 && currentIndex < playlistManager->count()) {
        return playlistManager->trackAt(currentIndex);
    }
    return TrackInfo();
}

void PlaylistWidget::onItemDoubleClicked(int row) {
    if (row >= 0 && row < playlistManager->count()) {
        setCurrentIndex(row);
    }
}

void PlaylistWidget::onAddFilesClicked() {
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Add Music Files"),
                                                      currentDirectory,
                                                      tr("Audio Files (*.mp3 *.wav *.flac *.ogg *.m4a)"));
    if (!files.isEmpty()) {
        QFileInfo fileInfo(files.first());
        currentDirectory = fileInfo.absolutePath();
        addFiles(files);
    }
}

void PlaylistWidget::onRemoveSelectedClicked() {
    int row = ui->playlistView->currentRow();
    if (row >= 0) {
        // Guardar el índice actual para ajustarlo después
        int oldCurrentIndex = currentIndex;

        // Eliminar la pista
        playlistManager->removeTrack(row);
        ui->playlistView->takeItem(row);

        // Ajustar el índice actual si es necesario
        if (row == oldCurrentIndex) {
            // Si se eliminó la pista actual
            if (playlistManager->isEmpty()) {
                currentIndex = -1;
            } else if (row >= playlistManager->count()) {
                currentIndex = playlistManager->count() - 1;
                ui->playlistView->setCurrentRow(currentIndex);

                // Emitir señal para reproducir la nueva pista actual
                TrackInfo track = playlistManager->trackAt(currentIndex);
                emit itemSelected(track.url);
                emit trackInfoChanged(track);
            }
        } else if (row < oldCurrentIndex) {
            // Si se eliminó una pista antes de la actual, ajustar el índice
            currentIndex--;
        }

        emit playlistChanged();
    }
}

void PlaylistWidget::onClearPlaylistClicked() {
    playlistManager->clear();
    ui->playlistView->clear();
    currentIndex = -1;

    emit playlistChanged();
}

void PlaylistWidget::onSavePlaylistClicked() {
    if (playlistManager->isEmpty()) {
        QMessageBox::information(this, tr("Save Playlist"), tr("Playlist is empty."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Playlist"),
                                                    currentDirectory,
                                                    tr("Playlist Files (*.m3u *.m3u8)"));
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".m3u", Qt::CaseInsensitive) && !fileName.endsWith(".m3u8", Qt::CaseInsensitive)) {
            fileName += ".m3u";
        }

        if (playlistManager->saveToFile(fileName)) {
            QMessageBox::information(this, tr("Save Playlist"), tr("Playlist saved successfully."));
        } else {
            QMessageBox::warning(this, tr("Save Playlist"), tr("Failed to save playlist."));
        }
    }
}

void PlaylistWidget::onLoadPlaylistClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Playlist"),
                                                    currentDirectory,
                                                    tr("Playlist Files (*.m3u *.m3u8)"));
    if (!fileName.isEmpty()) {
        if (playlistManager->loadFromFile(fileName)) {
            updatePlaylistView();
            currentIndex = -1;

            if (!playlistManager->isEmpty()) {
                currentIndex = 0;
                ui->playlistView->setCurrentRow(currentIndex);
            }

            emit playlistChanged();
        } else {
            QMessageBox::warning(this, tr("Load Playlist"), tr("Failed to load playlist."));
        }
    }
}

void PlaylistWidget::onShuffleClicked(bool checked) {
    playlistManager->setShuffleMode(checked);
    if (checked) {
        playlistManager->updateShuffleIndices(currentIndex);
    }
}

void PlaylistWidget::updatePlaylistView() {
    ui->playlistView->clear();

    for (int i = 0; i < playlistManager->count(); ++i) {
        TrackInfo track = playlistManager->trackAt(i);
        QString displayText;

        if (!track.artist.isEmpty() && !track.title.isEmpty()) {
            displayText = QString("%1 - %2").arg(track.artist, track.title);
        } else if (!track.title.isEmpty()) {
            displayText = track.title;
        } else {
            QFileInfo fileInfo(track.url.toLocalFile());
            displayText = fileInfo.fileName();
        }

        ui->playlistView->addItem(displayText);
    }
}

QStringList PlaylistWidget::supportedAudioFormats() const {
    return QStringList() << "mp3" << "wav" << "flac" << "ogg" << "m4a" << "aac" << "wma";
}

void PlaylistWidget::scanFolderForMusic(const QString &folder, QStringList &files) {
    QDir dir(folder);
    QStringList supportedFormats = supportedAudioFormats();
    QStringList filters;

    // Crear filtros para cada formato soportado
    for (const QString &format : supportedFormats) {
        filters << QString("*.%1").arg(format);
    }

    // Filtrar archivos de audio
    QStringList audioFiles = dir.entryList(filters, QDir::Files);
    for (const QString &file : audioFiles) {
        files.append(dir.absoluteFilePath(file));
    }

    // Escanear subdirectorios
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &subdir : subdirs) {
        scanFolderForMusic(dir.absoluteFilePath(subdir), files);
    }
}

void PlaylistWidget::extractMetadata(TrackInfo &track) {
    // En una implementación real, aquí extraeríamos metadatos del archivo
    // Usando QMediaMetaData o bibliotecas como TagLib

    // Ejemplo simplificado: extraer información del nombre del archivo
    QFileInfo fileInfo(track.url.toLocalFile());
    QString fileName = fileInfo.baseName();

    // Intentar extraer artista y título si el nombre tiene formato "Artista - Título"
    int dashPos = fileName.indexOf(" - ");
    if (dashPos > 0) {
        track.artist = fileName.left(dashPos).trimmed();
        track.title = fileName.mid(dashPos + 3).trimmed();
    } else {
        track.title = fileName;
    }
}

void PlaylistWidget::updateCurrentTrackDisplay() {
    if (currentIndex >= 0 && currentIndex < ui->playlistView->count()) {
        ui->playlistView->setCurrentRow(currentIndex);
    }
}

void PlaylistWidget::setupUI() {
    // Estilizar la lista de reproducción
    ui->playlistView->setAlternatingRowColors(true);
    ui->playlistView->setStyleSheet(
        "QListWidget {"
        "    background-color: #2A2A2A;"
        "    alternate-background-color: #323232;"
        "    border: 1px solid #3A3A3A;"
        "    border-radius: 4px;"
        "    padding: 2px;"
        "}"
        "QListWidget::item {"
        "    padding: 4px;"
        "    border-bottom: 1px solid #3A3A3A;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #2a82da;"
        "    color: white;"
        "}"
        "QListWidget::item:hover:!selected {"
        "    background-color: #404040;"
        "}"
        );

    // Mejorar los botones de control de playlist
    QString buttonStyle =
        "QPushButton {"
        "    background-color: #404040;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 6px 12px;"
        "    color: white;"
        "}"
        "QPushButton:hover {"
        "    background-color: #505050;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #606060;"
        "}";

    ui->addButton->setStyleSheet(buttonStyle);
    ui->removeButton->setStyleSheet(buttonStyle);
    ui->clearButton->setStyleSheet(buttonStyle);
}
