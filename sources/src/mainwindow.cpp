#include <vd/mainwindow.hpp>
#include <vd/ffmpeg.hpp>
#include <vd/sdl.hpp>

#include "ui_mainwindow.h"

#include <QLabel>
#include <QThread>
#include <QPushButton>
#include <QStateMachine>

QApplication* app_;

namespace vd {
	
MainWindow::MainWindow(QWidget *parent) 
:	QMainWindow(parent),
	ui(new ::Ui::MainWindow)
{
	ui->setupUi(this);
	showNormal();
	QLabel* l = new QLabel(ui->statusbar);
	ui->statusbar->showMessage("Loaded", 2000);
	VD_LOG("UI successully initalized");

	grabKeyboard();

	FfmpegPlugin ffmpeg;
	ffmpeg.install();

	preview_ = new Preview(ui->video);

	project_.reset(new Project(std::make_shared<SdlVideoPresenter>(ui->video), 
		std::make_shared<SdlAudioPresenter>(preview_->audio())));
	project_->_create_test();
	preview_->bind(project_.get());

	ui->timeline->bind(&*project_->time_line_);
	ui->timeline->set_preview(preview_);
	
	QState* pause_state_ = new QState();
	pause_state_->assignProperty(ui->play_btn, "icon", QIcon(":/icon-play.png"));
	connect(pause_state_, SIGNAL(entered()), preview_, SLOT(pause_play()));
 
	play_state_ = new QState();
	play_state_->assignProperty(ui->play_btn, "icon", QIcon(":/icon-pause.png"));
	connect(play_state_, SIGNAL(entered()), preview_, SLOT(continue_play()));

	pause_state_->addTransition(ui->play_btn, SIGNAL(clicked()), play_state_);
	play_state_->addTransition(ui->play_btn, SIGNAL(clicked()), pause_state_);

	machine_.addState(pause_state_);
	machine_.addState(play_state_);

	machine_.setInitialState(pause_state_);
	machine_.start();
	
	preview_thread_ = new QThread(this);
	connect(preview_thread_, SIGNAL(started()), preview_, SLOT(_start_play()));
	preview_->moveToThread(preview_thread_);

	connect(ui->volume, SIGNAL(valueChanged(int)), this, SLOT(audio_volume_changed(int)));

	VD_LOG("Running preview thread");
	preview_thread_->start();
}
	
MainWindow::~MainWindow()
{
	VD_LOG("Stopping preview thread");
	preview_->stop(); // Signal to stop
	while (preview_thread_->isRunning()) {} // Waiting
	VD_LOG("Preview thread stopped");

	delete preview_thread_;
	delete preview_;

	VD_LOG("Closing UI");
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent* ev) 
{
     ev->accept();

}

void MainWindow::audio_volume_changed(int volume)
{
	preview_->set_audio_volume(float(volume) / 100.f);
}

void MainWindow::keyPressEvent(QKeyEvent* ev)
{
	switch (ev->key())
	{
	case Qt::Key_Enter:
	case Qt::Key_Space:
	{
		emit ui->play_btn->clicked(true);
	}
		break;
	}
}

} // namespace vidos

