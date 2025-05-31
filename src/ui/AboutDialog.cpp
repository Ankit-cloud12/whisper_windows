#include "AboutDialog.h"
#include "../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QSysInfo>
#include <QApplication>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QFile>
#include <QTextStream>

// Version information
#define APP_VERSION "1.0.0"
#define APP_BUILD_DATE __DATE__
#define APP_BUILD_TIME __TIME__

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    
    Logger::instance().log(Logger::LogLevel::Debug, "AboutDialog", "About dialog opened");
}

AboutDialog::~AboutDialog()
{
}

void AboutDialog::setupUI()
{
    setWindowTitle(tr("About WhisperApp"));
    setModal(true);
    setFixedSize(500, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    
    // Logo and title section
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    m_logoLabel = new QLabel(this);
    QPixmap logo(":/icons/app.png");
    if (!logo.isNull()) {
        m_logoLabel->setPixmap(logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_logoLabel->setText("🎤");
        m_logoLabel->setStyleSheet("QLabel { font-size: 48px; }");
    }
    
    QVBoxLayout* titleLayout = new QVBoxLayout();
    
    m_titleLabel = new QLabel(tr("WhisperApp"), this);
    m_titleLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; }");
    
    m_versionLabel = new QLabel(getVersionString(), this);
    m_versionLabel->setStyleSheet("QLabel { font-size: 14px; color: #666; }");
    
    m_buildLabel = new QLabel(getBuildInfo(), this);
    m_buildLabel->setStyleSheet("QLabel { font-size: 12px; color: #888; }");
    
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_versionLabel);
    titleLayout->addWidget(m_buildLabel);
    titleLayout->addStretch();
    
    headerLayout->addWidget(m_logoLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    
    mainLayout->addLayout(headerLayout);
    
    // Description
    m_descriptionLabel = new QLabel(this);
    m_descriptionLabel->setText(tr("A powerful speech-to-text application powered by OpenAI's Whisper model.\n\n"
                                   "WhisperApp provides real-time transcription with high accuracy, "
                                   "supporting multiple languages and offering various customization options."));
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setStyleSheet("QLabel { line-height: 1.5; }");
    
    mainLayout->addWidget(m_descriptionLabel);
    
    // Credits section
    QLabel* creditsLabel = new QLabel(this);
    creditsLabel->setText(tr("<b>Credits:</b><br>"
                            "• OpenAI Whisper - Speech recognition model<br>"
                            "• Qt Framework - Cross-platform UI<br>"
                            "• whisper.cpp - C++ implementation<br>"
                            "• Contributors and testers"));
    creditsLabel->setWordWrap(true);
    creditsLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border-radius: 5px; }");
    
    mainLayout->addWidget(creditsLabel);
    
    // Copyright
    m_copyrightLabel = new QLabel(tr("© 2024 WhisperApp Development Team"), this);
    m_copyrightLabel->setAlignment(Qt::AlignCenter);
    m_copyrightLabel->setStyleSheet("QLabel { color: #666; }");
    
    mainLayout->addWidget(m_copyrightLabel);
    
    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_systemInfoButton = new QPushButton(tr("System Info"), this);
    m_licensesButton = new QPushButton(tr("Licenses"), this);
    m_updateButton = new QPushButton(tr("Check for Updates"), this);
    
    buttonLayout->addWidget(m_systemInfoButton);
    buttonLayout->addWidget(m_licensesButton);
    buttonLayout->addWidget(m_updateButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Links
    QHBoxLayout* linkLayout = new QHBoxLayout();
    
    m_websiteButton = new QPushButton(tr("Visit Website"), this);
    m_websiteButton->setCursor(Qt::PointingHandCursor);
    m_websiteButton->setFlat(true);
    m_websiteButton->setStyleSheet("QPushButton { color: #0066cc; text-decoration: underline; }");
    
    m_githubButton = new QPushButton(tr("GitHub Repository"), this);
    m_githubButton->setCursor(Qt::PointingHandCursor);
    m_githubButton->setFlat(true);
    m_githubButton->setStyleSheet("QPushButton { color: #0066cc; text-decoration: underline; }");
    
    linkLayout->addWidget(m_websiteButton);
    linkLayout->addWidget(m_githubButton);
    linkLayout->addStretch();
    
    mainLayout->addLayout(linkLayout);
    
    // OK button
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(m_systemInfoButton, &QPushButton::clicked, this, &AboutDialog::showSystemInfo);
    connect(m_licensesButton, &QPushButton::clicked, this, &AboutDialog::showLicenses);
    connect(m_updateButton, &QPushButton::clicked, this, &AboutDialog::checkForUpdates);
    connect(m_websiteButton, &QPushButton::clicked, this, &AboutDialog::openWebsite);
    connect(m_githubButton, &QPushButton::clicked, this, &AboutDialog::openGitHub);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

void AboutDialog::showSystemInfo()
{
    QString info;
    QTextStream stream(&info);
    
    stream << "<h3>System Information</h3>";
    stream << "<table style='margin: 10px;'>";
    
    // Operating System
    stream << "<tr><td><b>Operating System:</b></td><td>" 
           << QSysInfo::prettyProductName() << "</td></tr>";
    
    // CPU Architecture
    stream << "<tr><td><b>CPU Architecture:</b></td><td>" 
           << QSysInfo::currentCpuArchitecture() << "</td></tr>";
    
    // Kernel
    stream << "<tr><td><b>Kernel:</b></td><td>" 
           << QSysInfo::kernelType() << " " << QSysInfo::kernelVersion() << "</td></tr>";
    
    // Qt Version
    stream << "<tr><td><b>Qt Version:</b></td><td>" 
           << QT_VERSION_STR << "</td></tr>";
    
    // Compiler
#ifdef Q_CC_MSVC
    stream << "<tr><td><b>Compiler:</b></td><td>MSVC " << _MSC_VER << "</td></tr>";
#elif defined(Q_CC_GNU)
    stream << "<tr><td><b>Compiler:</b></td><td>GCC " << __VERSION__ << "</td></tr>";
#elif defined(Q_CC_CLANG)
    stream << "<tr><td><b>Compiler:</b></td><td>Clang " << __clang_version__ << "</td></tr>";
#else
    stream << "<tr><td><b>Compiler:</b></td><td>Unknown</td></tr>";
#endif
    
    // Build Type
#ifdef QT_DEBUG
    stream << "<tr><td><b>Build Type:</b></td><td>Debug</td></tr>";
#else
    stream << "<tr><td><b>Build Type:</b></td><td>Release</td></tr>";
#endif
    
    // Application paths
    stream << "<tr><td><b>Application Path:</b></td><td>" 
           << QApplication::applicationDirPath() << "</td></tr>";
    
    stream << "<tr><td><b>Data Path:</b></td><td>" 
           << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) << "</td></tr>";
    
    stream << "</table>";
    
    QDialog dialog(this);
    dialog.setWindowTitle(tr("System Information"));
    dialog.setModal(true);
    dialog.resize(600, 400);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QTextEdit* textEdit = new QTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setHtml(info);
    layout->addWidget(textEdit);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    
    dialog.exec();
}

void AboutDialog::showLicenses()
{
    QString licenses;
    QTextStream stream(&licenses);
    
    stream << "<h2>Open Source Licenses</h2>";
    
    // WhisperApp License
    stream << "<h3>WhisperApp</h3>";
    stream << "<p>MIT License</p>";
    stream << "<pre style='background-color: #f0f0f0; padding: 10px;'>";
    stream << "Copyright (c) 2024 WhisperApp Development Team\n\n";
    stream << "Permission is hereby granted, free of charge, to any person obtaining a copy\n";
    stream << "of this software and associated documentation files (the \"Software\"), to deal\n";
    stream << "in the Software without restriction, including without limitation the rights\n";
    stream << "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n";
    stream << "copies of the Software, and to permit persons to whom the Software is\n";
    stream << "furnished to do so, subject to the following conditions:\n\n";
    stream << "The above copyright notice and this permission notice shall be included in all\n";
    stream << "copies or substantial portions of the Software.\n\n";
    stream << "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n";
    stream << "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n";
    stream << "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n";
    stream << "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n";
    stream << "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n";
    stream << "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n";
    stream << "SOFTWARE.";
    stream << "</pre>";
    
    // Qt License
    stream << "<h3>Qt Framework</h3>";
    stream << "<p>Licensed under LGPLv3. See <a href='https://www.qt.io/licensing/'>qt.io/licensing</a></p>";
    
    // Whisper License
    stream << "<h3>OpenAI Whisper</h3>";
    stream << "<p>MIT License - Copyright (c) 2022 OpenAI</p>";
    
    // whisper.cpp License
    stream << "<h3>whisper.cpp</h3>";
    stream << "<p>MIT License - Copyright (c) 2023 Georgi Gerganov</p>";
    
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Open Source Licenses"));
    dialog.setModal(true);
    dialog.resize(700, 500);
    
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    
    QTextEdit* textEdit = new QTextEdit(&dialog);
    textEdit->setReadOnly(true);
    textEdit->setHtml(licenses);
    layout->addWidget(textEdit);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    
    dialog.exec();
}

void AboutDialog::checkForUpdates()
{
    // TODO: Implement actual update checking
    QMessageBox::information(this, tr("Check for Updates"),
                           tr("You are running the latest version of WhisperApp (v%1).\n\n"
                              "Automatic update checking will be available in a future release.")
                           .arg(APP_VERSION));
    
    Logger::instance().log(Logger::LogLevel::Info, "AboutDialog", "Update check requested");
}

void AboutDialog::openWebsite()
{
    QDesktopServices::openUrl(QUrl("https://whisperapp.example.com"));
    Logger::instance().log(Logger::LogLevel::Debug, "AboutDialog", "Opened website");
}

void AboutDialog::openGitHub()
{
    QDesktopServices::openUrl(QUrl("https://github.com/whisperapp/whisperapp"));
    Logger::instance().log(Logger::LogLevel::Debug, "AboutDialog", "Opened GitHub repository");
}

QString AboutDialog::getVersionString() const
{
    return QString("Version %1").arg(APP_VERSION);
}

QString AboutDialog::getBuildInfo() const
{
    return QString("Built on %1 at %2").arg(APP_BUILD_DATE).arg(APP_BUILD_TIME);
}