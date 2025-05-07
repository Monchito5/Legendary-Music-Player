#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private slots:
    void onAccepted();
    void onRejected();
    void onThemeChanged(int index);
    void onBrowseOutputDirectory();

private:
    Ui::SettingsDialog *ui;
    
    void loadSettings();
    void saveSettings();
    void applyTheme(const QString &themeName);
};

#endif // SETTINGSDIALOG_H
