﻿// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <memory>

#include <QDialog>
#include <QKeyEvent>

#include "ui_configure_input.h"

class QPushButton;
class QString;
class QTimer;

namespace Ui {
class ConfigureInput;
}

void OnDockedModeChanged(bool last_state, bool new_state);

class ConfigureInput : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureInput(QWidget* parent = nullptr);
    ~ConfigureInput() override;

    /// Save all button configurations to settings file
    void applyConfiguration();

private:
    void updateUIEnabled();

    /// Load configuration settings.
    void loadConfiguration();
    /// Restore all buttons to their default values.
    void restoreDefaults();

    std::unique_ptr<Ui::ConfigureInput> ui;

    std::array<QComboBox*, 8> players_controller;
    std::array<QPushButton*, 8> players_configure;
};
