/*
 *  IHU -- I Hear U, easy VoIP application using Speex and Qt
 *
 *  Copyright (C) 2003-2008 Matteo Trotta - <mrotta@users.sourceforge.net>
 *
 *  http://ihu.sourceforge.net
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 */

#include "PlayerDialog.hpp"

#include <QtGui>
#include <Qt3Support>

#include "Config.h"
#include "Error.h"

#define STAT_TIME 1000

PlayerDialog::PlayerDialog( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : QDialog( parent, name, modal, fl )
{
	setName( "PlayerDialog" );
	setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, sizePolicy().hasHeightForWidth() ) );

	setMaximumSize(600, 150);

	Q3BoxLayout *playerLayout = new Q3VBoxLayout( this, 0, 0);

	playerFrame = new Q3Frame( this, "playerFrame" );

	Q3BoxLayout *playLayout = new Q3VBoxLayout( playerFrame, 5, 5 );

	playLayout->addStretch();

	slider = new QSlider( playerFrame, "slider" );
	slider->setTracking(FALSE);
	slider->setOrientation( Qt::Horizontal );
	slider->setMinValue(0);
	slider->setMaxValue(100);
	slider->setEnabled(FALSE);
	playLayout->addWidget(slider);

	playLayout->addStretch();

	openButton = new QPushButton( playerFrame, "openButton" );
	convertButton = new QPushButton( playerFrame, "convertButton" );
	pauseButton = new QPushButton( playerFrame, "pauseButton" );
	pauseButton->setToggleButton(TRUE);
	pauseButton->setEnabled(FALSE);
	stopButton = new QPushButton( playerFrame, "stopButton" );
	stopButton->setEnabled(FALSE);

	Q3BoxLayout *h2Layout = new Q3HBoxLayout( playLayout );
	h2Layout->addWidget(openButton);
	h2Layout->addWidget(pauseButton);
	h2Layout->addWidget(stopButton);
	h2Layout->addSpacing(5);
	h2Layout->addWidget(convertButton);

	playLayout->addStretch();

	playerLayout->addWidget(playerFrame);

	statusbar = new QStatusBar(this, "statusbar");

	QWidget *progressWidget = new QWidget( this, "progressWidget");
	progressWidget->setMaximumHeight(16);
	progressWidget->setMaximumWidth(80);
	Q3BoxLayout *pLayout = new Q3HBoxLayout(progressWidget);
	progressBar = new Q3ProgressBar( progressWidget, "progressBar" );
	progressBar->setMaximumHeight(16);
	progressBar->setMaximumWidth(80);
	progressBar->reset();
	progressBar->setEnabled(FALSE);

	pLayout->addWidget(progressBar);
	statusbar->addWidget( progressWidget, 0, TRUE);

	playerLayout->addWidget(statusbar);
	
	statTimer = new QTimer(this);

	languageChange();

	// signals and slots connections
	connect( openButton, SIGNAL( clicked() ), this, SLOT( playFile() ) );
	connect( convertButton, SIGNAL( clicked() ), this, SLOT( convertFile() ) );
	connect( pauseButton, SIGNAL( toggled(bool) ), this, SLOT( pauseButtonToggled(bool) ) );
	connect( stopButton, SIGNAL( clicked() ), this, SLOT( stopButtonClicked() ) );
	connect( slider, SIGNAL( sliderPressed() ), this, SLOT( sliderPress() ) );
	connect( slider, SIGNAL( sliderReleased() ), this, SLOT( sliderRelease() ) );
	connect( statTimer, SIGNAL( timeout() ), this, SLOT( statistics() ) );

	fileplayer = new FilePlayer();

	connect( fileplayer, SIGNAL(keyRequest()), this, SLOT(keyRequest()) );
	connect( fileplayer, SIGNAL(finish()), this, SLOT(stopFile()) );
	connect( fileplayer, SIGNAL(warning(QString)), this, SLOT(warning(QString)) );
	connect( fileplayer, SIGNAL(error(QString)), this, SLOT(abort(QString)) );

	sliderFree = true;
	fileName = "";
	resize(minimumSizeHint());
}

/*
 *  Destroys the object and frees any allocated resources
 */
PlayerDialog::~PlayerDialog()
{
	delete fileplayer;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void PlayerDialog::languageChange()
{
	setCaption( tr( "IHU File Player" ) );
	openButton->setText( tr( "&Open File..." ) );
	openButton->setIconSet( QIcon( qPixmapFromMimeSource( "open.png" ) ) );
	openButton->setAccel( QString("Ctrl+O") );
	convertButton->setText( tr( "&Convert to .spx" ) );
	convertButton->setIconSet( QIcon( qPixmapFromMimeSource( "save.png" ) ) );
	pauseButton->setText( tr( "&Pause" ) );
	pauseButton->setIconSet( QIcon( qPixmapFromMimeSource( "pause.png" ) ) );
	stopButton->setText( tr( "&Stop" ) );
	stopButton->setIconSet( QIcon( qPixmapFromMimeSource( "stop.png" ) ) );
	statusbar->message("Please open a file to play/convert");
}

void PlayerDialog::playFile()
{
	QString fName = Q3FileDialog::getOpenFileName("", "*"IHU_EXT, this, 0, "Open IHU file to play...");
	if (!fName.isEmpty())
        {
		playFile(fName);
	}
}

void PlayerDialog::playFile(QString fName)
{
	fileplayer->end();
	fileName = fName;
	try
	{
		ihu2spx = false;
		fileplayer->playFile(fileName);
		pauseButton->setEnabled(TRUE);
		stopButton->setEnabled(TRUE);
		convertButton->setEnabled(FALSE);
		slider->setEnabled(TRUE);
		progressBar->setEnabled(TRUE);
		changeProgress(0);
		message("Opening " + fileName + "...") ;
		statTimer->start(STAT_TIME, false);
	}
	catch (Error e)
	{
		abort(e.getText());
	}
}

void PlayerDialog::convertFile()
{
	fileplayer->end();
	try
	{
	        fileName=Q3FileDialog::getOpenFileName("", "*"IHU_EXT, this, 0, "Open IHU file to convert...");
	        if (!fileName.isEmpty())
	        {
			fileplayer->convertFile(fileName);
			ihu2spx = true;
			openButton->setEnabled(FALSE);
			convertButton->setEnabled(FALSE);
			stopButton->setEnabled(TRUE);
			slider->setEnabled(FALSE);
			progressBar->setEnabled(TRUE);
			changeProgress(0);
			message("Opening " + fileName + "...") ;
			statTimer->start(STAT_TIME, false);
		}
	}
	catch (Error e)
	{
		abort(e.getText());
	}
}

void PlayerDialog::stopFile()
{
	message(tr("Done"));
	fileplayer->end();
	statTimer->stop();
	openButton->setEnabled(TRUE);
	if (pauseButton->isOn())
		pauseButton->setOn(FALSE);
	pauseButton->setEnabled(FALSE);
	stopButton->setEnabled(FALSE);
	convertButton->setEnabled(TRUE);
	slider->setValue(0);
	slider->setEnabled(FALSE);
	progressBar->reset();
	progressBar->setEnabled(FALSE);
	sliderFree = true;
	ihu2spx = false;
	fileName = "";
}

void PlayerDialog::pauseButtonToggled(bool on)
{
	if (on)
		fileplayer->pause();
	else
		fileplayer->resume();
}

void PlayerDialog::stopButtonClicked()
{
	stopFile();
}

void PlayerDialog::warning(QString text)
{
	message(text);
}

void PlayerDialog::abort(QString text)
{
	stopFile();
	message("Aborted");
	QMessageBox::critical(0, "IHU Player Error", tr("Error: ") + text);
}

void PlayerDialog::sliderPress()
{
	fileplayer->stop();
	sliderFree = false;
}

void PlayerDialog::sliderRelease()
{
	fileplayer->seekFile(slider->value());
	fileplayer->go();
	sliderFree = true;
}

void PlayerDialog::changeProgress(int pos)
{
	progressBar->setProgress(pos);
	if (sliderFree && !ihu2spx)
	{
		slider->setValue(pos);
	}
}

void PlayerDialog::keyRequest()
{
	fileplayer->stop();
	QString text;
	switch (QMessageBox::warning(0, "IHU Error", "Warning: stream is crypted but the decryption key is not available.\n\nOK if you want to enter the decryption passphrase, IGNORE if you\nwant to continue anyway, CANCEL to stop.", QMessageBox::Ok, QMessageBox::Ignore, QMessageBox::Cancel))
	{
		case QMessageBox::Ok:
			bool ok;
			text = QInputDialog::getText("Decryption Passphrase", "Enter the decryption passphrase, leave blank to reset.", QLineEdit::Password, QString::null, &ok, this );
			if (ok)
			{
				fileplayer->disableDecrypt();
				if (!text.isEmpty())
				{
					fileplayer->enableDecrypt((char *) text.ascii(), text.length());
					fileplayer->go();
					message(tr("Decryption key changed successfully."));
				}
			}
			break;
		case QMessageBox::Ignore:
			fileplayer->noDecrypt();
			fileplayer->go();
			break;
		default:
			stopFile();
			break;
	}
}

void PlayerDialog::statistics()
{
	changeProgress(fileplayer->getProgress());
	if (ihu2spx)
		message("Converting " + fileName + "...") ;
	else
		message(QString("Playing %1 (%2)").arg(fileName).arg(fileplayer->getCallerName()));
}

void PlayerDialog::message(QString text)
{
	statusbar->message(text, STAT_TIME*3);
}

void PlayerDialog::setupPlayer(QString interface)
{
	fileplayer->setupPlayer(interface);
}
