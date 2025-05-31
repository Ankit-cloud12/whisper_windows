/*
 * ModelManagerTest.cpp
 * 
 * Unit tests for ModelManager class
 */

#include <gtest/gtest.h>
#include "core/ModelManager.h"
#include "core/ErrorCodes.h"
#include "../TestUtils.h"
#include <filesystem>
#include <fstream>

class ModelManagerTest : public ::testing::Test {
protected:
    std::string testModelDir;
    ModelManager* modelManager;
    
    void SetUp() override {
        // Create temporary directory for test models
        testModelDir = TestUtils::FileUtils::createTempDirectory();
        
        // Create ModelManager instance
        modelManager = new ModelManager(testModelDir);
    }
    
    void TearDown() override {
        delete modelManager;
        TestUtils::FileUtils::cleanupTempDirectory(testModelDir);
    }
    
    void createMockModelFile(const std::string& modelName) {
        namespace fs = std::filesystem;
        
        std::string modelPath = testModelDir + "/" + modelName;
        std::ofstream file(modelPath, std::ios::binary);
        
        // Write mock model data
        std::vector<uint8_t> mockData(1024, 0xAB); // 1KB of dummy data
        file.write(reinterpret_cast<const char*>(mockData.data()), mockData.size());
        file.close();
    }
};

// Test model discovery
TEST_F(ModelManagerTest, DiscoverModels) {
    // Create mock model files
    createMockModelFile("ggml-tiny.en.bin");
    createMockModelFile("ggml-base.en.bin");
    createMockModelFile("ggml-small.bin");
    createMockModelFile("ggml-medium.bin");
    createMockModelFile("not-a-model.txt"); // Should be ignored
    
    // Scan for models
    auto result = modelManager->scanModels();
    EXPECT_EQ(result, ErrorCode::Success);
    
    // Get available models
    auto models = modelManager->getAvailableModels();
    EXPECT_EQ(models.size(), 4);
    
    // Verify model names
    std::set<std::string> expectedModels = {"tiny.en", "base.en", "small", "medium"};
    for (const auto& model : models) {
        EXPECT_TRUE(expectedModels.find(model.name) != expectedModels.end());
    }
}

// Test model info
TEST_F(ModelManagerTest, GetModelInfo) {
    createMockModelFile("ggml-base.en.bin");
    modelManager->scanModels();
    
    auto info = modelManager->getModelInfo("base.en");
    EXPECT_TRUE(info.has_value());
    EXPECT_EQ(info->name, "base.en");
    EXPECT_TRUE(info->isEnglishOnly);
    EXPECT_FALSE(info->isMultilingual);
    EXPECT_EQ(info->parameters, 74); // Expected for base model
    EXPECT_FALSE(info->path.empty());
    EXPECT_GT(info->size, 0);
}

// Test model validation
TEST_F(ModelManagerTest, ValidateModel) {
    // Test with valid model
    createMockModelFile("ggml-base.bin");
    EXPECT_TRUE(modelManager->isValidModel(testModelDir + "/ggml-base.bin"));
    
    // Test with invalid file
    std::ofstream badFile(testModelDir + "/bad-model.bin");
    badFile << "not a valid model";
    badFile.close();
    EXPECT_FALSE(modelManager->isValidModel(testModelDir + "/bad-model.bin"));
    
    // Test with non-existent file
    EXPECT_FALSE(modelManager->isValidModel(testModelDir + "/nonexistent.bin"));
}

// Test model paths
TEST_F(ModelManagerTest, ModelPaths) {
    createMockModelFile("ggml-small.bin");
    modelManager->scanModels();
    
    // Test getting model path
    auto path = modelManager->getModelPath("small");
    EXPECT_TRUE(path.has_value());
    EXPECT_TRUE(std::filesystem::exists(*path));
    EXPECT_TRUE(path->find("ggml-small.bin") != std::string::npos);
    
    // Test non-existent model
    auto noPath = modelManager->getModelPath("large");
    EXPECT_FALSE(noPath.has_value());
}

// Test default model
TEST_F(ModelManagerTest, DefaultModel) {
    // No models available
    EXPECT_TRUE(modelManager->getDefaultModel().empty());
    
    // Create models
    createMockModelFile("ggml-base.en.bin");
    createMockModelFile("ggml-small.bin");
    modelManager->scanModels();
    
    // Should prefer base.en as default
    EXPECT_EQ(modelManager->getDefaultModel(), "base.en");
}

// Test model size information
TEST_F(ModelManagerTest, ModelSizes) {
    // Create models with different sizes
    namespace fs = std::filesystem;
    
    // Tiny model - ~39MB
    std::ofstream tiny(testModelDir + "/ggml-tiny.bin", std::ios::binary);
    std::vector<uint8_t> tinyData(39 * 1024 * 1024, 0);
    tiny.write(reinterpret_cast<const char*>(tinyData.data()), tinyData.size());
    tiny.close();
    
    // Base model - ~142MB
    std::ofstream base(testModelDir + "/ggml-base.bin", std::ios::binary);
    std::vector<uint8_t> baseData(142 * 1024 * 1024, 0);
    base.write(reinterpret_cast<const char*>(baseData.data()), baseData.size());
    base.close();
    
    modelManager->scanModels();
    
    auto tinyInfo = modelManager->getModelInfo("tiny");
    EXPECT_TRUE(tinyInfo.has_value());
    EXPECT_EQ(tinyInfo->parameters, 39);
    EXPECT_NEAR(tinyInfo->size, 39 * 1024 * 1024, 1024);
    
    auto baseInfo = modelManager->getModelInfo("base");
    EXPECT_TRUE(baseInfo.has_value());
    EXPECT_EQ(baseInfo->parameters, 74);
    EXPECT_NEAR(baseInfo->size, 142 * 1024 * 1024, 1024);
}

// Test required models check
TEST_F(ModelManagerTest, RequiredModelsCheck) {
    // No models - should return false
    EXPECT_FALSE(modelManager->hasRequiredModels());
    
    // Create base model
    createMockModelFile("ggml-base.en.bin");
    modelManager->scanModels();
    
    // Should have required models now
    EXPECT_TRUE(modelManager->hasRequiredModels());
}

// Test model directory management
TEST_F(ModelManagerTest, DirectoryManagement) {
    // Test getting models directory
    EXPECT_EQ(modelManager->getModelsDirectory(), testModelDir);
    
    // Test setting new directory
    std::string newDir = TestUtils::FileUtils::createTempDirectory();
    modelManager->setModelsDirectory(newDir);
    EXPECT_EQ(modelManager->getModelsDirectory(), newDir);
    
    // Create model in new directory
    std::ofstream file(newDir + "/ggml-tiny.bin");
    file << "test";
    file.close();
    
    modelManager->scanModels();
    auto models = modelManager->getAvailableModels();
    EXPECT_EQ(models.size(), 1);
    
    TestUtils::FileUtils::cleanupTempDirectory(newDir);
}

// Test model format detection
TEST_F(ModelManagerTest, ModelFormatDetection) {
    // Create different format models
    createMockModelFile("ggml-base.bin");      // GGML format
    createMockModelFile("base.pt");            // PyTorch format (not supported)
    createMockModelFile("whisper-base.onnx");  // ONNX format (not supported)
    
    modelManager->scanModels();
    
    // Should only detect GGML format
    auto models = modelManager->getAvailableModels();
    EXPECT_EQ(models.size(), 1);
    EXPECT_EQ(models[0].name, "base");
}

// Test concurrent model scanning
TEST_F(ModelManagerTest, ConcurrentScanning) {
    // Create multiple models
    for (int i = 0; i < 5; ++i) {
        createMockModelFile("ggml-model" + std::to_string(i) + ".bin");
    }
    
    // Scan from multiple threads
    const int numThreads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> successCount(0);
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, &successCount]() {
            if (modelManager->scanModels() == ErrorCode::Success) {
                successCount++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successCount, numThreads);
    EXPECT_EQ(modelManager->getAvailableModels().size(), 5);
}

// Test model naming conventions
TEST_F(ModelManagerTest, ModelNamingConventions) {
    // Test various naming patterns
    createMockModelFile("ggml-tiny.en.bin");
    createMockModelFile("ggml-base.en.bin");
    createMockModelFile("ggml-small.en.bin");
    createMockModelFile("ggml-medium.en.bin");
    createMockModelFile("ggml-large-v1.bin");
    createMockModelFile("ggml-large-v2.bin");
    createMockModelFile("ggml-large-v3.bin");
    
    modelManager->scanModels();
    auto models = modelManager->getAvailableModels();
    
    // Verify all models detected
    EXPECT_EQ(models.size(), 7);
    
    // Check specific models
    std::set<std::string> modelNames;
    for (const auto& model : models) {
        modelNames.insert(model.name);
    }
    
    EXPECT_TRUE(modelNames.find("tiny.en") != modelNames.end());
    EXPECT_TRUE(modelNames.find("base.en") != modelNames.end());
    EXPECT_TRUE(modelNames.find("small.en") != modelNames.end());
    EXPECT_TRUE(modelNames.find("medium.en") != modelNames.end());
    EXPECT_TRUE(modelNames.find("large-v1") != modelNames.end());
    EXPECT_TRUE(modelNames.find("large-v2") != modelNames.end());
    EXPECT_TRUE(modelNames.find("large-v3") != modelNames.end());
}