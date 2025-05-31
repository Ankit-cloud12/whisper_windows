/*
 * AudioLevelWidget.h
 * 
 * Real-time audio level visualization widget
 */

#ifndef AUDIOLEVELWIDGET_H
#define AUDIOLEVELWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>

/**
 * @brief Audio level visualization modes
 */
enum class VisualizationMode {
    LEVEL_METER,    // Classic VU meter style
    WAVEFORM,       // Oscilloscope-style waveform
    SPECTRUM,       // Simple frequency spectrum (future)
    COMBINED        // Multiple visualizations
};

/**
 * @brief Audio level meter style
 */
enum class MeterStyle {
    CLASSIC,        // Traditional analog VU meter
    LED,            // LED bar graph style
    GRADIENT,       // Smooth gradient
    SEGMENTED       // Segmented bar
};

/**
 * @brief Configuration for audio level widget
 */
struct AudioLevelConfig {
    // General settings
    VisualizationMode mode = VisualizationMode::LEVEL_METER;
    bool show_peak_hold = true;
    int peak_hold_time_ms = 2000;
    bool show_clipping_indicator = true;
    int clipping_threshold_db = -0.1;
    
    // Level meter settings
    MeterStyle meter_style = MeterStyle::GRADIENT;
    float min_db = -60.0f;
    float max_db = 0.0f;
    float warning_db = -6.0f;
    float danger_db = -3.0f;
    int meter_segments = 20;
    
    // Waveform settings
    int waveform_samples = 512;
    bool waveform_fill = false;
    bool waveform_centered = true;
    
    // Colors
    QColor background_color{40, 40, 40};
    QColor normal_color{0, 200, 0};
    QColor warning_color{255, 200, 0};
    QColor danger_color{255, 50, 0};
    QColor peak_color{255, 255, 255};
    QColor grid_color{80, 80, 80};
    QColor text_color{200, 200, 200};
    
    // Animation
    float attack_time = 0.01f;    // Fast attack
    float release_time = 0.3f;    // Slower release
    int update_rate_ms = 33;      // ~30 FPS
};

/**
 * @brief Real-time audio level visualization widget
 */
class AudioLevelWidget : public QWidget {
    Q_OBJECT
    
    Q_PROPERTY(float level READ level WRITE setLevel NOTIFY levelChanged)
    Q_PROPERTY(VisualizationMode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool clipping READ isClipping NOTIFY clippingDetected)
    
public:
    explicit AudioLevelWidget(QWidget* parent = nullptr);
    ~AudioLevelWidget();
    
    /**
     * @brief Set configuration
     * @param config Widget configuration
     */
    void setConfig(const AudioLevelConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    AudioLevelConfig config() const;
    
    /**
     * @brief Set current audio level
     * @param level Audio level (0.0 to 1.0)
     */
    void setLevel(float level);
    
    /**
     * @brief Get current audio level
     * @return Current level (0.0 to 1.0)
     */
    float level() const;
    
    /**
     * @brief Set audio level in decibels
     * @param db Level in decibels
     */
    void setLevelDb(float db);
    
    /**
     * @brief Get current level in decibels
     * @return Level in decibels
     */
    float levelDb() const;
    
    /**
     * @brief Add waveform samples
     * @param samples Audio samples
     * @param count Number of samples
     */
    void addWaveformSamples(const float* samples, size_t count);
    
    /**
     * @brief Clear waveform display
     */
    void clearWaveform();
    
    /**
     * @brief Set visualization mode
     * @param mode New visualization mode
     */
    void setMode(VisualizationMode mode);
    
    /**
     * @brief Get current visualization mode
     * @return Current mode
     */
    VisualizationMode mode() const;
    
    /**
     * @brief Reset peak hold
     */
    void resetPeak();
    
    /**
     * @brief Check if clipping is detected
     * @return true if clipping
     */
    bool isClipping() const;
    
    /**
     * @brief Reset clipping indicator
     */
    void resetClipping();
    
    /**
     * @brief Start/stop visualization
     * @param enable true to start, false to stop
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Set number of channels to display
     * @param channels Number of channels (1 or 2)
     */
    void setChannels(int channels);
    
    /**
     * @brief Get number of channels
     * @return Number of channels
     */
    int channels() const;
    
signals:
    /**
     * @brief Emitted when level changes
     * @param level New level (0.0 to 1.0)
     */
    void levelChanged(float level);
    
    /**
     * @brief Emitted when clipping is detected
     */
    void clippingDetected();
    
    /**
     * @brief Emitted when mode changes
     * @param mode New mode
     */
    void modeChanged(VisualizationMode mode);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    
private:
    void drawLevelMeter(QPainter& painter);
    void drawWaveform(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawScale(QPainter& painter);
    void drawClippingIndicator(QPainter& painter);
    void updatePeakHold();
    void updateSmoothing();
    
    float linearToDb(float linear) const;
    float dbToLinear(float db) const;
    QColor getLevelColor(float db) const;
    
private:
    // Configuration
    AudioLevelConfig config_;
    
    // Current state
    std::atomic<float> current_level_{0.0f};
    std::atomic<float> smoothed_level_{0.0f};
    std::atomic<float> peak_level_{0.0f};
    std::atomic<bool> clipping_{false};
    int channels_ = 1;
    
    // Waveform data
    mutable std::mutex waveform_mutex_;
    std::deque<float> waveform_buffer_;
    
    // Peak hold
    QElapsedTimer peak_timer_;
    
    // Animation
    int timer_id_ = 0;
    QElapsedTimer frame_timer_;
    
    // Drawing cache
    QPixmap background_cache_;
    bool cache_valid_ = false;
};

/**
 * @brief Multi-channel audio level widget
 */
class MultiChannelAudioLevelWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit MultiChannelAudioLevelWidget(int channels = 2, QWidget* parent = nullptr);
    
    /**
     * @brief Set level for specific channel
     * @param channel Channel index (0-based)
     * @param level Audio level (0.0 to 1.0)
     */
    void setChannelLevel(int channel, float level);
    
    /**
     * @brief Set levels for all channels
     * @param levels Array of levels
     * @param count Number of channels
     */
    void setLevels(const float* levels, int count);
    
    /**
     * @brief Set configuration for all channels
     * @param config Configuration to apply
     */
    void setConfig(const AudioLevelConfig& config);
    
    /**
     * @brief Get level widget for specific channel
     * @param channel Channel index
     * @return Level widget pointer
     */
    AudioLevelWidget* channelWidget(int channel) const;
    
    /**
     * @brief Set channel labels
     * @param labels List of channel labels
     */
    void setChannelLabels(const QStringList& labels);
    
private:
    std::vector<AudioLevelWidget*> channel_widgets_;
    std::vector<QLabel*> channel_labels_;
};

#endif // AUDIOLEVELWIDGET_H