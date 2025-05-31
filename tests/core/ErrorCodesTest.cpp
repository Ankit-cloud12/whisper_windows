/*
 * ErrorCodesTest.cpp
 * 
 * Unit tests for ErrorCodes and error handling
 */

#include <gtest/gtest.h>
#include "core/ErrorCodes.h"
#include <sstream>

class ErrorCodesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Nothing to set up
    }
    
    void TearDown() override {
        // Nothing to tear down
    }
};

// Test error code to string conversion
TEST_F(ErrorCodesTest, ErrorCodeToString) {
    EXPECT_EQ(errorCodeToString(ErrorCode::Success), "Success");
    EXPECT_EQ(errorCodeToString(ErrorCode::Unknown), "Unknown error");
    EXPECT_EQ(errorCodeToString(ErrorCode::InvalidParameter), "Invalid parameter");
    EXPECT_EQ(errorCodeToString(ErrorCode::FileNotFound), "File not found");
    EXPECT_EQ(errorCodeToString(ErrorCode::AccessDenied), "Access denied");
    EXPECT_EQ(errorCodeToString(ErrorCode::OutOfMemory), "Out of memory");
    EXPECT_EQ(errorCodeToString(ErrorCode::NotImplemented), "Not implemented");
    EXPECT_EQ(errorCodeToString(ErrorCode::InvalidFormat), "Invalid format");
    EXPECT_EQ(errorCodeToString(ErrorCode::OperationCancelled), "Operation cancelled");
    EXPECT_EQ(errorCodeToString(ErrorCode::Timeout), "Timeout");
    
    // Audio errors
    EXPECT_EQ(errorCodeToString(ErrorCode::AudioDeviceNotFound), "Audio device not found");
    EXPECT_EQ(errorCodeToString(ErrorCode::AudioFormatNotSupported), "Audio format not supported");
    EXPECT_EQ(errorCodeToString(ErrorCode::AudioInitializationFailed), "Audio initialization failed");
    EXPECT_EQ(errorCodeToString(ErrorCode::AudioCaptureFailed), "Audio capture failed");
    
    // Model errors
    EXPECT_EQ(errorCodeToString(ErrorCode::ModelNotFound), "Model not found");
    EXPECT_EQ(errorCodeToString(ErrorCode::ModelLoadFailed), "Model load failed");
    EXPECT_EQ(errorCodeToString(ErrorCode::ModelInvalid), "Model invalid");
    
    // Transcription errors
    EXPECT_EQ(errorCodeToString(ErrorCode::TranscriptionFailed), "Transcription failed");
    EXPECT_EQ(errorCodeToString(ErrorCode::LanguageNotSupported), "Language not supported");
    
    // Network errors
    EXPECT_EQ(errorCodeToString(ErrorCode::NetworkError), "Network error");
    EXPECT_EQ(errorCodeToString(ErrorCode::DownloadFailed), "Download failed");
    
    // System errors
    EXPECT_EQ(errorCodeToString(ErrorCode::HotkeyRegistrationFailed), "Hotkey registration failed");
    EXPECT_EQ(errorCodeToString(ErrorCode::ClipboardOperationFailed), "Clipboard operation failed");
    EXPECT_EQ(errorCodeToString(ErrorCode::WindowNotFound), "Window not found");
}

// Test error code to string for invalid codes
TEST_F(ErrorCodesTest, InvalidErrorCodeToString) {
    // Test with an invalid error code
    ErrorCode invalidCode = static_cast<ErrorCode>(9999);
    std::string result = errorCodeToString(invalidCode);
    EXPECT_FALSE(result.empty());
    EXPECT_TRUE(result.find("Unknown error code") != std::string::npos);
}

// Test isSuccess function
TEST_F(ErrorCodesTest, IsSuccess) {
    EXPECT_TRUE(isSuccess(ErrorCode::Success));
    EXPECT_FALSE(isSuccess(ErrorCode::Unknown));
    EXPECT_FALSE(isSuccess(ErrorCode::FileNotFound));
    EXPECT_FALSE(isSuccess(ErrorCode::AudioCaptureFailed));
    EXPECT_FALSE(isSuccess(ErrorCode::ModelNotFound));
}

// Test isError function
TEST_F(ErrorCodesTest, IsError) {
    EXPECT_FALSE(isError(ErrorCode::Success));
    EXPECT_TRUE(isError(ErrorCode::Unknown));
    EXPECT_TRUE(isError(ErrorCode::FileNotFound));
    EXPECT_TRUE(isError(ErrorCode::AudioCaptureFailed));
    EXPECT_TRUE(isError(ErrorCode::ModelNotFound));
}

// Test error code categories
TEST_F(ErrorCodesTest, ErrorCodeCategories) {
    // General errors (0-999)
    EXPECT_EQ(static_cast<int>(ErrorCode::Success), 0);
    EXPECT_LT(static_cast<int>(ErrorCode::Unknown), 1000);
    EXPECT_LT(static_cast<int>(ErrorCode::InvalidParameter), 1000);
    
    // Audio errors (1000-1999)
    EXPECT_GE(static_cast<int>(ErrorCode::AudioDeviceNotFound), 1000);
    EXPECT_LT(static_cast<int>(ErrorCode::AudioDeviceNotFound), 2000);
    EXPECT_GE(static_cast<int>(ErrorCode::AudioCaptureFailed), 1000);
    EXPECT_LT(static_cast<int>(ErrorCode::AudioCaptureFailed), 2000);
    
    // Model errors (2000-2999)
    EXPECT_GE(static_cast<int>(ErrorCode::ModelNotFound), 2000);
    EXPECT_LT(static_cast<int>(ErrorCode::ModelNotFound), 3000);
    EXPECT_GE(static_cast<int>(ErrorCode::ModelLoadFailed), 2000);
    EXPECT_LT(static_cast<int>(ErrorCode::ModelLoadFailed), 3000);
    
    // Transcription errors (3000-3999)
    EXPECT_GE(static_cast<int>(ErrorCode::TranscriptionFailed), 3000);
    EXPECT_LT(static_cast<int>(ErrorCode::TranscriptionFailed), 4000);
    EXPECT_GE(static_cast<int>(ErrorCode::LanguageNotSupported), 3000);
    EXPECT_LT(static_cast<int>(ErrorCode::LanguageNotSupported), 4000);
    
    // Network errors (4000-4999)
    EXPECT_GE(static_cast<int>(ErrorCode::NetworkError), 4000);
    EXPECT_LT(static_cast<int>(ErrorCode::NetworkError), 5000);
    EXPECT_GE(static_cast<int>(ErrorCode::DownloadFailed), 4000);
    EXPECT_LT(static_cast<int>(ErrorCode::DownloadFailed), 5000);
    
    // System errors (5000-5999)
    EXPECT_GE(static_cast<int>(ErrorCode::HotkeyRegistrationFailed), 5000);
    EXPECT_LT(static_cast<int>(ErrorCode::HotkeyRegistrationFailed), 6000);
    EXPECT_GE(static_cast<int>(ErrorCode::ClipboardOperationFailed), 5000);
    EXPECT_LT(static_cast<int>(ErrorCode::ClipboardOperationFailed), 6000);
}

// Test error result wrapper
TEST_F(ErrorCodesTest, ErrorResultWrapper) {
    // Test with success
    {
        ErrorResult<int> result(42);
        EXPECT_TRUE(result.isSuccess());
        EXPECT_FALSE(result.isError());
        EXPECT_TRUE(result.hasValue());
        EXPECT_EQ(result.value(), 42);
        EXPECT_EQ(result.code(), ErrorCode::Success);
    }
    
    // Test with error
    {
        ErrorResult<int> result(ErrorCode::FileNotFound);
        EXPECT_FALSE(result.isSuccess());
        EXPECT_TRUE(result.isError());
        EXPECT_FALSE(result.hasValue());
        EXPECT_EQ(result.code(), ErrorCode::FileNotFound);
        EXPECT_THROW(result.value(), std::runtime_error);
    }
    
    // Test with string value
    {
        ErrorResult<std::string> result("Hello");
        EXPECT_TRUE(result.isSuccess());
        EXPECT_EQ(result.value(), "Hello");
    }
    
    // Test move semantics
    {
        ErrorResult<std::string> result1("Test");
        ErrorResult<std::string> result2(std::move(result1));
        EXPECT_EQ(result2.value(), "Test");
    }
}

// Test error handling macros
TEST_F(ErrorCodesTest, ErrorHandlingMacros) {
    auto testFunction = [](bool shouldSucceed) -> ErrorCode {
        if (shouldSucceed) {
            return ErrorCode::Success;
        }
        return ErrorCode::InvalidParameter;
    };
    
    // Test CHECK_ERROR macro
    {
        auto wrapper = [&testFunction]() -> ErrorCode {
            CHECK_ERROR(testFunction(true));
            CHECK_ERROR(testFunction(false)); // This should return early
            return ErrorCode::Success; // Should not reach here
        };
        
        EXPECT_EQ(wrapper(), ErrorCode::InvalidParameter);
    }
    
    // Test RETURN_IF_ERROR macro
    {
        auto wrapper = [&testFunction]() -> ErrorCode {
            RETURN_IF_ERROR(testFunction(true));
            RETURN_IF_ERROR(testFunction(false)); // This should return
            return ErrorCode::Success; // Should not reach here
        };
        
        EXPECT_EQ(wrapper(), ErrorCode::InvalidParameter);
    }
}

// Test error logging functionality
TEST_F(ErrorCodesTest, ErrorLogging) {
    // Capture error log output
    std::stringstream ss;
    auto oldBuf = std::cerr.rdbuf();
    std::cerr.rdbuf(ss.rdbuf());
    
    // Log error
    logError(ErrorCode::ModelNotFound, "Test model");
    
    // Restore cerr
    std::cerr.rdbuf(oldBuf);
    
    // Check output
    std::string output = ss.str();
    EXPECT_TRUE(output.find("Model not found") != std::string::npos);
    EXPECT_TRUE(output.find("Test model") != std::string::npos);
}

// Test chaining error results
TEST_F(ErrorCodesTest, ErrorResultChaining) {
    auto divide = [](int a, int b) -> ErrorResult<int> {
        if (b == 0) {
            return ErrorCode::InvalidParameter;
        }
        return a / b;
    };
    
    auto multiply = [](int a, int b) -> ErrorResult<int> {
        return a * b;
    };
    
    // Test successful chain
    {
        auto result1 = divide(10, 2);
        EXPECT_TRUE(result1.isSuccess());
        
        if (result1.isSuccess()) {
            auto result2 = multiply(result1.value(), 3);
            EXPECT_TRUE(result2.isSuccess());
            EXPECT_EQ(result2.value(), 15);
        }
    }
    
    // Test failed chain
    {
        auto result1 = divide(10, 0);
        EXPECT_FALSE(result1.isSuccess());
        EXPECT_EQ(result1.code(), ErrorCode::InvalidParameter);
    }
}

// Test custom error messages
TEST_F(ErrorCodesTest, CustomErrorMessages) {
    ErrorResult<void> result(ErrorCode::FileNotFound, "config.json");
    EXPECT_FALSE(result.isSuccess());
    EXPECT_EQ(result.code(), ErrorCode::FileNotFound);
    EXPECT_EQ(result.message(), "config.json");
    
    // Test with formatted message
    ErrorResult<void> result2(ErrorCode::InvalidParameter, 
                             "Invalid value: " + std::to_string(42));
    EXPECT_EQ(result2.message(), "Invalid value: 42");
}