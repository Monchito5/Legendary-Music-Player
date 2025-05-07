#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QSettings>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFile>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    
    // Conectar señales y slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::onRejected);
    connect(ui->themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onThemeChanged);
    connect(ui->browseButton, &QPushButton::clicked, this, &SettingsDialog::onBrowseOutputDirectory);
    
    // Cargar configuración actual
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::onAccepted()
{
    saveSettings();
    accept();
}

void SettingsDialog::onRejected()
{
    // Restaurar configuración original
    loadSettings();
    reject();
}

void SettingsDialog::onThemeChanged(int index)
{
    QString themeName = ui->themeComboBox->itemText(index).toLower();
    applyTheme(themeName);
}

void SettingsDialog::onBrowseOutputDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                   ui->outputDirLineEdit->text(),
                                                   QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        ui->outputDirLineEdit->setText(dir);
    }
}

void SettingsDialog::loadSettings()
{
    QSettings settings("EpicMusicPlayer", "Settings");
    
    // Cargar tema
    QString theme = settings.value("theme", "dark").toString();
    int themeIndex = ui->themeComboBox->findText(theme, Qt::MatchFixedString);
    if (themeIndex >= 0) {
        ui->themeComboBox->setCurrentIndex(themeIndex);
    }
    
    // Cargar directorio de salida
    QString outputDir = settings.value("outputDirectory", 
                                      QStandardPaths::writableLocation(QStandardPaths::MusicLocation))
                                      .toString();
    ui->outputDirLineEdit->setText(outputDir);
    
    // Cargar opciones de reproducción
    ui->crossfadeCheckBox->setChecked(settings.value("crossfade", true).toBool());
    ui->crossfadeSpinBox->setValue(settings.value("crossfadeDuration", 2.0).toDouble());
    ui->gaplessCheckBox->setChecked(settings.value("gaplessPlayback", true).toBool());
    ui->replayGainCheckBox->setChecked(settings.value("replayGain", true).toBool());
    
    // Cargar opciones de visualización
    ui->fpsSpinBox->setValue(settings.value("visualizationFPS", 60).toInt());
    ui->sensitivitySlider->setValue(settings.value("visualizationSensitivity", 50).toInt());
}

void SettingsDialog::saveSettings()
{
    QSettings settings("EpicMusicPlayer", "Settings");
    
    // Guardar tema
    settings.setValue("theme", ui->themeComboBox->currentText().toLower());
    
    // Guardar directorio de salida
    settings.setValue("outputDirectory", ui->outputDirLineEdit->text());
    
    // Guardar opciones de reproducción
    settings.setValue("crossfade", ui->crossfadeCheckBox->isChecked());
    settings.setValue("crossfadeDuration", ui->crossfadeSpinBox->value());
    settings.setValue("gaplessPlayback", ui->gaplessCheckBox->isChecked());
    settings.setValue("replayGain", ui->replayGainCheckBox->isChecked());
    
    // Guardar opciones de visualización
    settings.setValue("visualizationFPS", ui->fpsSpinBox->value());
    settings.setValue("visualizationSensitivity", ui->sensitivitySlider->value());
}

void SettingsDialog::applyTheme(const QString &themeName)
{
    QFile styleFile;
    
    if (themeName == "dark") {
        styleFile.setFileName(":/styles/dark.qss");
    } else if (themeName == "light") {
        styleFile.setFileName(":/styles/light.qss");
    } else if (themeName == "blue") {
        styleFile.setFileName(":/styles/blue.qss");
    } else {
        return;
    }
    
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = styleFile.readAll();
        qApp->setStyleSheet(style);
        styleFile.close();
    }
}
