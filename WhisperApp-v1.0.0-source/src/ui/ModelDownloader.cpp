#include "ModelDownloader.h"
#include "../core/ModelManager.h"
#include "../core/Settings.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QHeaderView>
#include <QTimer>
#include <QCheckBox>
#include <QTextEdit>
#include <QSplitter>
#include <QFormLayout>
#include <QStorageInfo>
#include <QTime>
#include <QQueue>
#include <QDesktopServices>
#include <QIcon>
#include <QPixmap>
#include <QApplication>

ModelDownloader::ModelDownloader(ModelManager* model_manager, QWidget* parent)
    : QDialog(parent)
    , model_manager(model_manager)
    , is_downloading(false)
{
    if (!model_manager) {
        throw std::invalid_argument("ModelManager cannot be null");
    }
    
    setupUi();
    refreshModelList();
    
    // Setup update timer
    update_timer = new QTimer(this);
    update_timer->setInterval(UPDATE_INTERVAL_MS);
    connect(update_timer, &QTimer::timeout, this, &ModelDownloader::onTimerTimeout);
    
    Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", "Model downloader initialized");
}

ModelDownloader::~ModelDownloader()
{
    if (is_downloading && !current_download_id.isEmpty()) {
        model_manager->cancelDownload(current_download_id.toStdString());
    }
}

void ModelDownloader::setupUi()
{
    setWindowTitle(tr("Model Manager"));
    setModal(true);
    resize(900, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create splitter for model list and details
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side - Model list
    QWidget* leftWidget = new QWidget(splitter);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    
    // Model list group
    QGroupBox* modelGroup = new QGroupBox(tr("Available Models"), leftWidget);
    QVBoxLayout* modelLayout = new QVBoxLayout(modelGroup);
    
    createModelTable();
    modelLayout->addWidget(model_table);
    
    leftLayout->addWidget(modelGroup);
    
    // Right side - Model details
    QWidget* rightWidget = new QWidget(splitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    // Model info group
    QGroupBox* infoGroup = new QGroupBox(tr("Model Information"), rightWidget);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    model_name_label = new QLabel(tr("Select a model"), infoGroup);
    model_name_label->setWordWrap(true);
    model_size_label = new QLabel("", infoGroup);
    model_performance_label = new QLabel("", infoGroup);
    model_languages_label = new QLabel("", infoGroup);
    model_description_label = new QLabel("", infoGroup);
    model_description_label->setWordWrap(true);
    
    QFormLayout* detailsLayout = new QFormLayout();
    detailsLayout->addRow(tr("Name:"), model_name_label);
    detailsLayout->addRow(tr("Size:"), model_size_label);
    detailsLayout->addRow(tr("Performance:"), model_performance_label);
    detailsLayout->addRow(tr("Languages:"), model_languages_label);
    detailsLayout->addRow(tr("Description:"), model_description_label);
    
    infoLayout->addLayout(detailsLayout);
    rightLayout->addWidget(infoGroup);
    
    // Download statistics group
    QGroupBox* statsGroup = new QGroupBox(tr("Download Statistics"), rightWidget);
    QFormLayout* statsLayout = new QFormLayout(statsGroup);
    
    download_speed_label = new QLabel(tr("0 MB/s"), statsGroup);
    download_eta_label = new QLabel(tr("--:--"), statsGroup);
    
    statsLayout->addRow(tr("Download speed:"), download_speed_label);
    statsLayout->addRow(tr("Remaining time:"), download_eta_label);
    
    rightLayout->addWidget(statsGroup);
    
    // Disk space info
    QGroupBox* diskGroup = new QGroupBox(tr("Disk Space"), rightWidget);
    QFormLayout* diskLayout = new QFormLayout(diskGroup);
    
    disk_space_label = new QLabel(diskGroup);
    diskLayout->addRow(tr("Available space:"), disk_space_label);
    
    rightLayout->addWidget(diskGroup);
    rightLayout->addStretch();
    
    // Add widgets to splitter
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Progress group
    QGroupBox* progressGroup = new QGroupBox(tr("Download Progress"), this);
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    
    download_progress = new QProgressBar(this);
    download_progress->setTextVisible(true);
    download_status_label = new QLabel(tr("Ready"), this);
    
    progressLayout->addWidget(download_progress);
    progressLayout->addWidget(download_status_label);
    
    mainLayout->addWidget(progressGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    download_button = new QPushButton(tr("Download"), this);
    download_button->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    cancel_button = new QPushButton(tr("Cancel"), this);
    cancel_button->setEnabled(false);
    delete_button = new QPushButton(tr("Delete"), this);
    delete_button->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    verify_button = new QPushButton(tr("Verify"), this);
    refresh_button = new QPushButton(tr("Refresh"), this);
    refresh_button->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    close_button = new QPushButton(tr("Close"), this);
    
    buttonLayout->addWidget(download_button);
    buttonLayout->addWidget(cancel_button);
    buttonLayout->addWidget(delete_button);
    buttonLayout->addWidget(verify_button);
    buttonLayout->addWidget(refresh_button);
    buttonLayout->addStretch();
    buttonLayout->addWidget(close_button);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(download_button, &QPushButton::clicked, this, &ModelDownloader::onDownloadClicked);
    connect(cancel_button, &QPushButton::clicked, this, &ModelDownloader::onCancelDownload);
    connect(delete_button, &QPushButton::clicked, this, &ModelDownloader::onDeleteModel);
    connect(verify_button, &QPushButton::clicked, this, &ModelDownloader::onVerifyModel);
    connect(refresh_button, &QPushButton::clicked, this, &ModelDownloader::refreshModelList);
    connect(close_button, &QPushButton::clicked, this, &QDialog::accept);
    
    updateDiskSpace();
}

void ModelDownloader::createModelTable()
{
    model_table = new QTableWidget(0, COLUMN_COUNT, this);
    
    QStringList headers;
    headers << tr("Status") << tr("Model") << tr("Size") << tr("Performance") << tr("Languages") << tr("Description");
    model_table->setHorizontalHeaderLabels(headers);
    
    model_table->horizontalHeader()->setStretchLastSection(true);
    model_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    model_table->setAlternatingRowColors(true);
    model_table->verticalHeader()->setVisible(false);
    model_table->setSortingEnabled(true);
    
    // Set column widths
    model_table->setColumnWidth(COL_STATUS, 80);
    model_table->setColumnWidth(COL_NAME, 120);
    model_table->setColumnWidth(COL_SIZE, 80);
    model_table->setColumnWidth(COL_PERFORMANCE, 100);
    model_table->setColumnWidth(COL_LANGUAGES, 120);
    
    connect(model_table, &QTableWidget::currentItemChanged,
            this, [this](QTableWidgetItem* current, QTableWidgetItem* previous) {
                Q_UNUSED(previous)
                if (current) {
                    onModelSelectionChanged(current->row(), current->column());
                }
            });
}

void ModelDownloader::populateModelTable()
{
    model_table->setRowCount(0);
    model_status_map.clear();
    
    // Get available models from ModelManager
    auto models = model_manager->getAvailableModels();
    
    for (const auto& model : models) {
        QString model_id = QString::fromStdString(model.id);
        ModelStatus status = getModelStatus(model_id);
        model_status_map[model_id] = status;
        
        int row = model_table->rowCount();
        model_table->insertRow(row);
        
        updateModelRow(row, model, status);
    }
    
    updateButtonStates();
}

void ModelDownloader::updateModelRow(int row, const ModelInfo& model, ModelStatus status)
{
    QString model_id = QString::fromStdString(model.id);
    
    // Status column
    QTableWidgetItem* statusItem = new QTableWidgetItem();
    statusItem->setIcon(getStatusIcon(status));
    statusItem->setData(Qt::UserRole, model_id);
    
    switch (status) {
        case ModelStatus::NotDownloaded:
            statusItem->setText(tr("Not Downloaded"));
            break;
        case ModelStatus::Downloading:
            statusItem->setText(tr("Downloading"));
            break;
        case ModelStatus::Downloaded:
            statusItem->setText(tr("Downloaded"));
            statusItem->setForeground(Qt::darkGreen);
            break;
        case ModelStatus::UpdateAvailable:
            statusItem->setText(tr("Update Available"));
            statusItem->setForeground(Qt::blue);
            break;
        case ModelStatus::Corrupted:
            statusItem->setText(tr("Corrupted"));
            statusItem->setForeground(Qt::red);
            break;
        case ModelStatus::Queued:
            statusItem->setText(tr("Queued"));
            statusItem->setForeground(Qt::darkYellow);
            break;
    }
    
    model_table->setItem(row, COL_STATUS, statusItem);
    
    // Name column
    model_table->setItem(row, COL_NAME, new QTableWidgetItem(QString::fromStdString(model.name)));
    
    // Size column
    model_table->setItem(row, COL_SIZE, new QTableWidgetItem(formatFileSize(model.size_bytes)));
    
    // Performance column
    QString perf = QString("Acc: %1% / Spd: %2%")
                   .arg(model.performance.accuracy, 0, 'f', 0)
                   .arg(model.performance.relative_speed * 100, 0, 'f', 0);
    model_table->setItem(row, COL_PERFORMANCE, new QTableWidgetItem(perf));
    
    // Languages column
    QString langs;
    if (model.capabilities.multilingual) {
        langs = tr("Multilingual (%1)").arg(model.capabilities.languages.size());
    } else if (!model.capabilities.languages.empty()) {
        langs = QString::fromStdString(model.capabilities.languages[0]).toUpper();
    }
    model_table->setItem(row, COL_LANGUAGES, new QTableWidgetItem(langs));
    
    // Description column
    model_table->setItem(row, COL_DESCRIPTION, new QTableWidgetItem(QString::fromStdString(model.description)));
}

ModelStatus ModelDownloader::getModelStatus(const QString& model_id) const
{
    std::string id = model_id.toStdString();
    
    if (model_manager->isDownloading(id)) {
        return ModelStatus::Downloading;
    }
    
    if (model_manager->isModelDownloaded(id)) {
        if (model_manager->verifyModel(id)) {
            return ModelStatus::Downloaded;
        } else {
            return ModelStatus::Corrupted;
        }
    }
    
    return ModelStatus::NotDownloaded;
}

void ModelDownloader::showWithModel(const QString& model_id)
{
    show();
    
    if (!model_id.isEmpty()) {
        // Find and select the model in the table
        for (int row = 0; row < model_table->rowCount(); ++row) {
            QTableWidgetItem* item = model_table->item(row, COL_STATUS);
            if (item && item->data(Qt::UserRole).toString() == model_id) {
                model_table->selectRow(row);
                break;
            }
        }
    }
}

QString ModelDownloader::getSelectedModel() const
{
    int currentRow = model_table->currentRow();
    if (currentRow >= 0) {
        QTableWidgetItem* item = model_table->item(currentRow, COL_STATUS);
        if (item) {
            return item->data(Qt::UserRole).toString();
        }
    }
    return QString();
}

void ModelDownloader::refreshModelList()
{
    populateModelTable();
    updateDiskSpace();
    checkForUpdates();
}

void ModelDownloader::onModelSelectionChanged(int row, int column)
{
    Q_UNUSED(column)
    
    if (row >= 0) {
        QTableWidgetItem* item = model_table->item(row, COL_STATUS);
        if (item) {
            QString model_id = item->data(Qt::UserRole).toString();
            showModelDetails(model_id);
            emit modelSelected(model_id);
        }
    }
    
    updateButtonStates();
}

void ModelDownloader::onDownloadClicked()
{
    QString model_id = getSelectedModel();
    if (model_id.isEmpty()) {
        return;
    }
    
    std::string id = model_id.toStdString();
    ModelInfo model = model_manager->getModelInfo(id);
    
    if (model.id.empty()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected model not found."));
        return;
    }
    
    // Check disk space
    uint64_t available_space = model_manager->getAvailableDiskSpace();
    if (available_space < model.size_bytes * 1.2) { // 20% buffer
        QMessageBox::critical(this, tr("Insufficient Disk Space"),
                            tr("Not enough disk space to download this model.\n"
                               "Required: %1\nAvailable: %2")
                            .arg(formatFileSize(model.size_bytes))
                            .arg(formatFileSize(available_space)));
        return;
    }
    
    // Start download
    current_download_id = model_id;
    is_downloading = true;
    
    download_status_label->setText(tr("Starting download of %1...").arg(QString::fromStdString(model.name)));
    download_progress->setValue(0);
    
    auto progress_callback = [this](const DownloadProgress& progress) {
        // Update progress on UI thread
        QMetaObject::invokeMethod(this, [this, progress]() {
            onDownloadProgress(progress);
        });
    };
    
    auto completion_callback = [this](bool success, const std::string& error) {
        // Update completion on UI thread
        QMetaObject::invokeMethod(this, [this, success, error]() {
            onDownloadComplete(current_download_id, success, QString::fromStdString(error));
        });
    };
    
    bool started = model_manager->downloadModel(id, progress_callback, completion_callback);
    
    if (!started) {
        QMessageBox::warning(this, tr("Download Failed"), tr("Failed to start download."));
        is_downloading = false;
        current_download_id.clear();
    } else {
        update_timer->start();
        updateButtonStates();
    }
}

void ModelDownloader::onCancelDownload()
{
    if (!current_download_id.isEmpty()) {
        model_manager->cancelDownload(current_download_id.toStdString());
        download_status_label->setText(tr("Cancelling download..."));
    }
}

void ModelDownloader::onDeleteModel()
{
    QString model_id = getSelectedModel();
    if (model_id.isEmpty()) {
        return;
    }
    
    std::string id = model_id.toStdString();
    ModelInfo model = model_manager->getModelInfo(id);
    
    if (!model_manager->isModelDownloaded(id)) {
        QMessageBox::information(this, tr("Not Downloaded"), tr("Model is not downloaded."));
        return;
    }
    
    int ret = QMessageBox::question(this, tr("Delete Model"),
                                   tr("Are you sure you want to delete the %1 model?")
                                   .arg(QString::fromStdString(model.name)),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (model_manager->deleteModel(id)) {
            refreshModelList();
            QMessageBox::information(this, tr("Model Deleted"),
                                   tr("Model %1 has been deleted successfully.")
                                   .arg(QString::fromStdString(model.name)));
        } else {
            QMessageBox::critical(this, tr("Delete Failed"),
                                tr("Failed to delete model %1.")
                                .arg(QString::fromStdString(model.name)));
        }
    }
}

void ModelDownloader::onVerifyModel()
{
    QString model_id = getSelectedModel();
    if (model_id.isEmpty()) {
        return;
    }
    
    std::string id = model_id.toStdString();
    ModelInfo model = model_manager->getModelInfo(id);
    
    if (!model_manager->isModelDownloaded(id)) {
        QMessageBox::information(this, tr("Not Downloaded"), tr("Model is not downloaded."));
        return;
    }
    
    bool is_valid = model_manager->verifyModel(id);
    
    if (is_valid) {
        QMessageBox::information(this, tr("Verification Successful"),
                               tr("Model %1 is valid and ready to use.")
                               .arg(QString::fromStdString(model.name)));
    } else {
        QMessageBox::warning(this, tr("Verification Failed"),
                           tr("Model %1 failed verification. Consider re-downloading.")
                           .arg(QString::fromStdString(model.name)));
    }
    
    refreshModelList();
}

void ModelDownloader::onDownloadProgress(const DownloadProgress& progress)
{
    download_progress->setValue(static_cast<int>(progress.progress_percent));
    
    QString status = tr("Downloading %1: %2%")
                    .arg(QString::fromStdString(progress.model_id))
                    .arg(progress.progress_percent, 0, 'f', 1);
    download_status_label->setText(status);
    
    download_speed_label->setText(formatSpeed(progress.speed_mbps));
    download_eta_label->setText(formatTimeRemaining(progress.eta_seconds));
}

void ModelDownloader::onDownloadComplete(const QString& model_id, bool success, const QString& error)
{
    update_timer->stop();
    is_downloading = false;
    current_download_id.clear();
    
    download_progress->setValue(success ? 100 : 0);
    
    if (success) {
        download_status_label->setText(tr("Download completed successfully"));
        refreshModelList();
        emit modelDownloaded(model_id);
        
        QMessageBox::information(this, tr("Download Complete"),
                               tr("Model %1 has been downloaded successfully.")
                               .arg(model_id));
    } else {
        download_status_label->setText(tr("Download failed: %1").arg(error));
        
        QMessageBox::critical(this, tr("Download Failed"),
                            tr("Failed to download model %1:\n%2")
                            .arg(model_id).arg(error));
    }
    
    updateButtonStates();
}

void ModelDownloader::updateDiskSpace()
{
    uint64_t available = model_manager->getAvailableDiskSpace();
    uint64_t used = model_manager->getTotalDiskUsage();
    
    disk_space_label->setText(tr("Used: %1 / Available: %2")
                             .arg(formatFileSize(used))
                             .arg(formatFileSize(available)));
    
    if (available < 1000000000) { // Less than 1GB
        disk_space_label->setStyleSheet("QLabel { color: red; }");
    } else if (available < 5000000000) { // Less than 5GB
        disk_space_label->setStyleSheet("QLabel { color: orange; }");
    } else {
        disk_space_label->setStyleSheet("");
    }
}

void ModelDownloader::updateDownloadStats()
{
    // Statistics are updated via progress callback
    updateDiskSpace();
}

void ModelDownloader::checkForUpdates()
{
    // Check for model updates using ModelManager
    model_manager->checkForUpdates([this](const std::vector<std::string>& updated_models) {
        for (const auto& model_id : updated_models) {
            QString id = QString::fromStdString(model_id);
            model_status_map[id] = ModelStatus::UpdateAvailable;
        }
        
        if (!updated_models.empty()) {
            QMetaObject::invokeMethod(this, [this]() {
                refreshModelList();
            });
        }
    });
}

void ModelDownloader::onTimerTimeout()
{
    updateDownloadStats();
}

void ModelDownloader::updateButtonStates()
{
    QString selected_model = getSelectedModel();
    bool has_selection = !selected_model.isEmpty();
    
    if (has_selection) {
        ModelStatus status = getModelStatus(selected_model);
        
        download_button->setEnabled(!is_downloading && status == ModelStatus::NotDownloaded);
        delete_button->setEnabled(!is_downloading && status == ModelStatus::Downloaded);
        verify_button->setEnabled(!is_downloading && status == ModelStatus::Downloaded);
    } else {
        download_button->setEnabled(false);
        delete_button->setEnabled(false);
        verify_button->setEnabled(false);
    }
    
    cancel_button->setEnabled(is_downloading);
    refresh_button->setEnabled(!is_downloading);
}

void ModelDownloader::showModelDetails(const QString& model_id)
{
    if (model_id.isEmpty()) {
        model_name_label->setText(tr("Select a model"));
        model_size_label->clear();
        model_performance_label->clear();
        model_languages_label->clear();
        model_description_label->clear();
        return;
    }
    
    std::string id = model_id.toStdString();
    ModelInfo model = model_manager->getModelInfo(id);
    
    if (model.id.empty()) {
        return;
    }
    
    model_name_label->setText(QString::fromStdString(model.name));
    model_size_label->setText(formatFileSize(model.size_bytes));
    
    QString perf = tr("Accuracy: %1% / Speed: %2% / Memory: %3 MB")
                  .arg(model.performance.accuracy, 0, 'f', 0)
                  .arg(model.performance.relative_speed * 100, 0, 'f', 0)
                  .arg(model.performance.memory_mb);
    model_performance_label->setText(perf);
    
    QStringList langs;
    for (const auto& lang : model.capabilities.languages) {
        langs << QString::fromStdString(lang).toUpper();
    }
    model_languages_label->setText(langs.join(", "));
    
    model_description_label->setText(QString::fromStdString(model.description));
}

QIcon ModelDownloader::getStatusIcon(ModelStatus status) const
{
    switch (status) {
        case ModelStatus::NotDownloaded:
            return style()->standardIcon(QStyle::SP_DialogCancelButton);
        case ModelStatus::Downloading:
            return style()->standardIcon(QStyle::SP_ArrowDown);
        case ModelStatus::Downloaded:
            return style()->standardIcon(QStyle::SP_DialogApplyButton);
        case ModelStatus::UpdateAvailable:
            return style()->standardIcon(QStyle::SP_BrowserReload);
        case ModelStatus::Corrupted:
            return style()->standardIcon(QStyle::SP_MessageBoxWarning);
        case ModelStatus::Queued:
            return style()->standardIcon(QStyle::SP_MediaPlay);
        default:
            return QIcon();
    }
}

bool ModelDownloader::showConfirmation(const QString& title, const QString& message)
{
    return QMessageBox::question(this, title, message, 
                               QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

QString ModelDownloader::formatFileSize(uint64_t bytes) const
{
    const uint64_t KB = 1024;
    const uint64_t MB = KB * 1024;
    const uint64_t GB = MB * 1024;
    
    if (bytes >= GB) {
        return QString::number(bytes / double(GB), 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / double(MB), 'f', 1) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / double(KB), 'f', 1) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

QString ModelDownloader::formatSpeed(float mbps) const
{
    if (mbps >= 1.0f) {
        return QString::number(mbps, 'f', 1) + " MB/s";
    } else {
        return QString::number(mbps * 1024, 'f', 0) + " KB/s";
    }
}

QString ModelDownloader::formatTimeRemaining(int seconds) const
{
    if (seconds < 0) {
        return "--:--";
    }
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
               .arg(hours)
               .arg(minutes, 2, 10, QChar('0'))
               .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
               .arg(minutes, 2, 10, QChar('0'))
               .arg(secs, 2, 10, QChar('0'));
    }
}