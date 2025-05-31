/*
 * ErrorCodes.h
 * 
 * Comprehensive error handling system for WhisperApp.
 * Defines error codes, error messages, and exception classes.
 */

#ifndef ERRORCODES_H
#define ERRORCODES_H

#include <string>
#include <stdexcept>
#include <unordered_map>

namespace WhisperApp {

/**
 * @brief Error codes for WhisperApp operations
 */
enum class ErrorCode {
    // Success
    Success = 0,
    
    // General errors (1-99)
    UnknownError = 1,
    NotImplemented = 2,
    InvalidArgument = 3,
    OutOfMemory = 4,
    OperationCancelled = 5,
    
    // Model errors (100-199)
    ModelNotFound = 100,
    ModelLoadFailed = 101,
    ModelNotLoaded = 102,
    ModelCorrupted = 103,
    ModelVersionMismatch = 104,
    ModelDownloadFailed = 105,
    
    // Audio errors (200-299)
    AudioFormatUnsupported = 200,
    AudioDataEmpty = 201,
    AudioConversionFailed = 202,
    AudioSampleRateInvalid = 203,
    AudioChannelCountInvalid = 204,
    AudioDurationTooLong = 205,
    AudioDurationTooShort = 206,
    
    // Transcription errors (300-399)
    TranscriptionFailed = 300,
    TranscriptionTimeout = 301,
    TranscriptionInProgress = 302,
    TranscriptionCancelled = 303,
    TranscriptionLanguageUnsupported = 304,
    
    // File system errors (400-499)
    FileNotFound = 400,
    FileAccessDenied = 401,
    FileWriteFailed = 402,
    DirectoryNotFound = 403,
    DiskSpaceInsufficient = 404,
    
    // Network errors (500-599)
    NetworkConnectionFailed = 500,
    NetworkTimeout = 501,
    NetworkSSLError = 502,
    NetworkProxyError = 503,
    
    // Configuration errors (600-699)
    ConfigurationInvalid = 600,
    ConfigurationMissing = 601,
    ConfigurationCorrupted = 602,
    
    // System errors (700-799)
    SystemResourceUnavailable = 700,
    SystemPermissionDenied = 701,
    SystemGPUNotAvailable = 702,
    SystemCUDAError = 703,
    SystemThreadCreationFailed = 704,
    
    // UI errors (800-899)
    UIInitializationFailed = 800,
    UIComponentNotFound = 801,
    UIEventHandlingError = 802
};

/**
 * @brief Get human-readable error message for an error code
 */
inline std::string getErrorMessage(ErrorCode code) {
    static const std::unordered_map<ErrorCode, std::string> errorMessages = {
        // Success
        {ErrorCode::Success, "Operation completed successfully"},
        
        // General errors
        {ErrorCode::UnknownError, "An unexpected error occurred. Please try again or restart the application."},
        {ErrorCode::NotImplemented, "This feature is not yet available. It will be implemented in a future update."},
        {ErrorCode::InvalidArgument, "Invalid input provided. Please check your settings and try again."},
        {ErrorCode::OutOfMemory, "Not enough memory available. Please close other applications and try again."},
        {ErrorCode::OperationCancelled, "The operation was cancelled by user request."},
        
        // Model errors
        {ErrorCode::ModelNotFound, "The AI model file could not be found. Please download it from the Model Manager."},
        {ErrorCode::ModelLoadFailed, "Failed to load the AI model. Please ensure you have enough memory and the model file is not corrupted."},
        {ErrorCode::ModelNotLoaded, "No AI model is currently loaded. Please select a model from the dropdown menu."},
        {ErrorCode::ModelCorrupted, "The AI model file appears to be corrupted. Please re-download it from the Model Manager."},
        {ErrorCode::ModelVersionMismatch, "This model version is not compatible with the current version of WhisperApp. Please update the model."},
        {ErrorCode::ModelDownloadFailed, "Failed to download the AI model. Please check your internet connection and try again."},
        
        // Audio errors
        {ErrorCode::AudioFormatUnsupported, "The audio format is not supported. WhisperApp supports WAV, MP3, and common audio formats."},
        {ErrorCode::AudioDataEmpty, "No audio data was recorded. Please ensure your microphone is working and try again."},
        {ErrorCode::AudioConversionFailed, "Failed to process the audio. Please try recording again."},
        {ErrorCode::AudioSampleRateInvalid, "The audio sample rate is not supported. Please check your audio device settings."},
        {ErrorCode::AudioChannelCountInvalid, "The audio channel configuration is not supported. Please use mono or stereo audio."},
        {ErrorCode::AudioDurationTooLong, "The recording is too long. Please keep recordings under 30 minutes for best results."},
        {ErrorCode::AudioDurationTooShort, "The recording is too short. Please record at least 1 second of audio."},
        
        // Transcription errors
        {ErrorCode::TranscriptionFailed, "Failed to transcribe the audio. Please try again with a clearer recording."},
        {ErrorCode::TranscriptionTimeout, "Transcription took too long and was stopped. Try using a smaller model or shorter audio."},
        {ErrorCode::TranscriptionInProgress, "A transcription is already in progress. Please wait for it to complete."},
        {ErrorCode::TranscriptionCancelled, "The transcription was cancelled. You can start a new recording whenever you're ready."},
        {ErrorCode::TranscriptionLanguageUnsupported, "The selected language is not supported. Please choose a different language."},
        
        // File system errors
        {ErrorCode::FileNotFound, "The requested file could not be found. Please check the file path and try again."},
        {ErrorCode::FileAccessDenied, "Access to the file was denied. Please check file permissions."},
        {ErrorCode::FileWriteFailed, "Failed to save the file. Please check that you have write permissions and enough disk space."},
        {ErrorCode::DirectoryNotFound, "The specified directory does not exist. Please create it or choose a different location."},
        {ErrorCode::DiskSpaceInsufficient, "Not enough disk space available. Please free up some space and try again."},
        
        // Network errors
        {ErrorCode::NetworkConnectionFailed, "Could not connect to the internet. Please check your network connection."},
        {ErrorCode::NetworkTimeout, "The network request timed out. Please check your internet connection and try again."},
        {ErrorCode::NetworkSSLError, "A secure connection could not be established. Please check your system date and time."},
        {ErrorCode::NetworkProxyError, "Could not connect through the proxy. Please check your proxy settings."},
        
        // Configuration errors
        {ErrorCode::ConfigurationInvalid, "The configuration file contains invalid settings. Please reset to defaults in Settings."},
        {ErrorCode::ConfigurationMissing, "The configuration file is missing. Default settings will be used."},
        {ErrorCode::ConfigurationCorrupted, "The configuration file is corrupted. Please reset to defaults in Settings."},
        
        // System errors
        {ErrorCode::SystemResourceUnavailable, "A required system resource is not available. Please restart the application."},
        {ErrorCode::SystemPermissionDenied, "Permission denied by the system. Please run the application as administrator."},
        {ErrorCode::SystemGPUNotAvailable, "GPU acceleration is not available. The application will use CPU mode instead."},
        {ErrorCode::SystemCUDAError, "GPU processing failed. Switching to CPU mode. Check your graphics drivers."},
        {ErrorCode::SystemThreadCreationFailed, "Failed to create processing thread. Please restart the application."},
        
        // UI errors
        {ErrorCode::UIInitializationFailed, "Failed to initialize the user interface. Please restart the application."},
        {ErrorCode::UIComponentNotFound, "A required UI component could not be loaded. Please reinstall the application."},
        {ErrorCode::UIEventHandlingError, "An error occurred while processing UI events. Please restart the application."}
    };
    
    auto it = errorMessages.find(code);
    if (it != errorMessages.end()) {
        return it->second;
    }
    return "Unknown error code: " + std::to_string(static_cast<int>(code));
}

/**
 * @brief Get user-friendly error message with suggestions
 */
inline std::string getUserFriendlyError(ErrorCode code, const std::string& technicalDetails = "") {
    std::string message = getErrorMessage(code);
    
    // Add specific suggestions based on error type
    switch (code) {
        case ErrorCode::ModelNotFound:
        case ErrorCode::ModelDownloadFailed:
            message += "\n\nSuggestion: Go to Tools > Model Manager to download the required model.";
            break;
            
        case ErrorCode::AudioDataEmpty:
        case ErrorCode::AudioSampleRateInvalid:
        case ErrorCode::AudioChannelCountInvalid:
            message += "\n\nSuggestion: Check your microphone in Settings > Audio Devices.";
            break;
            
        case ErrorCode::NetworkConnectionFailed:
        case ErrorCode::NetworkTimeout:
            message += "\n\nSuggestion: Check your internet connection and firewall settings.";
            break;
            
        case ErrorCode::DiskSpaceInsufficient:
            message += "\n\nSuggestion: Free up at least 1GB of disk space for model files.";
            break;
            
        case ErrorCode::SystemGPUNotAvailable:
        case ErrorCode::SystemCUDAError:
            message += "\n\nNote: CPU mode will be used, which may be slower but still functional.";
            break;
            
        default:
            break;
    }
    
    // Add technical details if available (for advanced users)
    if (!technicalDetails.empty()) {
        message += "\n\nTechnical details: " + technicalDetails;
    }
    
    return message;
}

/**
 * @brief Base exception class for WhisperApp
 */
class WhisperException : public std::runtime_error {
private:
    ErrorCode errorCode_;
    std::string details_;
    
public:
    WhisperException(ErrorCode code, const std::string& details = "")
        : std::runtime_error(createMessage(code, details))
        , errorCode_(code)
        , details_(details) {}
    
    ErrorCode getErrorCode() const { return errorCode_; }
    const std::string& getDetails() const { return details_; }
    
private:
    static std::string createMessage(ErrorCode code, const std::string& details) {
        std::string msg = getErrorMessage(code);
        if (!details.empty()) {
            msg += ": " + details;
        }
        return msg;
    }
};

/**
 * @brief Model-related exceptions
 */
class ModelException : public WhisperException {
public:
    ModelException(ErrorCode code, const std::string& details = "")
        : WhisperException(code, details) {}
};

/**
 * @brief Audio-related exceptions
 */
class AudioException : public WhisperException {
public:
    AudioException(ErrorCode code, const std::string& details = "")
        : WhisperException(code, details) {}
};

/**
 * @brief Transcription-related exceptions
 */
class TranscriptionException : public WhisperException {
public:
    TranscriptionException(ErrorCode code, const std::string& details = "")
        : WhisperException(code, details) {}
};

/**
 * @brief Network-related exceptions
 */
class NetworkException : public WhisperException {
public:
    NetworkException(ErrorCode code, const std::string& details = "")
        : WhisperException(code, details) {}
};

/**
 * @brief System-related exceptions
 */
class SystemException : public WhisperException {
public:
    SystemException(ErrorCode code, const std::string& details = "")
        : WhisperException(code, details) {}
};

} // namespace WhisperApp

#endif // ERRORCODES_H