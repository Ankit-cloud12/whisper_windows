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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QCheckBox>
#include <QTextEdit>
#include <QSplitter>
#include <QFormLayout>
#include <QStorageInfo>
#include <QTime>
#include <QQueue>
#include <QDesktopServices>

struct ModelInfo {
    QString name;
    QString fileName;
    QString url;
    qint64 size;
    QString description;
    float accuracy;
    float speed;
    bool isDownloaded;
};

ModelDownloader::ModelDownloader(QWidget* parent)
    : QDialog(parent)
    , m_networkManager(nullptr)
    , m_currentDownload(nullptr)
    , m_currentModelIndex(-1)
{
    setupUI();
    loadAvailableModels();
    connectSignals();
    checkInstalledModels();
    
    Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", "Model downloader initialized");
}

ModelDownloader::~ModelDownloader()
{
    if (m_currentDownload) {
        m_currentDownload->abort();
        delete m_currentDownload;
    }
    
    if (m_networkManager) {
        delete m_networkManager;
    }
}

void ModelDownloader::setupUI()
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
    
    // Create model table
    m_modelTable = new QTableWidget(0, 6, this);
    m_modelTable->setHorizontalHeaderLabels(
        QStringList() << tr("Model") << tr("Size") << tr("Accuracy") 
                     << tr("Speed") << tr("Status") << tr("Progress")
    );
    m_modelTable->horizontalHeader()->setStretchLastSection(true);
    m_modelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_modelTable->setAlternatingRowColors(true);
    m_modelTable->verticalHeader()->setVisible(false);
    
    // Set column widths
    m_modelTable->setColumnWidth(0, 120);
    m_modelTable->setColumnWidth(1, 80);
    m_modelTable->setColumnWidth(2, 80);
    m_modelTable->setColumnWidth(3, 80);
    m_modelTable->setColumnWidth(4, 100);
    
    modelLayout->addWidget(m_modelTable);
    
    // Download queue
    m_queueCheck = new QCheckBox(tr("Queue downloads"), modelGroup);
    modelLayout->addWidget(m_queueCheck);
    
    leftLayout->addWidget(modelGroup);
    
    // Right side - Model details
    QWidget* rightWidget = new QWidget(splitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    // Model info group
    QGroupBox* infoGroup = new QGroupBox(tr("Model Information"), rightWidget);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    m_modelInfoText = new QTextEdit(infoGroup);
    m_modelInfoText->setReadOnly(true);
    m_modelInfoText->setMaximumHeight(200);
    infoLayout->addWidget(m_modelInfoText);
    
    rightLayout->addWidget(infoGroup);
    
    // Download statistics group
    QGroupBox* statsGroup = new QGroupBox(tr("Download Statistics"), rightWidget);
    QFormLayout* statsLayout = new QFormLayout(statsGroup);
    
    m_downloadSpeedLabel = new QLabel(tr("0 MB/s"), statsGroup);
    m_downloadedSizeLabel = new QLabel(tr("0 MB / 0 MB"), statsGroup);
    m_downloadTimeLabel = new QLabel(tr("00:00"), statsGroup);
    m_remainingTimeLabel = new QLabel(tr("--:--"), statsGroup);
    
    statsLayout->addRow(tr("Download speed:"), m_downloadSpeedLabel);
    statsLayout->addRow(tr("Downloaded:"), m_downloadedSizeLabel);
    statsLayout->addRow(tr("Elapsed time:"), m_downloadTimeLabel);
    statsLayout->addRow(tr("Remaining time:"), m_remainingTimeLabel);
    
    rightLayout->addWidget(statsGroup);
    
    // Disk space info
    QGroupBox* diskGroup = new QGroupBox(tr("Disk Space"), rightWidget);
    QFormLayout* diskLayout = new QFormLayout(diskGroup);
    
    m_diskSpaceLabel = new QLabel(diskGroup);
    diskLayout->addRow(tr("Available space:"), m_diskSpaceLabel);
    
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
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(true);
    m_statusLabel = new QLabel(tr("Ready"), this);
    
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(progressGroup);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_downloadButton = new QPushButton(tr("Download"), this);
    m_downloadButton->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    m_downloadAllButton = new QPushButton(tr("Download All"), this);
    m_pauseButton = new QPushButton(tr("Pause"), this);
    m_pauseButton->setEnabled(false);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setEnabled(false);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_deleteButton->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_settingsButton = new QPushButton(tr("Settings"), this);
    QPushButton* closeButton = new QPushButton(tr("Close"), this);
    
    buttonLayout->addWidget(m_downloadButton);
    buttonLayout->addWidget(m_downloadAllButton);
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_settingsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect close button
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    // Update timer for download stats
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &ModelDownloader::updateDownloadStats);
    
    updateDiskSpace();
}

void ModelDownloader::connectSignals()
{
    connect(m_downloadButton, &QPushButton::clicked, this, &ModelDownloader::downloadSelectedModel);
    connect(m_downloadAllButton, &QPushButton::clicked, this, &ModelDownloader::downloadAllModels);
    connect(m_pauseButton, &QPushButton::clicked, this, &ModelDownloader::pauseDownload);
    connect(m_cancelButton, &QPushButton::clicked, this, &ModelDownloader::cancelDownload);
    connect(m_deleteButton, &QPushButton::clicked, this, &ModelDownloader::deleteSelectedModel);
    connect(m_refreshButton, &QPushButton::clicked, this, &ModelDownloader::refreshModelList);
    connect(m_settingsButton, &QPushButton::clicked, this, &ModelDownloader::showDownloadSettings);
    
    connect(m_modelTable, &QTableWidget::itemSelectionChanged,
            this, &ModelDownloader::onModelSelectionChanged);
    connect(m_modelTable, &QTableWidget::cellDoubleClicked,
            this, &ModelDownloader::onModelDoubleClicked);
}

void ModelDownloader::loadAvailableModels()
{
    // Clear existing models
    m_models.clear();
    m_modelTable->setRowCount(0);
    
    // Define available models
    m_models.append({
        "tiny", "ggml-tiny.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.bin",
        39000000, // 39 MB
        tr("Smallest model, fastest processing. Good for quick transcriptions with moderate accuracy."),
        0.60f, 1.0f, false
    });
    
    m_models.append({
        "tiny.en", "ggml-tiny.en.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-tiny.en.bin",
        39000000, // 39 MB
        tr("English-only tiny model. Faster and more accurate for English."),
        0.65f, 1.0f, false
    });
    
    m_models.append({
        "base", "ggml-base.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.bin",
        74000000, // 74 MB
        tr("Base model with better accuracy than tiny. Good balance of speed and quality."),
        0.70f, 0.8f, false
    });
    
    m_models.append({
        "base.en", "ggml-base.en.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin",
        74000000, // 74 MB
        tr("English-only base model. Better accuracy for English transcriptions."),
        0.75f, 0.8f, false
    });
    
    m_models.append({
        "small", "ggml-small.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin",
        244000000, // 244 MB
        tr("Small model with good accuracy. Suitable for most use cases."),
        0.80f, 0.6f, false
    });
    
    m_models.append({
        "small.en", "ggml-small.en.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.en.bin",
        244000000, // 244 MB
        tr("English-only small model. Excellent accuracy for English."),
        0.85f, 0.6f, false
    });
    
    m_models.append({
        "medium", "ggml-medium.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin",
        769000000, // 769 MB
        tr("Medium model with high accuracy. Good for professional use."),
        0.90f, 0.4f, false
    });
    
    m_models.append({
        "medium.en", "ggml-medium.en.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.en.bin",
        769000000, // 769 MB
        tr("English-only medium model. Very high accuracy for English."),
        0.92f, 0.4f, false
    });
    
    m_models.append({
        "large-v1", "ggml-large-v1.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v1.bin",
        1550000000, // 1.55 GB
        tr("Large v1 model with excellent accuracy. Requires significant processing power."),
        0.95f, 0.2f, false
    });
    
    m_models.append({
        "large-v2", "ggml-large-v2.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v2.bin",
        1550000000, // 1.55 GB
        tr("Large v2 model. Latest version with best accuracy. Very slow processing."),
        0.97f, 0.15f, false
    });
    
    m_models.append({
        "large-v3", "ggml-large-v3.bin",
        "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin",
        1550000000, // 1.55 GB
        tr("Large v3 model. State-of-the-art accuracy. Requires high-end hardware."),
        0.98f, 0.1f, false
    });
    
    // Add models to table
    for (int i = 0; i < m_models.size(); ++i) {
        addModelToTable(i);
    }
}

void ModelDownloader::addModelToTable(int modelIndex)
{
    const ModelInfo& model = m_models[modelIndex];
    
    int row = m_modelTable->rowCount();
    m_modelTable->insertRow(row);
    
    // Model name
    QTableWidgetItem* nameItem = new QTableWidgetItem(model.name);
    nameItem->setData(Qt::UserRole, modelIndex);
    m_modelTable->setItem(row, 0, nameItem);
    
    // Size
    QString sizeStr = formatSize(model.size);
    m_modelTable->setItem(row, 1, new QTableWidgetItem(sizeStr));
    
    // Accuracy
    m_modelTable->setItem(row, 2, new QTableWidgetItem(QString::number(model.accuracy * 100, 'f', 0) + "%"));
    
    // Speed
    m_modelTable->setItem(row, 3, new QTableWidgetItem(QString::number(model.speed * 100, 'f', 0) + "%"));
    
    // Status
    QString status = model.isDownloaded ? tr("Downloaded") : tr("Not Downloaded");
    QTableWidgetItem* statusItem = new QTableWidgetItem(status);
    if (model.isDownloaded) {
        statusItem->setForeground(Qt::darkGreen);
    }
    m_modelTable->setItem(row, 4, statusItem);
    
    // Progress (will be used for download progress)
    QProgressBar* progressBar = new QProgressBar();
    progressBar->setVisible(false);
    m_modelTable->setCellWidget(row, 5, progressBar);
}

void ModelDownloader::checkInstalledModels()
{
    Settings& settings = Settings::instance();
    QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();
    
    if (modelsPath.isEmpty()) {
        modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
    }
    
    QDir modelsDir(modelsPath);
    
    for (int i = 0; i < m_models.size(); ++i) {
        QString modelPath = modelsDir.filePath(m_models[i].fileName);
        m_models[i].isDownloaded = QFile::exists(modelPath);
        
        // Update table
        for (int row = 0; row < m_modelTable->rowCount(); ++row) {
            int modelIndex = m_modelTable->item(row, 0)->data(Qt::UserRole).toInt();
            if (modelIndex == i) {
                QString status = m_models[i].isDownloaded ? tr("Downloaded") : tr("Not Downloaded");
                QTableWidgetItem* statusItem = m_modelTable->item(row, 4);
                statusItem->setText(status);
                statusItem->setForeground(m_models[i].isDownloaded ? Qt::darkGreen : Qt::black);
                break;
            }
        }
    }
    
    updateButtonStates();
}

void ModelDownloader::downloadSelectedModel()
{
    int currentRow = m_modelTable->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    int modelIndex = m_modelTable->item(currentRow, 0)->data(Qt::UserRole).toInt();
    startDownload(modelIndex);
}

void ModelDownloader::downloadAllModels()
{
    m_downloadQueue.clear();
    
    for (int i = 0; i < m_models.size(); ++i) {
        if (!m_models[i].isDownloaded) {
            m_downloadQueue.enqueue(i);
        }
    }
    
    if (!m_downloadQueue.isEmpty()) {
        processDownloadQueue();
    }
}

void ModelDownloader::pauseDownload()
{
    if (m_currentDownload) {
        // TODO: Implement pause functionality
        m_isPaused = !m_isPaused;
        m_pauseButton->setText(m_isPaused ? tr("Resume") : tr("Pause"));
    }
}

void ModelDownloader::cancelDownload()
{
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_statusLabel->setText(tr("Download cancelled"));
        Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", "Download cancelled by user");
    }
}

void ModelDownloader::deleteSelectedModel()
{
    int currentRow = m_modelTable->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    int modelIndex = m_modelTable->item(currentRow, 0)->data(Qt::UserRole).toInt();
    const ModelInfo& model = m_models[modelIndex];
    
    if (!model.isDownloaded) {
        return;
    }
    
    int ret = QMessageBox::question(this, tr("Delete Model"),
                                   tr("Are you sure you want to delete the %1 model?").arg(model.name),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        Settings& settings = Settings::instance();
        QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();
        
        if (modelsPath.isEmpty()) {
            modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
        }
        
        QString modelPath = QDir(modelsPath).filePath(model.fileName);
        
        if (QFile::remove(modelPath)) {
            m_models[modelIndex].isDownloaded = false;
            checkInstalledModels();
            QMessageBox::information(this, tr("Model Deleted"),
                                   tr("Model %1 has been deleted successfully.").arg(model.name));
            Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", 
                                 QString("Deleted model: %1").arg(model.name).toStdString());
        } else {
            QMessageBox::critical(this, tr("Delete Failed"),
                                tr("Failed to delete model %1.").arg(model.name));
        }
    }
}

void ModelDownloader::refreshModelList()
{
    loadAvailableModels();
    checkInstalledModels();
    updateDiskSpace();
}

void ModelDownloader::showDownloadSettings()
{
    // TODO: Show download settings dialog
    QMessageBox::information(this, tr("Download Settings"), 
                           tr("Download settings dialog coming soon!"));
}

void ModelDownloader::startDownload(int modelIndex)
{
    if (modelIndex < 0 || modelIndex >= m_models.size()) {
        return;
    }
    
    if (m_models[modelIndex].isDownloaded) {
        QMessageBox::information(this, tr("Already Downloaded"),
                               tr("Model %1 is already downloaded.").arg(m_models[modelIndex].name));
        return;
    }
    
    if (!m_networkManager) {
        m_networkManager = new QNetworkAccessManager(this);
    }
    
    m_currentModelIndex = modelIndex;
    const ModelInfo& model = m_models[modelIndex];
    
    // Check disk space
    qint64 availableSpace = QStorageInfo(QDir::currentPath()).bytesAvailable();
    if (availableSpace < model.size * 1.2) { // 20% buffer
        QMessageBox::critical(this, tr("Insufficient Disk Space"),
                            tr("Not enough disk space to download this model.\n"
                               "Required: %1\nAvailable: %2")
                            .arg(formatSize(model.size))
                            .arg(formatSize(availableSpace)));
        return;
    }
    
    m_statusLabel->setText(tr("Downloading %1 model...").arg(model.name));
    m_progressBar->setValue(0);
    
    // Update UI
    m_downloadButton->setEnabled(false);
    m_downloadAllButton->setEnabled(false);
    m_pauseButton->setEnabled(true);
    m_cancelButton->setEnabled(true);
    
    // Show progress bar for this model
    for (int row = 0; row < m_modelTable->rowCount(); ++row) {
        if (m_modelTable->item(row, 0)->data(Qt::UserRole).toInt() == modelIndex) {
            QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_modelTable->cellWidget(row, 5));
            if (progressBar) {
                progressBar->setVisible(true);
                progressBar->setValue(0);
            }
            break;
        }
    }
    
    // Reset download stats
    m_downloadStartTime = QTime::currentTime();
    m_lastBytesReceived = 0;
    m_updateTimer->start();
    
    // Create request
    QNetworkRequest request(model.url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    
    m_currentDownload = m_networkManager->get(request);
    
    // Connect signals
    connect(m_currentDownload, &QNetworkReply::downloadProgress,
            this, &ModelDownloader::updateDownloadProgress);
    connect(m_currentDownload, &QNetworkReply::finished,
            this, &ModelDownloader::downloadFinished);
    connect(m_currentDownload, &QNetworkReply::readyRead,
            this, &ModelDownloader::downloadReadyRead);
    
    // Prepare output file
    Settings& settings = Settings::instance();
    QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();
    
    if (modelsPath.isEmpty()) {
        modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
    }
    
    QDir().mkpath(modelsPath);
    QString outputPath = QDir(modelsPath).filePath(model.fileName + ".tmp");
    
    m_outputFile = new QFile(outputPath);
    if (!m_outputFile->open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("File Error"),
                            tr("Cannot create output file: %1").arg(m_outputFile->errorString()));
        cancelDownload();
        return;
    }
    
    Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", 
                         QString("Started downloading model: %1").arg(model.name).toStdString());
}

void ModelDownloader::updateDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
        m_progressBar->setValue(progress);
        
        // Update model-specific progress bar
        for (int row = 0; row < m_modelTable->rowCount(); ++row) {
            if (m_modelTable->item(row, 0)->data(Qt::UserRole).toInt() == m_currentModelIndex) {
                QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_modelTable->cellWidget(row, 5));
                if (progressBar) {
                    progressBar->setValue(progress);
                }
                break;
            }
        }
        
        m_downloadedSizeLabel->setText(QString("%1 / %2")
            .arg(formatSize(bytesReceived))
            .arg(formatSize(bytesTotal)));
    }
}

void ModelDownloader::downloadReadyRead()
{
    if (m_outputFile && m_currentDownload) {
        m_outputFile->write(m_currentDownload->readAll());
    }
}

void ModelDownloader::downloadFinished()
{
    m_updateTimer->stop();
    
    if (m_outputFile) {
        m_outputFile->close();
        delete m_outputFile;
        m_outputFile = nullptr;
    }
    
    if (!m_currentDownload) {
        return;
    }
    
    // Hide progress bar
    for (int row = 0; row < m_modelTable->rowCount(); ++row) {
        if (m_modelTable->item(row, 0)->data(Qt::UserRole).toInt() == m_currentModelIndex) {
            QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_modelTable->cellWidget(row, 5));
            if (progressBar) {
                progressBar->setVisible(false);
            }
            break;
        }
    }
    
    if (m_currentDownload->error() == QNetworkReply::NoError) {
        // Rename temp file to final name
        Settings& settings = Settings::instance();
        QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();

void ModelDownloader::onModelDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    
    int modelIndex = m_modelTable->item(row, 0)->data(Qt::UserRole).toInt();
    if (!m_models[modelIndex].isDownloaded) {
        startDownload(modelIndex);
    }
}

void ModelDownloader::updateButtonStates()
{
    int currentRow = m_modelTable->currentRow();
    bool hasSelection = (currentRow >= 0);
    bool isDownloading = (m_currentDownload != nullptr);
    
    if (hasSelection) {
        int modelIndex = m_modelTable->item(currentRow, 0)->data(Qt::UserRole).toInt();
        bool isDownloaded = m_models[modelIndex].isDownloaded;
        
        m_downloadButton->setEnabled(!isDownloaded && !isDownloading);
        m_deleteButton->setEnabled(isDownloaded && !isDownloading);
    } else {
        m_downloadButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
    }
    
    m_downloadAllButton->setEnabled(!isDownloading);
    m_refreshButton->setEnabled(!isDownloading);
}

void ModelDownloader::updateDownloadStats()
{
    if (!m_currentDownload) {
        return;
    }
    
    // Calculate elapsed time
    QTime elapsed = QTime(0, 0).addMSecs(m_downloadStartTime.msecsTo(QTime::currentTime()));
    m_downloadTimeLabel->setText(elapsed.toString("mm:ss"));
    
    // Calculate download speed
    qint64 bytesReceived = m_currentDownload->bytesAvailable();
    if (m_lastBytesReceived > 0) {
        qint64 bytesPerSecond = bytesReceived - m_lastBytesReceived;
        m_downloadSpeedLabel->setText(formatSize(bytesPerSecond) + "/s");
        
        // Estimate remaining time
        qint64 totalBytes = m_currentDownload->header(QNetworkRequest::ContentLengthHeader).toLongLong();
        if (totalBytes > 0 && bytesPerSecond > 0) {
            qint64 remainingBytes = totalBytes - bytesReceived;
            int remainingSeconds = remainingBytes / bytesPerSecond;
            QTime remaining = QTime(0, 0).addSecs(remainingSeconds);
            m_remainingTimeLabel->setText(remaining.toString("mm:ss"));
        }
    }
    
    m_lastBytesReceived = bytesReceived;
}

void ModelDownloader::updateDiskSpace()
{
    Settings& settings = Settings::instance();
    QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();
    
    if (modelsPath.isEmpty()) {
        modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
    }
    
    QStorageInfo storage(modelsPath);
    qint64 available = storage.bytesAvailable();
    qint64 total = storage.bytesTotal();
    
    m_diskSpaceLabel->setText(QString("%1 / %2")
        .arg(formatSize(available))
        .arg(formatSize(total)));
    
    if (available < 1000000000) { // Less than 1GB
        m_diskSpaceLabel->setStyleSheet("QLabel { color: red; }");
    } else if (available < 5000000000) { // Less than 5GB
        m_diskSpaceLabel->setStyleSheet("QLabel { color: orange; }");
    } else {
        m_diskSpaceLabel->setStyleSheet("");
    }
}

        if (modelsPath.isEmpty()) {
            modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
        }
        
        const ModelInfo& model = m_models[m_currentModelIndex];
        QString tempPath = QDir(modelsPath).filePath(model.fileName + ".tmp");
        QString finalPath = QDir(modelsPath).filePath(model.fileName);
        
        if (QFile::rename(tempPath, finalPath)) {
            m_statusLabel->setText(tr("Download completed: %1").arg(model.name));
            m_models[m_currentModelIndex].isDownloaded = true;
            checkInstalledModels();
            
            Logger::instance().log(Logger::LogLevel::Info, "ModelDownloader", 
                                 QString("Successfully downloaded model: %1").arg(model.name).toStdString());
            
            // Process next in queue if any
            if (!m_downloadQueue.isEmpty()) {
                processDownloadQueue();
            } else {
                QMessageBox::information(this, tr("Download Complete"),
                                       tr("Model %1 has been downloaded successfully.").arg(model.name));
            }
        } else {
            QMessageBox::critical(this, tr("File Error"),
                                tr("Failed to save downloaded file."));
        }
    } else if (m_currentDownload->error() == QNetworkReply::OperationCanceledError) {
        // Download was cancelled
        Settings& settings = Settings::instance();
        QString modelsPath = settings.getSetting(Settings::Key::ModelsPath).toString();
        
        if (modelsPath.isEmpty()) {
            modelsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
        }
        
        const ModelInfo& model = m_models[m_currentModelIndex];
        QString tempPath = QDir(modelsPath).filePath(model.fileName + ".tmp");
        QFile::remove(tempPath);
    } else {
        m_statusLabel->setText(tr("Download failed: %1").arg(m_currentDownload->errorString()));
        
        Logger::instance().log(Logger::LogLevel::Error, "ModelDownloader", 
                             QString("Download failed: %1").arg(m_currentDownload->errorString()).toStdString());
        
        QMessageBox::critical(this, tr("Download Failed"),
                            tr("Failed to download model: %1").arg(m_currentDownload->errorString()));
    }
    
    m_currentDownload->deleteLater();
    m_currentDownload = nullptr;
    m_progressBar->setValue(0);
    
    // Update UI
    m_downloadButton->setEnabled(true);
    m_downloadAllButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    
    updateButtonStates();
    updateDiskSpace();
}

void ModelDownloader::processDownloadQueue()
{
    if (m_downloadQueue.isEmpty()) {
        return;
    }
    
    int modelIndex = m_downloadQueue.dequeue();
    startDownload(modelIndex);
}

void ModelDownloader::onModelSelectionChanged()
{
    int currentRow = m_modelTable->currentRow();
    
    if (currentRow >= 0) {
        int modelIndex = m_modelTable->item(currentRow, 0)->data(Qt::UserRole).toInt();
        const ModelInfo& model = m_models[modelIndex];
        
        // Update model info display
        QString info = QString("<h3>%1</h3>").arg(model.name);
        info += QString("<p>%1</p>").arg(model.description);
        info += QString("<p><b>File:</b> %1</p>").arg(model.fileName);
        info += QString("<p><b>Size:</b> %1</p>").arg(formatSize(model.size));
        info += QString("<p><b>Accuracy:</b> %1%</p>").arg(model.accuracy * 100, 0, 'f', 0);
        info += QString("<p><b>Speed:</b> %1%</p>").arg(model.speed * 100, 0, 'f', 0);
        
        m_modelInfoText->setHtml(info);
    } else {
        m_modelInfoText->clear();
    }
    
    updateButtonStates();
}

QString ModelDownloader::formatSize(qint64 bytes) const
{
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;
    const qint64 GB = MB * 1024;
    
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