// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <regex>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QThreadPool>
#include <fmt/format.h>
#include "common/common_paths.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/file_sys/patch_manager.h"
#include "yuzu/compatibility_list.h"
#include "yuzu/game_list.h"
#include "yuzu/game_list_p.h"
#include "yuzu/game_list_worker.h"
#include "yuzu/main.h"
#include "yuzu/ui_settings.h"

GameListSearchField::KeyReleaseEater::KeyReleaseEater(GameList* gamelist) : gamelist{gamelist} {}

// EventFilter in order to process systemkeys while editing the searchfield
bool GameListSearchField::KeyReleaseEater::eventFilter(QObject* obj, QEvent* event) {
    // If it isn't a KeyRelease event then continue with standard event processing
    if (event->type() != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);

    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    int rowCount = gamelist->tree_view->model()->rowCount();
    QString edit_filter_text = gamelist->search_field->edit_filter->text().toLower();

    // If the searchfield's text hasn't changed special function keys get checked
    // If no function key changes the searchfield's text the filter doesn't need to get reloaded
    if (edit_filter_text == edit_filter_text_old) {
        switch (keyEvent->key()) {
        // Escape: Resets the searchfield
        case Qt::Key_Escape: {
            if (edit_filter_text_old.isEmpty()) {
                return QObject::eventFilter(obj, event);
            } else {
                gamelist->search_field->edit_filter->clear();
                edit_filter_text = "";
            }
            break;
        }
        // Return and Enter
        // If the enter key gets pressed first checks how many and which entry is visible
        // If there is only one result launch this game
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            QStandardItemModel* item_model = new QStandardItemModel(gamelist->tree_view);
            QModelIndex root_index = item_model->invisibleRootItem()->index();
            QStandardItem* child_file;
            QString file_path;
            int resultCount = 0;
            for (int i = 0; i < rowCount; ++i) {
                if (!gamelist->tree_view->isRowHidden(i, root_index)) {
                    ++resultCount;
                    child_file = gamelist->item_model->item(i, 0);
                    file_path = child_file->data(GameListItemPath::FullPathRole).toString();
                }
            }
            if (resultCount == 1) {
                // To avoid loading error dialog loops while confirming them using enter
                // Also users usually want to run a diffrent game after closing one
                gamelist->search_field->edit_filter->setText("");
                edit_filter_text = "";
                emit gamelist->GameChosen(file_path);
            } else {
                return QObject::eventFilter(obj, event);
            }
            break;
        }
        default:
            return QObject::eventFilter(obj, event);
        }
    }
    edit_filter_text_old = edit_filter_text;
    return QObject::eventFilter(obj, event);
}

void GameListSearchField::setFilterResult(int visible, int total) {
    label_filter_result->setText(tr("%1 of %n result(s)", "", total).arg(visible));
}

void GameListSearchField::clear() {
    edit_filter->setText("");
}

void GameListSearchField::setFocus() {
    if (edit_filter->isVisible()) {
        edit_filter->setFocus();
    }
}

GameListSearchField::GameListSearchField(GameList* parent) : QWidget{parent} {
    KeyReleaseEater* keyReleaseEater = new KeyReleaseEater(parent);
    layout_filter = new QHBoxLayout;
    layout_filter->setMargin(8);
    label_filter = new QLabel;
    label_filter->setText(tr("Filter:"));
    edit_filter = new QLineEdit;
    edit_filter->setText("");
    edit_filter->setPlaceholderText(tr("Enter pattern to filter"));
    edit_filter->installEventFilter(keyReleaseEater);
    edit_filter->setClearButtonEnabled(true);
    connect(edit_filter, &QLineEdit::textChanged, parent, &GameList::onTextChanged);
    label_filter_result = new QLabel;
    button_filter_close = new QToolButton(this);
    button_filter_close->setText("X");
    button_filter_close->setCursor(Qt::ArrowCursor);
    button_filter_close->setStyleSheet("QToolButton{ border: none; padding: 0px; color: "
                                       "#000000; font-weight: bold; background: #F0F0F0; }"
                                       "QToolButton:hover{ border: none; padding: 0px; color: "
                                       "#EEEEEE; font-weight: bold; background: #E81123}");
    connect(button_filter_close, &QToolButton::clicked, parent, &GameList::onFilterCloseClicked);
    layout_filter->setSpacing(10);
    layout_filter->addWidget(label_filter);
    layout_filter->addWidget(edit_filter);
    layout_filter->addWidget(label_filter_result);
    layout_filter->addWidget(button_filter_close);
    setLayout(layout_filter);
}

/**
 * Checks if all words separated by spaces are contained in another string
 * This offers a word order insensitive search function
 *
 * @param haystack String that gets checked if it contains all words of the userinput string
 * @param userinput String containing all words getting checked
 * @return true if the haystack contains all words of userinput
 */
static bool ContainsAllWords(const QString& haystack, const QString& userinput) {
    const QStringList userinput_split =
        userinput.split(' ', QString::SplitBehavior::SkipEmptyParts);

    return std::all_of(userinput_split.begin(), userinput_split.end(),
                       [&haystack](const QString& s) { return haystack.contains(s); });
}

// Event in order to filter the gamelist after editing the searchfield
void GameList::onTextChanged(const QString& newText) {
    int rowCount = tree_view->model()->rowCount();
    QString edit_filter_text = newText.toLower();

    QModelIndex root_index = item_model->invisibleRootItem()->index();

    // If the searchfield is empty every item is visible
    // Otherwise the filter gets applied
    if (edit_filter_text.isEmpty()) {
        for (int i = 0; i < rowCount; ++i) {
            tree_view->setRowHidden(i, root_index, false);
        }
        search_field->setFilterResult(rowCount, rowCount);
    } else {
        int result_count = 0;
        for (int i = 0; i < rowCount; ++i) {
            const QStandardItem* child_file = item_model->item(i, 0);
            const QString file_path =
                child_file->data(GameListItemPath::FullPathRole).toString().toLower();
            QString file_name = file_path.mid(file_path.lastIndexOf('/') + 1);
            const QString file_title =
                child_file->data(GameListItemPath::TitleRole).toString().toLower();
            const QString file_programmid =
                child_file->data(GameListItemPath::ProgramIdRole).toString().toLower();

            // Only items which filename in combination with its title contains all words
            // that are in the searchfield will be visible in the gamelist
            // The search is case insensitive because of toLower()
            // I decided not to use Qt::CaseInsensitive in containsAllWords to prevent
            // multiple conversions of edit_filter_text for each game in the gamelist
            if (ContainsAllWords(file_name.append(' ').append(file_title), edit_filter_text) ||
                (file_programmid.count() == 16 && edit_filter_text.contains(file_programmid))) {
                tree_view->setRowHidden(i, root_index, false);
                ++result_count;
            } else {
                tree_view->setRowHidden(i, root_index, true);
            }
            search_field->setFilterResult(result_count, rowCount);
        }
    }
}

void GameList::onFilterCloseClicked() {
    main_window->filterBarSetChecked(false);
}

GameList::GameList(FileSys::VirtualFilesystem vfs, GMainWindow* parent)
    : QWidget{parent}, vfs(std::move(vfs)) {
    watcher = new QFileSystemWatcher(this);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &GameList::RefreshGameDirectory);

    this->main_window = parent;
    layout = new QVBoxLayout;
    tree_view = new QTreeView;
    search_field = new GameListSearchField(this);
    item_model = new QStandardItemModel(tree_view);
    tree_view->setModel(item_model);

    tree_view->setAlternatingRowColors(true);
    tree_view->setSelectionMode(QHeaderView::SingleSelection);
    tree_view->setSelectionBehavior(QHeaderView::SelectRows);
    tree_view->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    tree_view->setSortingEnabled(true);
    tree_view->setEditTriggers(QHeaderView::NoEditTriggers);
    tree_view->setUniformRowHeights(true);
    tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view->setStyleSheet("QTreeView{ border: none; }");

    item_model->insertColumns(0, UISettings::values.show_add_ons ? COLUMN_COUNT : COLUMN_COUNT - 1);
    item_model->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
    item_model->setHeaderData(COLUMN_COMPATIBILITY, Qt::Horizontal, tr("Compatibility"));

    if (UISettings::values.show_add_ons) {
        item_model->setHeaderData(COLUMN_ADD_ONS, Qt::Horizontal, tr("Add-ons"));
        item_model->setHeaderData(COLUMN_FILE_TYPE, Qt::Horizontal, tr("File type"));
        item_model->setHeaderData(COLUMN_SIZE, Qt::Horizontal, tr("Size"));
    } else {
        item_model->setHeaderData(COLUMN_FILE_TYPE - 1, Qt::Horizontal, tr("File type"));
        item_model->setHeaderData(COLUMN_SIZE - 1, Qt::Horizontal, tr("Size"));
    }

    connect(tree_view, &QTreeView::activated, this, &GameList::ValidateEntry);
    connect(tree_view, &QTreeView::customContextMenuRequested, this, &GameList::PopupContextMenu);

    // We must register all custom types with the Qt Automoc system so that we are able to use it
    // with signals/slots. In this case, QList falls under the umbrells of custom types.
    qRegisterMetaType<QList<QStandardItem*>>("QList<QStandardItem*>");

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(tree_view);
    layout->addWidget(search_field);
    setLayout(layout);
}

GameList::~GameList() {
    emit ShouldCancelWorker();
}

void GameList::setFilterFocus() {
    if (tree_view->model()->rowCount() > 0) {
        search_field->setFocus();
    }
}

void GameList::setFilterVisible(bool visibility) {
    search_field->setVisible(visibility);
}

void GameList::clearFilter() {
    search_field->clear();
}

void GameList::AddEntry(const QList<QStandardItem*>& entry_items) {
    item_model->invisibleRootItem()->appendRow(entry_items);
}

void GameList::ValidateEntry(const QModelIndex& item) {
    // We don't care about the individual QStandardItem that was selected, but its row.
    const int row = item_model->itemFromIndex(item)->row();
    const QStandardItem* child_file = item_model->invisibleRootItem()->child(row, COLUMN_NAME);
    const QString file_path = child_file->data(GameListItemPath::FullPathRole).toString();

    if (file_path.isEmpty())
        return;

    if (!QFileInfo::exists(file_path))
        return;

    const QFileInfo file_info{file_path};
    if (file_info.isDir()) {
        const QDir dir{file_path};
        const QStringList matching_main = dir.entryList(QStringList("main"), QDir::Files);
        if (matching_main.size() == 1) {
            emit GameChosen(dir.path() + DIR_SEP + matching_main[0]);
        }
        return;
    }

    // Users usually want to run a diffrent game after closing one
    search_field->clear();
    emit GameChosen(file_path);
}

void GameList::DonePopulating(QStringList watch_list) {
    // Clear out the old directories to watch for changes and add the new ones
    auto watch_dirs = watcher->directories();
    if (!watch_dirs.isEmpty()) {
        watcher->removePaths(watch_dirs);
    }
    // Workaround: Add the watch paths in chunks to allow the gui to refresh
    // This prevents the UI from stalling when a large number of watch paths are added
    // Also artificially caps the watcher to a certain number of directories
    constexpr int LIMIT_WATCH_DIRECTORIES = 5000;
    constexpr int SLICE_SIZE = 25;
    int len = std::min(watch_list.length(), LIMIT_WATCH_DIRECTORIES);
    for (int i = 0; i < len; i += SLICE_SIZE) {
        watcher->addPaths(watch_list.mid(i, i + SLICE_SIZE));
        QCoreApplication::processEvents();
    }
    tree_view->setEnabled(true);
    int rowCount = tree_view->model()->rowCount();
    search_field->setFilterResult(rowCount, rowCount);
    if (rowCount > 0) {
        search_field->setFocus();
    }
}

void GameList::PopupContextMenu(const QPoint& menu_location) {
    QModelIndex item = tree_view->indexAt(menu_location);
    if (!item.isValid())
        return;

    int row = item_model->itemFromIndex(item)->row();
    QStandardItem* child_file = item_model->invisibleRootItem()->child(row, COLUMN_NAME);
    u64 program_id = child_file->data(GameListItemPath::ProgramIdRole).toULongLong();
    std::string path = child_file->data(GameListItemPath::FullPathRole).toString().toStdString();

    QMenu context_menu;
    QAction* open_save_location = context_menu.addAction(tr("Open Save Data Location"));
    QAction* open_lfs_location = context_menu.addAction(tr("Open Mod Data Location"));
    context_menu.addSeparator();
    QAction* dump_romfs = context_menu.addAction(tr("Dump RomFS"));
    QAction* copy_tid = context_menu.addAction(tr("Copy Title ID to Clipboard"));
    QAction* navigate_to_gamedb_entry = context_menu.addAction(tr("Navigate to GameDB entry"));
    context_menu.addSeparator();
    QAction* properties = context_menu.addAction(tr("Properties"));

    open_save_location->setEnabled(program_id != 0);
    auto it = FindMatchingCompatibilityEntry(compatibility_list, program_id);
    navigate_to_gamedb_entry->setVisible(it != compatibility_list.end() && program_id != 0);

    connect(open_save_location, &QAction::triggered,
            [&]() { emit OpenFolderRequested(program_id, GameListOpenTarget::SaveData); });
    connect(open_lfs_location, &QAction::triggered,
            [&]() { emit OpenFolderRequested(program_id, GameListOpenTarget::ModData); });
    connect(dump_romfs, &QAction::triggered, [&]() { emit DumpRomFSRequested(program_id, path); });
    connect(copy_tid, &QAction::triggered, [&]() { emit CopyTIDRequested(program_id); });
    connect(navigate_to_gamedb_entry, &QAction::triggered,
            [&]() { emit NavigateToGamedbEntryRequested(program_id, compatibility_list); });
    connect(properties, &QAction::triggered, [&]() { emit OpenPerGameGeneralRequested(path); });

    context_menu.exec(tree_view->viewport()->mapToGlobal(menu_location));
}

void GameList::LoadCompatibilityList() {
    QFile compat_list{":compatibility_list/compatibility_list.json"};

    if (!compat_list.open(QFile::ReadOnly | QFile::Text)) {
        LOG_ERROR(Frontend, "Unable to open game compatibility list");
        return;
    }

    if (compat_list.size() == 0) {
        LOG_WARNING(Frontend, "Game compatibility list is empty");
        return;
    }

    const QByteArray content = compat_list.readAll();
    if (content.isEmpty()) {
        LOG_ERROR(Frontend, "Unable to completely read game compatibility list");
        return;
    }

    const QString string_content = content;
    QJsonDocument json = QJsonDocument::fromJson(string_content.toUtf8());
    QJsonArray arr = json.array();

    for (const QJsonValueRef value : arr) {
        QJsonObject game = value.toObject();

        if (game.contains("compatibility") && game["compatibility"].isDouble()) {
            int compatibility = game["compatibility"].toInt();
            QString directory = game["directory"].toString();
            QJsonArray ids = game["releases"].toArray();

            for (const QJsonValueRef id_ref : ids) {
                QJsonObject id_object = id_ref.toObject();
                QString id = id_object["id"].toString();
                compatibility_list.emplace(
                    id.toUpper().toStdString(),
                    std::make_pair(QString::number(compatibility), directory));
            }
        }
    }
}

void GameList::PopulateAsync(const QString& dir_path, bool deep_scan) {
    const QFileInfo dir_info{dir_path};
    if (!dir_info.exists() || !dir_info.isDir()) {
        LOG_ERROR(Frontend, "Could not find game list folder at {}", dir_path.toStdString());
        search_field->setFilterResult(0, 0);
        return;
    }

    tree_view->setEnabled(false);

    // Update the columns in case UISettings has changed
    item_model->removeColumns(0, item_model->columnCount());
    item_model->insertColumns(0, UISettings::values.show_add_ons ? COLUMN_COUNT : COLUMN_COUNT - 1);
    item_model->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
    item_model->setHeaderData(COLUMN_COMPATIBILITY, Qt::Horizontal, tr("Compatibility"));

    if (UISettings::values.show_add_ons) {
        item_model->setHeaderData(COLUMN_ADD_ONS, Qt::Horizontal, tr("Add-ons"));
        item_model->setHeaderData(COLUMN_FILE_TYPE, Qt::Horizontal, tr("File type"));
        item_model->setHeaderData(COLUMN_SIZE, Qt::Horizontal, tr("Size"));
    } else {
        item_model->setHeaderData(COLUMN_FILE_TYPE - 1, Qt::Horizontal, tr("File type"));
        item_model->setHeaderData(COLUMN_SIZE - 1, Qt::Horizontal, tr("Size"));
        item_model->removeColumns(COLUMN_COUNT - 1, 1);
    }

    LoadInterfaceLayout();

    // Delete any rows that might already exist if we're repopulating
    item_model->removeRows(0, item_model->rowCount());

    emit ShouldCancelWorker();

    GameListWorker* worker = new GameListWorker(vfs, dir_path, deep_scan, compatibility_list);

    connect(worker, &GameListWorker::EntryReady, this, &GameList::AddEntry, Qt::QueuedConnection);
    connect(worker, &GameListWorker::Finished, this, &GameList::DonePopulating,
            Qt::QueuedConnection);
    // Use DirectConnection here because worker->Cancel() is thread-safe and we want it to cancel
    // without delay.
    connect(this, &GameList::ShouldCancelWorker, worker, &GameListWorker::Cancel,
            Qt::DirectConnection);

    QThreadPool::globalInstance()->start(worker);
    current_worker = std::move(worker);
}

void GameList::SaveInterfaceLayout() {
    UISettings::values.gamelist_header_state = tree_view->header()->saveState();
}

void GameList::LoadInterfaceLayout() {
    auto header = tree_view->header();
    if (!header->restoreState(UISettings::values.gamelist_header_state)) {
        // We are using the name column to display icons and titles
        // so make it as large as possible as default.
        header->resizeSection(COLUMN_NAME, header->width());
    }

    item_model->sort(header->sortIndicatorSection(), header->sortIndicatorOrder());
}

const QStringList GameList::supported_file_extensions = {"nso", "nro", "nca", "xci", "nsp"};

void GameList::RefreshGameDirectory() {
    if (!UISettings::values.gamedir.isEmpty() && current_worker != nullptr) {
        LOG_INFO(Frontend, "Change detected in the games directory. Reloading game list.");
        search_field->clear();
        PopulateAsync(UISettings::values.gamedir, UISettings::values.gamedir_deepscan);
    }
}
