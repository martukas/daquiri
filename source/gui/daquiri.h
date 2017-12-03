#pragma once

#include <QMainWindow>
#include "SettingsForm.h"

#include "custom_logger.h"
#include "qt_boost_logger.h"
#include "producer.h"

namespace Ui {
class daquiri;
}

class daquiri : public QMainWindow
{
    Q_OBJECT

  public:
    explicit daquiri(QWidget *parent = 0,
                     bool open_new_project = false,
                     bool start_daq = false);
    ~daquiri();

  private:
    Ui::daquiri *ui;

    //connect gui with boost logger framework
    std::stringstream log_stream_;
    LogEmitter        my_emitter_;
    LogStreamBuffer   text_buffer_;

    Container<DAQuiri::Detector>   detectors_;
    std::vector<DAQuiri::Detector> current_dets_;
    ThreadRunner                   runner_thread_;
    DAQuiri::ProducerStatus engine_status_;
    QString profile_description_;

    SettingsForm* main_tab_ {nullptr};

    bool gui_enabled_ {true};

    bool open_new_project_ {false};
    bool start_daq_ {false};


  signals:
    void toggle_push(bool, DAQuiri::ProducerStatus);

  protected:
    void closeEvent(QCloseEvent*) Q_DECL_OVERRIDE;

  private slots:
    void update_settings(DAQuiri::Setting,
                         std::vector<DAQuiri::Detector>,
                         DAQuiri::ProducerStatus);
    void toggleIO(bool);

    void close_tab_at(int index);
    void close_tab_widget(QWidget*);

    //logger receiver
    void add_log_text(QString);

    void on_splitter_splitterMoved(int pos, int index);

    void tabs_moved(int, int);
    void tab_changed(int);

    void open_list();
    void open_new_proj();
    void open_project(DAQuiri::ProjectPtr = nullptr, bool start = false);

    void initialize_settings_dir();

  private:
    //helper functions
    void saveSettings();
    void loadSettings();
    void reorder_tabs();
    void add_closable_tab(QWidget*);
};

