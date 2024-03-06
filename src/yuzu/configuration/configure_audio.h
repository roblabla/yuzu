// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureAudio;
}

class ConfigureAudio : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureAudio(QWidget* parent = nullptr);
    ~ConfigureAudio() override;

    void applyConfiguration();
    void retranslateUi();

private:
    void updateAudioDevices(int sink_index);

    void setConfiguration();
    void setOutputSinkFromSinkID();
    void setAudioDeviceFromDeviceID();
    void setVolumeIndicatorText(int percentage);

    std::unique_ptr<Ui::ConfigureAudio> ui;
};
