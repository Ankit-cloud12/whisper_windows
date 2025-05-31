#pragma once

#include <QDialog>

// Forward declarations
class QLabel;
class QPushButton;

/**
 * @brief About dialog showing application information
 * 
 * Displays version information, credits, licenses, and system info.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AboutDialog(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~AboutDialog();

private slots:
    /**
     * @brief Show system information
     */
    void showSystemInfo();
    
    /**
     * @brief Show licenses dialog
     */
    void showLicenses();
    
    /**
     * @brief Check for updates
     */
    void checkForUpdates();
    
    /**
     * @brief Open project website
     */
    void openWebsite();
    
    /**
     * @brief Open GitHub repository
     */
    void openGitHub();

private:
    /**
     * @brief Set up the user interface
     */
    void setupUI();
    
    /**
     * @brief Get application version string
     * @return Version string
     */
    QString getVersionString() const;
    
    /**
     * @brief Get build information
     * @return Build info string
     */
    QString getBuildInfo() const;

private:
    // UI elements
    QLabel* m_logoLabel;
    QLabel* m_titleLabel;
    QLabel* m_versionLabel;
    QLabel* m_buildLabel;
    QLabel* m_copyrightLabel;
    QLabel* m_descriptionLabel;
    QPushButton* m_systemInfoButton;
    QPushButton* m_licensesButton;
    QPushButton* m_updateButton;
    QPushButton* m_websiteButton;
    QPushButton* m_githubButton;
};