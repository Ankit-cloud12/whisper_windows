#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>
#include <QObject> // QObject for potential signals/slots if it becomes more complex

namespace WhisperApp {

// Basic Error Handler Stub
// In a real application, this would likely involve more complex error reporting,
// dialogs, logging integration, etc.

class ErrorHandler : public QObject {
    Q_OBJECT
public:
    enum class ErrorLevel {
        Info,
        Warning,
        Critical,
        Fatal
    };

    // Singleton access if preferred, or pass instance
    static ErrorHandler& instance() {
        static ErrorHandler handler;
        return handler;
    }

    // Report an error
    void handleError(const QString& title,
                     const QString& message,
                     ErrorLevel level = ErrorLevel::Critical,
                     bool showUserDialog = true)
    {
        // For now, just log it or print to console
        // In a real app, this would integrate with Logger and potentially show QMessageBox
        QString fullMessage = QString("[%1] %2: %3")
            .arg(levelToString(level))
            .arg(title)
            .arg(message);

        // Replace with proper logging
        #ifdef QT_DEBUG
        qDebug() << fullMessage;
        #else
        // Could write to a file or use a simpler console output for release if logger isn't ready
        // std::cerr << fullMessage.toStdString() << std::endl;
        #endif

        if (showUserDialog) {
            // In a real GUI app, you'd show a QMessageBox here.
            // This is a stub, so we won't pop UI from here directly to keep tests cleaner.
            // But one might emit a signal to MainWindow to show a message.
            // emit errorOccurred(title, message, level);
        }
    }

    void handleError(const std::exception& e,
                     const QString& context = "Unhandled Exception",
                     ErrorLevel level = ErrorLevel::Fatal)
    {
        handleError(context, QString::fromStdString(e.what()), level);
    }


signals:
    // void errorOccurred(const QString& title, const QString& message, ErrorHandler::ErrorLevel level);

private:
    ErrorHandler(QObject* parent = nullptr) : QObject(parent) {}
    ~ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    QString levelToString(ErrorLevel level) {
        switch(level) {
            case ErrorLevel::Info: return "INFO";
            case ErrorLevel::Warning: return "WARNING";
            case ErrorLevel::Critical: return "CRITICAL";
            case ErrorLevel::Fatal: return "FATAL";
            default: return "UNKNOWN";
        }
    }
};

// Global error handler function (optional convenience)
inline void ReportError(const QString& title, const QString& message,
                        ErrorHandler::ErrorLevel level = ErrorHandler::ErrorLevel::Critical,
                        bool showUserDialog = true) {
    ErrorHandler::instance().handleError(title, message, level, showUserDialog);
}

inline void ReportException(const std::exception& e,
                            const QString& context = "Unhandled Exception",
                            ErrorHandler::ErrorLevel level = ErrorHandler::ErrorLevel::Fatal) {
    ErrorHandler::instance().handleError(e, context, level);
}


} // namespace WhisperApp

#endif // ERRORHANDLER_H
