﻿#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <utility>
#include <QFileDialog>

#include <QDirIterator>
#include "common/common_types.h"
#include "common/file_util.h"
#include "core/settings.h"
#include "ui_configure_ui.h"
#include "yuzu/configuration/configure_ui.h"
#include "yuzu/uisettings.h"

namespace {
constexpr std::array default_icon_sizes{
    std::make_pair(0, QT_TR_NOOP("无")),
    std::make_pair(32, QT_TR_NOOP("小 (32x32)")),
    std::make_pair(64, QT_TR_NOOP("标准 (64x64)")),
    std::make_pair(128, QT_TR_NOOP("大 (128x128)")),
    std::make_pair(256, QT_TR_NOOP("最大 (256x256)")),
};

constexpr std::array row_text_names{
    QT_TR_NOOP("文件名称"),   QT_TR_NOOP("文件类型"), QT_TR_NOOP("游戏 ID"),
    QT_TR_NOOP("游戏名字"), QT_TR_NOOP("无"),
};
} // Anonymous namespace

ConfigureUi::ConfigureUi(QWidget* parent) : QWidget(parent), ui(new Ui::ConfigureUi) {
    ui->setupUi(this);

    InitializeLanguageComboBox();

    for (const auto& theme : UISettings::themes) {
        ui->theme_combobox->addItem(QString::fromUtf8(theme.first),
                                    QString::fromUtf8(theme.second));
    }

    InitializeIconSizeComboBox();
    InitializeRowComboBoxes();

    SetConfiguration();

    // Force game list reload if any of the relevant settings are changed.
    connect(ui->icon_size_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_1_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);
    connect(ui->row_2_text_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::RequestGameListUpdate);

    // Update text ComboBoxes after user interaction.
    connect(ui->row_1_text_combobox, QOverload<int>::of(&QComboBox::activated),
            [=]() { ConfigureUi::UpdateSecondRowComboBox(); });
    connect(ui->row_2_text_combobox, QOverload<int>::of(&QComboBox::activated),
            [=]() { ConfigureUi::UpdateFirstRowComboBox(); });

    // Set screenshot path to user specification.
    connect(ui->screenshot_path_button, &QToolButton::pressed, this, [this] {
        const QString& filename =
            QFileDialog::getExistingDirectory(
                this, tr("选择屏幕截图路径..."),
                QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::ScreenshotsDir))) +
            QDir::separator();
        if (!filename.isEmpty()) {
            ui->screenshot_path_edit->setText(filename);
        }
    });
}

ConfigureUi::~ConfigureUi() = default;

void ConfigureUi::ApplyConfiguration() {
    UISettings::values.theme =
        ui->theme_combobox->itemData(ui->theme_combobox->currentIndex()).toString();
    UISettings::values.show_add_ons = ui->show_add_ons->isChecked();
    UISettings::values.icon_size = ui->icon_size_combobox->currentData().toUInt();
    UISettings::values.row_1_text_id = ui->row_1_text_combobox->currentData().toUInt();
    UISettings::values.row_2_text_id = ui->row_2_text_combobox->currentData().toUInt();

    UISettings::values.enable_screenshot_save_as = ui->enable_screenshot_save_as->isChecked();
    FileUtil::GetUserPath(FileUtil::UserPath::ScreenshotsDir,
                          ui->screenshot_path_edit->text().toStdString());
    Settings::Apply();
}

void ConfigureUi::RequestGameListUpdate() {
    UISettings::values.is_game_list_reload_pending.exchange(true);
}

void ConfigureUi::SetConfiguration() {
    ui->theme_combobox->setCurrentIndex(ui->theme_combobox->findData(UISettings::values.theme));
    ui->language_combobox->setCurrentIndex(
        ui->language_combobox->findData(UISettings::values.language));
    ui->show_add_ons->setChecked(UISettings::values.show_add_ons);
    ui->icon_size_combobox->setCurrentIndex(
        ui->icon_size_combobox->findData(UISettings::values.icon_size));

    ui->enable_screenshot_save_as->setChecked(UISettings::values.enable_screenshot_save_as);
    ui->screenshot_path_edit->setText(
        QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::ScreenshotsDir)));
}

void ConfigureUi::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        RetranslateUI();
    }

    QWidget::changeEvent(event);
}

void ConfigureUi::RetranslateUI() {
    ui->retranslateUi(this);

    for (int i = 0; i < ui->icon_size_combobox->count(); i++) {
        ui->icon_size_combobox->setItemText(i, tr(default_icon_sizes[i].second));
    }

    for (int i = 0; i < ui->row_1_text_combobox->count(); i++) {
        const QString name = tr(row_text_names[i]);

        ui->row_1_text_combobox->setItemText(i, name);
        ui->row_2_text_combobox->setItemText(i, name);
    }
}

void ConfigureUi::InitializeLanguageComboBox() {
    ui->language_combobox->addItem(tr("<系统>"), QString{});
    ui->language_combobox->addItem(tr("汉化语言"), QStringLiteral("en"));
    QDirIterator it(QStringLiteral(":/languages"), QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString locale = it.next();
        locale.truncate(locale.lastIndexOf(QLatin1Char{'.'}));
        locale.remove(0, locale.lastIndexOf(QLatin1Char{'/'}) + 1);
        const QString lang = QLocale::languageToString(QLocale(locale).language());
        ui->language_combobox->addItem(lang, locale);
    }

    // Unlike other configuration changes, interface language changes need to be reflected on the
    // interface immediately. This is done by passing a signal to the main window, and then
    // retranslating when passing back.
    connect(ui->language_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ConfigureUi::OnLanguageChanged);
}

void ConfigureUi::InitializeIconSizeComboBox() {
    for (const auto& size : default_icon_sizes) {
        ui->icon_size_combobox->addItem(QString::fromUtf8(size.second), size.first);
    }
}

void ConfigureUi::InitializeRowComboBoxes() {
    UpdateFirstRowComboBox(true);
    UpdateSecondRowComboBox(true);
}

void ConfigureUi::UpdateFirstRowComboBox(bool init) {
    const int currentIndex =
        init ? UISettings::values.row_1_text_id
             : ui->row_1_text_combobox->findData(ui->row_1_text_combobox->currentData());

    ui->row_1_text_combobox->clear();

    for (std::size_t i = 0; i < row_text_names.size(); i++) {
        const QString row_text_name = QString::fromUtf8(row_text_names[i]);
        ui->row_1_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
    }

    ui->row_1_text_combobox->setCurrentIndex(ui->row_1_text_combobox->findData(currentIndex));

    ui->row_1_text_combobox->removeItem(4); // None
    ui->row_1_text_combobox->removeItem(
        ui->row_1_text_combobox->findData(ui->row_2_text_combobox->currentData()));
}

void ConfigureUi::UpdateSecondRowComboBox(bool init) {
    const int currentIndex =
        init ? UISettings::values.row_2_text_id
             : ui->row_2_text_combobox->findData(ui->row_2_text_combobox->currentData());

    ui->row_2_text_combobox->clear();

    for (std::size_t i = 0; i < row_text_names.size(); ++i) {
        const QString row_text_name = QString::fromUtf8(row_text_names[i]);
        ui->row_2_text_combobox->addItem(row_text_name, QVariant::fromValue(i));
    }

    ui->row_2_text_combobox->setCurrentIndex(ui->row_2_text_combobox->findData(currentIndex));

    ui->row_2_text_combobox->removeItem(
        ui->row_2_text_combobox->findData(ui->row_1_text_combobox->currentData()));
}

void ConfigureUi::OnLanguageChanged(int index) {
    if (index == -1)
        return;

    emit LanguageChanged(ui->language_combobox->itemData(index).toString());
}
