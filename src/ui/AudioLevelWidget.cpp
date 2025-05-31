/*
 * AudioLevelWidget.cpp
 * 
 * Implementation of real-time audio level visualization
 */

#include "AudioLevelWidget.h"
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>
#include <algorithm>

// Constants
constexpr float MIN_DB = -60.0f;
constexpr float MAX_DB = 0.0f;
constexpr int GRID_LINES = 6;

AudioLevelWidget::AudioLevelWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(20);
    setMinimumWidth(100);
    
    // Start animation timer
    timer_id_ = startTimer(config_.update_rate_ms);
    
    // Start timers
    peak_timer_.start();
    frame_timer_.start();
}

AudioLevelWidget::~AudioLevelWidget() {
    if (timer_id_ != 0) {
        killTimer(timer_id_);
    }
}

void AudioLevelWidget::setConfig(const AudioLevelConfig& config) {
    config_ = config;
    cache_valid_ = false;
    update();
}

AudioLevelConfig AudioLevelWidget::config() const {
    return config_;
}

void AudioLevelWidget::setLevel(float level) {
    level = std::max(0.0f, std::min(1.0f, level));
    current_level_ = level;
    
    // Update peak
    if (level > peak_level_) {
        peak_level_ = level;
        peak_timer_.restart();
    }
    
    // Check for clipping
    float db = linearToDb(level);
    if (db >= config_.clipping_threshold_db) {
        clipping_ = true;
        emit clippingDetected();
    }
    
    emit levelChanged(level);
}

float AudioLevelWidget::level() const {
    return current_level_;
}

void AudioLevelWidget::setLevelDb(float db) {
    setLevel(dbToLinear(db));
}

float AudioLevelWidget::levelDb() const {
    return linearToDb(current_level_);
}

void AudioLevelWidget::addWaveformSamples(const float* samples, size_t count) {
    std::lock_guard<std::mutex> lock(waveform_mutex_);
    
    for (size_t i = 0; i < count; ++i) {
        waveform_buffer_.push_back(samples[i]);
        
        // Keep buffer size limited
        if (waveform_buffer_.size() > config_.waveform_samples) {
            waveform_buffer_.pop_front();
        }
    }
}

void AudioLevelWidget::clearWaveform() {
    std::lock_guard<std::mutex> lock(waveform_mutex_);
    waveform_buffer_.clear();
}

void AudioLevelWidget::setMode(VisualizationMode mode) {
    if (config_.mode != mode) {
        config_.mode = mode;
        cache_valid_ = false;
        update();
        emit modeChanged(mode);
    }
}

VisualizationMode AudioLevelWidget::mode() const {
    return config_.mode;
}

void AudioLevelWidget::resetPeak() {
    peak_level_ = 0.0f;
    peak_timer_.restart();
}

bool AudioLevelWidget::isClipping() const {
    return clipping_;
}

void AudioLevelWidget::resetClipping() {
    clipping_ = false;
}

void AudioLevelWidget::setEnabled(bool enable) {
    if (enable && timer_id_ == 0) {
        timer_id_ = startTimer(config_.update_rate_ms);
    } else if (!enable && timer_id_ != 0) {
        killTimer(timer_id_);
        timer_id_ = 0;
    }
}

void AudioLevelWidget::setChannels(int channels) {
    channels_ = std::max(1, std::min(2, channels));
    cache_valid_ = false;
    update();
}

int AudioLevelWidget::channels() const {
    return channels_;
}

void AudioLevelWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    painter.fillRect(rect(), config_.background_color);
    
    // Draw based on mode
    switch (config_.mode) {
        case VisualizationMode::LEVEL_METER:
            drawLevelMeter(painter);
            break;
        case VisualizationMode::WAVEFORM:
            drawWaveform(painter);
            break;
        case VisualizationMode::COMBINED:
            // Split view
            painter.save();
            painter.setClipRect(0, 0, width(), height() / 2);
            drawLevelMeter(painter);
            painter.restore();
            
            painter.save();
            painter.setClipRect(0, height() / 2, width(), height() / 2);
            painter.translate(0, height() / 2);
            drawWaveform(painter);
            painter.restore();
            break;
        default:
            break;
    }
    
    // Draw clipping indicator
    if (config_.show_clipping_indicator && clipping_) {
        drawClippingIndicator(painter);
    }
}

void AudioLevelWidget::resizeEvent(QResizeEvent* event) {
    cache_valid_ = false;
    QWidget::resizeEvent(event);
}

void AudioLevelWidget::timerEvent(QTimerEvent* event) {
    if (event->timerId() == timer_id_) {
        updateSmoothing();
        updatePeakHold();
        update();
    }
}

void AudioLevelWidget::drawLevelMeter(QPainter& painter) {
    const int margin = 2;
    const int meter_height = height() - 2 * margin;
    const int meter_width = width() - 2 * margin;
    
    if (channels_ == 1) {
        // Single channel - horizontal meter
        QRect meter_rect(margin, margin, meter_width, meter_height);
        
        // Draw background
        painter.fillRect(meter_rect, Qt::black);
        
        // Draw level
        float db = linearToDb(smoothed_level_);
        float normalized = (db - config_.min_db) / (config_.max_db - config_.min_db);
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        
        int level_width = static_cast<int>(meter_width * normalized);
        
        if (config_.meter_style == MeterStyle::GRADIENT) {
            // Create gradient
            QLinearGradient gradient(meter_rect.left(), 0, meter_rect.right(), 0);
            gradient.setColorAt(0.0, config_.normal_color);
            gradient.setColorAt(0.7, config_.normal_color);
            gradient.setColorAt(0.85, config_.warning_color);
            gradient.setColorAt(0.95, config_.danger_color);
            gradient.setColorAt(1.0, config_.danger_color);
            
            painter.fillRect(meter_rect.left(), meter_rect.top(), 
                           level_width, meter_rect.height(), gradient);
        } else if (config_.meter_style == MeterStyle::SEGMENTED) {
            // Draw segments
            int segment_width = meter_width / config_.meter_segments;
            int filled_segments = static_cast<int>(config_.meter_segments * normalized);
            
            for (int i = 0; i < filled_segments; ++i) {
                float segment_db = config_.min_db + 
                    (i / float(config_.meter_segments)) * (config_.max_db - config_.min_db);
                QColor color = getLevelColor(segment_db);
                
                QRect segment(meter_rect.left() + i * segment_width,
                            meter_rect.top(),
                            segment_width - 1,
                            meter_rect.height());
                painter.fillRect(segment, color);
            }
        }
        
        // Draw peak hold
        if (config_.show_peak_hold && peak_level_ > 0) {
            float peak_db = linearToDb(peak_level_);
            float peak_normalized = (peak_db - config_.min_db) / (config_.max_db - config_.min_db);
            peak_normalized = std::max(0.0f, std::min(1.0f, peak_normalized));
            
            int peak_pos = meter_rect.left() + static_cast<int>(meter_width * peak_normalized);
            painter.setPen(QPen(config_.peak_color, 2));
            painter.drawLine(peak_pos, meter_rect.top(), peak_pos, meter_rect.bottom());
        }
        
        // Draw scale
        drawScale(painter);
    } else {
        // Stereo - vertical meters
        int channel_width = (meter_width - margin) / 2;
        
        for (int ch = 0; ch < channels_; ++ch) {
            QRect meter_rect(margin + ch * (channel_width + margin), 
                           margin, channel_width, meter_height);
            
            // Similar drawing logic for vertical meters
            // (Implementation would be similar but with vertical orientation)
        }
    }
}

void AudioLevelWidget::drawWaveform(QPainter& painter) {
    std::lock_guard<std::mutex> lock(waveform_mutex_);
    
    if (waveform_buffer_.empty()) {
        return;
    }
    
    const int margin = 2;
    const int wave_height = height() - 2 * margin;
    const int wave_width = width() - 2 * margin;
    const int center_y = height() / 2;
    
    // Draw center line
    painter.setPen(QPen(config_.grid_color, 1, Qt::DotLine));
    painter.drawLine(margin, center_y, width() - margin, center_y);
    
    // Draw waveform
    painter.setPen(QPen(config_.normal_color, 1));
    
    if (waveform_buffer_.size() > 1) {
        QPainterPath path;
        
        float x_scale = float(wave_width) / (waveform_buffer_.size() - 1);
        float y_scale = wave_height / 2.0f;
        
        bool first = true;
        int i = 0;
        for (float sample : waveform_buffer_) {
            float x = margin + i * x_scale;
            float y = center_y - sample * y_scale;
            
            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
            i++;
        }
        
        if (config_.waveform_fill) {
            // Fill under the waveform
            path.lineTo(width() - margin, center_y);
            path.lineTo(margin, center_y);
            path.closeSubpath();
            
            QColor fill_color = config_.normal_color;
            fill_color.setAlpha(50);
            painter.fillPath(path, fill_color);
        }
        
        painter.drawPath(path);
    }
}

void AudioLevelWidget::drawGrid(QPainter& painter) {
    painter.setPen(QPen(config_.grid_color, 1, Qt::DotLine));
    
    // Draw vertical grid lines (dB markers)
    for (int i = 0; i <= GRID_LINES; ++i) {
        float db = config_.min_db + (i / float(GRID_LINES)) * (config_.max_db - config_.min_db);
        float normalized = (db - config_.min_db) / (config_.max_db - config_.min_db);
        int x = static_cast<int>(width() * normalized);
        
        painter.drawLine(x, 0, x, height());
    }
}

void AudioLevelWidget::drawScale(QPainter& painter) {
    painter.setPen(config_.text_color);
    painter.setFont(QFont("Arial", 8));
    
    // Draw dB scale at bottom
    int y = height() - 2;
    
    for (int i = 0; i <= GRID_LINES; ++i) {
        float db = config_.min_db + (i / float(GRID_LINES)) * (config_.max_db - config_.min_db);
        float normalized = (db - config_.min_db) / (config_.max_db - config_.min_db);
        int x = static_cast<int>(width() * normalized);
        
        QString text = QString::number(static_cast<int>(db));
        QRect text_rect(x - 20, y - 15, 40, 15);
        painter.drawText(text_rect, Qt::AlignCenter, text);
    }
}

void AudioLevelWidget::drawClippingIndicator(QPainter& painter) {
    // Draw red border
    painter.setPen(QPen(Qt::red, 3));
    painter.drawRect(rect().adjusted(1, 1, -1, -1));
    
    // Draw "CLIP" text
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(Qt::red);
    painter.drawText(rect(), Qt::AlignTop | Qt::AlignRight, "CLIP");
}

void AudioLevelWidget::updatePeakHold() {
    if (config_.show_peak_hold && peak_timer_.elapsed() > config_.peak_hold_time_ms) {
        // Gradually decrease peak
        peak_level_ = peak_level_ * 0.95f;
        
        if (peak_level_ < 0.001f) {
            peak_level_ = 0.0f;
        }
    }
}

void AudioLevelWidget::updateSmoothing() {
    float target = current_level_;
    float current = smoothed_level_;
    
    // Calculate time delta
    float dt = frame_timer_.restart() / 1000.0f;
    
    // Apply attack/release
    float rate = (target > current) ? config_.attack_time : config_.release_time;
    float factor = std::exp(-dt / rate);
    
    smoothed_level_ = target + (current - target) * factor;
}

float AudioLevelWidget::linearToDb(float linear) const {
    if (linear <= 0.0f) {
        return MIN_DB;
    }
    return 20.0f * std::log10(linear);
}

float AudioLevelWidget::dbToLinear(float db) const {
    return std::pow(10.0f, db / 20.0f);
}

QColor AudioLevelWidget::getLevelColor(float db) const {
    if (db >= config_.danger_db) {
        return config_.danger_color;
    } else if (db >= config_.warning_db) {
        return config_.warning_color;
    } else {
        return config_.normal_color;
    }
}

// MultiChannelAudioLevelWidget implementation
MultiChannelAudioLevelWidget::MultiChannelAudioLevelWidget(int channels, QWidget* parent)
    : QWidget(parent) {
    
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(0, 0, 0, 0);
    
    for (int i = 0; i < channels; ++i) {
        auto* channel_layout = new QHBoxLayout();
        channel_layout->setSpacing(5);
        
        // Channel label
        auto* label = new QLabel(QString("CH %1").arg(i + 1));
        label->setFixedWidth(40);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        channel_labels_.push_back(label);
        channel_layout->addWidget(label);
        
        // Level widget
        auto* level_widget = new AudioLevelWidget();
        level_widget->setChannels(1);
        level_widget->setFixedHeight(20);
        channel_widgets_.push_back(level_widget);
        channel_layout->addWidget(level_widget, 1);
        
        layout->addLayout(channel_layout);
    }
}

void MultiChannelAudioLevelWidget::setChannelLevel(int channel, float level) {
    if (channel >= 0 && channel < static_cast<int>(channel_widgets_.size())) {
        channel_widgets_[channel]->setLevel(level);
    }
}

void MultiChannelAudioLevelWidget::setLevels(const float* levels, int count) {
    int max_channels = std::min(count, static_cast<int>(channel_widgets_.size()));
    for (int i = 0; i < max_channels; ++i) {
        channel_widgets_[i]->setLevel(levels[i]);
    }
}

void MultiChannelAudioLevelWidget::setConfig(const AudioLevelConfig& config) {
    for (auto* widget : channel_widgets_) {
        widget->setConfig(config);
    }
}

AudioLevelWidget* MultiChannelAudioLevelWidget::channelWidget(int channel) const {
    if (channel >= 0 && channel < static_cast<int>(channel_widgets_.size())) {
        return channel_widgets_[channel];
    }
    return nullptr;
}

void MultiChannelAudioLevelWidget::setChannelLabels(const QStringList& labels) {
    int max_labels = std::min(labels.size(), static_cast<int>(channel_labels_.size()));
    for (int i = 0; i < max_labels; ++i) {
        channel_labels_[i]->setText(labels[i]);
    }
}