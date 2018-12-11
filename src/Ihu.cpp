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

#include <stdlib.h>

#include <QtGui>
#include <Qt3Support>

#include "Error.h"
#include "Ihu.hpp"
#include "Settings.hpp"
#include "LogViewer.hpp"
#include "CallTab.hpp"

// Timeout (ms) for statistics
#define STAT_TIME 1000

#define VDATE "2008/Jan/30"

#define IHU_ENABLED "enabled"
#define IHU_DISABLED "disabled"

/*
 *  Constructs a Ihu as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
Ihu::Ihu( Config& ihucfg, QWidget* parent, const char* name, Qt::WFlags fl )
    : Q3MainWindow( parent, name, fl ), ihuconfig(ihucfg)
{
	setName( "IHU" );
	setIcon( qPixmapFromMimeSource( "ihu.png" ) );
	setCentralWidget( new QWidget( this, "qt_central_widget" ) );
	setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)1, 0, 0, sizePolicy().hasHeightForWidth() ) );
	setMinimumSize(QSize(300, 400));
	resize( QSize(300, 400).expandedTo(minimumSizeHint()) );
	
	Q3BoxLayout *ihuLayout = new Q3VBoxLayout( centralWidget(), 15, 15 );
	
	callFrame = new Q3Frame( centralWidget(), "callFrame" );
	callFrame->setFrameStyle( Q3Frame::Panel | Q3Frame::Raised );
	ihuLayout->addWidget(callFrame);
	
	Q3BoxLayout *mLayout = new Q3VBoxLayout( callFrame, 0, 0);
	
	callWidget = new QTabWidget(callFrame, "callTabWidget");
	mLayout->addWidget(callWidget);

	ihuLayout->addStretch();

	waitFrame = new Q3Frame( centralWidget(), "waitFrame" );
	ihuLayout->addWidget(waitFrame);

	Q3BoxLayout *whLayout = new Q3HBoxLayout(waitFrame, 0, 0);

	whLayout->addSpacing(30);

	muteMicButton = new QToolButton( waitFrame, "muteMicButton" );
	muteMicButton->setMinimumSize( QSize(40, 40) );
	muteMicButton->setToggleButton(TRUE);
	muteMicButton->setIconSet( QIcon( qPixmapFromMimeSource( "mic.png" ) ) );
	whLayout->addWidget(muteMicButton);

	whLayout->addStretch();

	waitButton = new QPushButton( waitFrame, "waitButton" );
	waitButton->setMinimumSize( QSize(160, 40) );
	waitButton->setToggleButton(TRUE);
	waitButton->setIconSet( QIcon( qPixmapFromMimeSource( "phone_no.png" ) ) );
	whLayout->addWidget(waitButton);

	whLayout->addStretch();

	muteSpkButton = new QToolButton( waitFrame, "muteSpkButton" );
	muteSpkButton->setMinimumSize( QSize(40, 40) );
	muteSpkButton->setToggleButton(TRUE);
	muteSpkButton->setIconSet( QIcon( qPixmapFromMimeSource( "speaker.png" ) ) );
	whLayout->addWidget(muteSpkButton);
	
	whLayout->addSpacing(30);

	ihuLayout->addStretch();

	otherFrame = new Q3Frame( centralWidget(), "otherFrame" );
	otherFrame->setFrameStyle( Q3Frame::Box | Q3Frame::Sunken );
	ihuLayout->addWidget(otherFrame);
	
	Q3BoxLayout *oLayout = new Q3VBoxLayout( otherFrame, 15, 15);
	
	Q3GridLayout *gLayout = new Q3GridLayout( oLayout, 2, 2, 10);
	
	threshold = new QLabel( otherFrame, "threshold" );
	
	thSlider = new QSlider( otherFrame, "thSlider" );
	thSlider->setTracking(TRUE);
	thSlider->setOrientation( Qt::Horizontal );
	thSlider->setRange(-96, 0);
	
	soundLabel = new QLabel( otherFrame, "soundLabel" );
	soundLabel->setEnabled(FALSE);
	
	soundLevel = new Q3ProgressBar( otherFrame, "soundLevel" );
	soundLevel->setMaximumHeight(20);
	soundLevel->setPercentageVisible(FALSE);
	soundLevel->reset();
	soundLevel->setEnabled(FALSE);
	
	gLayout->addWidget(threshold, 0, 0, Qt::AlignBottom | Qt::AlignHCenter);
	gLayout->addWidget(thSlider, 1, 0, Qt::AlignTop);
	gLayout->addWidget(soundLabel, 0, 1, Qt::AlignBottom | Qt::AlignHCenter);
	gLayout->addWidget(soundLevel, 1, 1, Qt::AlignTop);
	
	statusbar = statusBar();

	QWidget *delayWidget = new QWidget( this, "delayWidget");
	Q3BoxLayout *dLayout = new Q3HBoxLayout(delayWidget);
	delayLabel = new QLabel( delayWidget, "delayLabel" );
	delayLabel->setEnabled(FALSE);
	dLayout->addWidget(delayLabel);
	statusbar->addWidget( delayWidget, 0, TRUE);

	QWidget *trafficWidget = new QWidget( this, "trafficWidget");
	Q3BoxLayout *tLayout = new Q3HBoxLayout(trafficWidget);
	trafficLabel = new QLabel( trafficWidget, "trafficLabel" );
	tLayout->addWidget(trafficLabel);
	statusbar->addWidget( trafficWidget, 0, TRUE);
	
	// actions
	newCallAction = new QAction( this, "newCallAction" );
	closeCallAction = new QAction( this, "newCallAction" );
	filePlayFileAction = new QAction( this, "filePlayFileAction" );
	fileQuitAction = new QAction( this, "fileQuitAction" );
	helpContentsAction = new QAction( this, "helpContentsAction" );
	helpAboutAction = new QAction( this, "helpAboutAction" );
	settingsAction = new QAction( this, "settingsAction" );
	settingsAction->setIconSet( QIcon( qPixmapFromMimeSource( "configure.png" ) ) );
	logAction = new QAction( this, "logAction" );
	cryptAction = new QAction( this, "cryptAction" );
	cryptAction->setToggleAction(TRUE);
	changeKeyAction = new QAction( this, "changeKeyAction" );
	setDecKeyAction = new QAction( this, "setDecKeyAction" );
	adrAction = new QAction( this, "adrAction" );
	adrAction->setToggleAction(TRUE);
	agcAction = new QAction( this, "agcAction" );
	agcAction->setToggleAction(TRUE);
	changeKeyAction->setEnabled(FALSE);
	dumpRXAction = new QAction( this, "dumpRXAction" );
	dumpRXAction->setToggleAction(TRUE);
	dumpTXAction = new QAction( this, "dumpTXAction" );
	dumpTXAction->setToggleAction(TRUE);
	clearhAction = new QAction( this, "clearhAction" );
	clearmAction = new QAction( this, "clearmAction" );
	
	// menubar
	menubar = new QMenuBar( this, "menubar" );
	
	fileMenu = new QMenu( this );
	newCallAction->addTo( fileMenu );
	closeCallAction->addTo( fileMenu );
	fileMenu->insertSeparator();
	filePlayFileAction->addTo( fileMenu );
	logAction->addTo( fileMenu );
	fileMenu->insertSeparator();
	fileQuitAction->addTo( fileMenu );
	
	callMenu = new QMenu( this );
	cryptAction->addTo( callMenu );
	changeKeyAction->addTo( callMenu );
	setDecKeyAction->addTo( callMenu );
	callMenu->insertSeparator();
	dumpRXAction->addTo( callMenu );
	dumpTXAction->addTo( callMenu );

	optionsMenu = new QMenu( this );
	adrAction->addTo( optionsMenu );
	agcAction->addTo( optionsMenu );
	optionsMenu->insertSeparator();
	clearhAction->addTo( optionsMenu );
	clearmAction->addTo( optionsMenu );
	optionsMenu->insertSeparator();
	settingsAction->addTo( optionsMenu );
	
	helpMenu = new QMenu( this );
	
	helpContentsAction->addTo( helpMenu );
	helpMenu->insertSeparator();
	helpAboutAction->addTo( helpMenu );
	
	menubar->insertItem( "", fileMenu, 0);
	menubar->insertItem( "", callMenu, 1);
	menubar->insertItem( "", optionsMenu, 2 );
	menubar->insertItem( "", helpMenu, 3 );

	logViewer = new LogViewer(this);
	playerDialog = new PlayerDialog(this);

	timer = new QTimer(this);
	
	languageChange();

	//signals and slots connections
	connect( timer, SIGNAL(timeout()), this, SLOT(statistics()) );
	connect( newCallAction, SIGNAL( activated() ), this, SLOT( newCall() ) );
	connect( closeCallAction, SIGNAL( activated() ), this, SLOT( closeCall() ) );
	connect( fileQuitAction, SIGNAL( activated() ), this, SLOT( quit() ) );
	connect( helpContentsAction, SIGNAL( activated() ), this, SLOT( helpContents() ) );
	connect( helpAboutAction, SIGNAL( activated() ), this, SLOT( helpAbout() ) );
	connect( settingsAction, SIGNAL( activated() ), this, SLOT( settings() ) );
	connect( clearhAction, SIGNAL( activated() ), this, SLOT( clearHistory() ) );
	connect( clearmAction, SIGNAL( activated() ), this, SLOT( clearMissed() ) );
	connect( logAction, SIGNAL( activated() ), this, SLOT( showLogViewer() ) );
	connect( cryptAction, SIGNAL( activated() ), this, SLOT( crypt() ) );
	connect( changeKeyAction, SIGNAL( activated() ), this, SLOT( changeKey() ) );
	connect( setDecKeyAction, SIGNAL( activated() ), this, SLOT( setDecryptionKey() ) );
	connect( dumpRXAction, SIGNAL( activated() ), this, SLOT( dumpRXEnable() ) );
	connect( dumpTXAction, SIGNAL( activated() ), this, SLOT( dumpTXEnable() ) );
	connect( waitButton, SIGNAL( clicked() ), this, SLOT( waitButtonClicked() ) );
	connect( adrAction, SIGNAL( activated() ), this, SLOT( adrOn() ) );
	connect( agcAction, SIGNAL( activated() ), this, SLOT( agcOn() ) );
	connect( thSlider, SIGNAL( valueChanged(int) ), this, SLOT( thSliderChanged(int) ) );
	connect( filePlayFileAction, SIGNAL( activated() ), this, SLOT( showPlayer() ) );
	connect( muteMicButton, SIGNAL( toggled(bool) ), this, SLOT( recorderStatus(bool) ) );
	connect( muteSpkButton, SIGNAL( toggled(bool) ), this, SLOT( playerStatus(bool) ) );
	connect( menubar, SIGNAL( highlighted(int) ), this, SLOT( menuUpdate() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
Ihu::~Ihu()
{
	// no need to delete child widgets, Qt does it all for us
	delete phone;
	delete logger;
	delete[] callTab;
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void Ihu::languageChange()
{
	setCaption( tr( "IHU" ) );
	waitButton->setText( tr( "&Wait for calls" ) );
	trafficLabel->setText( tr( "0.0 KB/s" ) );
	threshold->setText( QString( "TX threshold: %1 dB" ).arg( thSlider->value() ) );
	soundLabel->setText( QString( "Recording Level: -96 dB" ) );
	newCallAction->setText( tr( "New Call" ) );
	newCallAction->setMenuText( tr( "&New Call" ) );
	newCallAction->setStatusTip( tr( "Create a new call" ) );
	closeCallAction->setText( tr( "Close Call" ) );
	closeCallAction->setMenuText( tr( "&Close Call" ) );
	closeCallAction->setStatusTip( tr( "Close current call" ) );
	fileQuitAction->setText( tr( "Quit" ) );
	fileQuitAction->setMenuText( tr( "&Quit" ) );
	helpContentsAction->setText( tr( "Contents" ) );
	helpContentsAction->setMenuText( tr( "&Contents..." ) );
	helpContentsAction->setStatusTip( tr( "Shows help contents" ) );
	helpAboutAction->setText( tr( "About" ) );
	helpAboutAction->setMenuText( tr( "&About" ) );
	helpAboutAction->setAccel( QKeySequence(QString::null) );
	helpAboutAction->setStatusTip( tr( "Show info on IHU" ) );
	filePlayFileAction->setText( tr( "File Player"  ) );
	filePlayFileAction->setMenuText( tr( "File &Player" ) );
	filePlayFileAction->setStatusTip( tr( "Shows the IHU Player to play recorded .ihu files" ) );
	settingsAction->setText( tr( "Settings" ) );
	settingsAction->setMenuText( tr( "&Settings..." ) );
	settingsAction->setStatusTip( tr( "Adjust settings" ) );
	logAction->setText( tr( "Log Viewer" ) );
	logAction->setMenuText( tr( "&Log Viewer" ) );
	logAction->setStatusTip( tr( "Shows the IHU Log Viewer" ) );
	clearhAction->setText( tr( "Clear Host History" ) );
	clearhAction->setMenuText( tr( "&Clear Host History" ) );
	clearhAction->setStatusTip( tr( "Clear Host History" ) );
	clearmAction->setText( tr( "Clear Missed Calls" ) );
	clearmAction->setMenuText( tr( "Clear &Missed Calls" ) );
	clearmAction->setStatusTip( tr( "Clear Missed Calls" ) );
	adrAction->setText( tr( "Audio Delay Reduction" ) );
	adrAction->setMenuText( tr( "Audio Delay &Reduction (ADR)" ) );
	adrAction->setStatusTip( tr( "Activates the automatic Audio Delay Reduction (ADR)" ) );
	agcAction->setText( tr( "Automatic Gain Control" ) );
	agcAction->setMenuText( tr( "Automatic &Gain Control (AGC)" ) );
	agcAction->setStatusTip( tr( "Activates the Automatic Gain Control (AGC)" ) );
	changeKeyAction->setText( tr( "Change encryption key..." ) );
	changeKeyAction->setMenuText( tr( "&Change encryption key" ) );
	changeKeyAction->setStatusTip( tr( "Change the encryption key" ) );
	setDecKeyAction->setText( tr( "Set/reset decryption passphrase..." ) );
	setDecKeyAction->setMenuText( tr( "Set/reset &decryption key" ) );
	setDecKeyAction->setStatusTip( tr( "Set/reset decryption key" ) );
	cryptAction->setText( tr( "Encrypt outgoing stream" ) );
	cryptAction->setMenuText( tr( "&Encrypt outgoing stream" ) );
	cryptAction->setStatusTip( tr( "Encrypt the outgoing stream" ) );
	dumpRXAction->setText( tr( "Record incoming RX stream to file" ) );
	dumpRXAction->setMenuText( tr( "Record &incoming RX stream to file" ) );
	dumpRXAction->setStatusTip( tr( "Record incoming RX stream to file" ) );
	dumpTXAction->setText( tr( "Record outgoing TX stream to file" ) );
	dumpTXAction->setMenuText( tr( "Record &outgoing TX stream to file" ) );
	dumpTXAction->setStatusTip( tr( "Record outgoing TX stream to file" ) );
	delayLabel->setText( tr( "0.00 s" ) );
	
	muteMicButton->setAccel( QKeySequence( tr( "Alt+I" ) ) );
	muteSpkButton->setAccel( QKeySequence( tr( "Alt+O" ) ) );
	newCallAction->setAccel( QKeySequence( tr( "Ctrl+N" ) ) );
	closeCallAction->setAccel( QKeySequence( tr( "Ctrl+W" ) ) );
	fileQuitAction->setAccel( QKeySequence( tr( "Ctrl+Q" ) ) );
	settingsAction->setAccel( QKeySequence( tr( "Ctrl+S" ) ) );
	logAction->setAccel( QKeySequence( tr( "Ctrl+L" ) ) );
	filePlayFileAction->setAccel( QKeySequence( tr( "Ctrl+P" ) ) );
	adrAction->setAccel( QKeySequence( tr( "Ctrl+R" ) ) );
	agcAction->setAccel( QKeySequence( tr( "Ctrl+G" ) ) );
	setDecKeyAction->setAccel( QKeySequence( tr( "Ctrl+D" ) ) );
	changeKeyAction->setAccel( QKeySequence( tr( "Ctrl+K" ) ) );
	cryptAction->setAccel( QKeySequence( tr( "Ctrl+E" ) ) );
	dumpRXAction->setAccel( QKeySequence( tr( "Ctrl+I" ) ) );
	dumpTXAction->setAccel( QKeySequence( tr( "Ctrl+O" ) ) );
	clearhAction->setAccel( QKeySequence( tr( "Ctrl+H" ) ) );
	clearmAction->setAccel( QKeySequence( tr( "Ctrl+M" ) ) );
	
	QToolTip::add( waitButton, tr( "Enable this button to receive calls" ) );
	QToolTip::add( soundLevel, tr( "Input Sound Level" ) );
	QToolTip::add( thSlider, tr( "Threshold value: set up to a value that\nTX led is off when you are not speaking" ) );
	QToolTip::add( trafficLabel, tr( "Total network traffic" ) );
	QToolTip::add( muteMicButton, tr( "Disable audio input" ) );
	QToolTip::add( muteSpkButton, tr( "Disable audio output" ) );
	QToolTip::add( delayLabel, tr( "Audio Output Delay" ) );
	
	menubar->findItem( 0 )->setText( tr( "&File" ) );
	menubar->findItem( 1 )->setText( tr( "Ca&ll" ) );
	menubar->findItem( 2 )->setText( tr( "O&ptions" ) );
	menubar->findItem( 3 )->setText( tr( "H&elp" ) );
}

void Ihu::initIhu()
{
	try
	{
		if (!ihuconfig.checkConfig())
		{
			showMessageCritical(QString("%1 contains invalid settings!\nPlease remove the file and restart IHU to restore settings.\nIHU will start with default settings now...").arg(ihuconfig.getFileName()));
			ihuconfig.setDefault();
		}

		closing = false;
		autoanswer = false;
		autocrypt = false;
		showkey = false;
		
		trayIcon = NULL;

		maxcalls = ihuconfig.getMaxCalls();
		phone = new Phone(maxcalls);
		
		callTab = new CallTab*[maxcalls];
		for (int i=0; i<maxcalls; i++)
			callTab[i] = NULL;
		
		logger = new Logger();

		connect( phone, SIGNAL(receivedCallSignal(int)), this, SLOT(receivedCall(int)) );
		connect( phone, SIGNAL(connectedCallSignal(int)), this, SLOT(connectedCall(int)) );
		connect( phone, SIGNAL(cancelCallSignal(int)), this, SLOT(cancelCall(int)) );
		connect( phone, SIGNAL(abortSignal(QString)), this, SLOT(abortAll(QString)) );
		connect( phone, SIGNAL(abortCallSignal(int, QString)), this, SLOT(abortCall(int, QString)) );
		connect( phone, SIGNAL(messageSignal(int, QString)), this, SLOT(message(int, QString)) );
		connect( phone, SIGNAL(warningSignal(QString)), this, SLOT(warning(QString)) );
		connect( phone, SIGNAL(newKeySignal(int, QString)), this, SLOT(receivedNewKey(int, QString)) );
		connect( phone, SIGNAL(cryptedSignal(int)), this, SLOT(toCrypt(int)) );
		connect( phone, SIGNAL(newCallSignal(int)), this, SLOT(newCall(int)) );
		connect( phone, SIGNAL(recorderSignal(bool)), this, SLOT(enableRecorder(bool)) );
		connect( phone, SIGNAL(playerSignal(bool)), this, SLOT(enablePlayer(bool)) );
	
		applySettings();

		newCall();

		if (ihuconfig.getAutoWait())
		{
			waitButton->setOn(TRUE);
			waitButtonClicked();
		}

		show();
	
		if (ihuconfig.getAutoHide())
			hide();

		timer->start(STAT_TIME, false);
	}
	catch (Error e)
	{
		showMessageCritical(e.getText());
		throw e;
	}

}

void Ihu::applySettings()
{
	phone->setMyName(ihuconfig.getMyName());
	phone->resize(ihuconfig.getMaxCalls());

	if (ihuconfig.getTrayIcon())
	{
		if (trayIcon == NULL)
		{
			icon_status = IHU_ICON_NORMAL;
			trayIcon = new TrayIcon( this, "trayIcon", qPixmapFromMimeSource( "ihu_tray.png" ), "IHU" );
                        trayIcon->setContextMenu(appendTrayMenu());
			trayIcon->show();
			connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
				this, SLOT(trayIconActionActivated(QSystemTrayIcon::ActivationReason)));
		}
	}
	else
	{
		if (trayIcon)
		{
			trayIcon->hide();
			delete trayIcon;
			trayIcon = NULL;
		}
	}
	
	int abr = ihuconfig.getABR()*1000;
	int vbr = ihuconfig.getBitrateMode();
	switch (vbr)
	{
		case 0:
				abr = 0;
				break;
		case 1:
				abr = 0;
				break;
		case 2:
				vbr = 0;
				break;
	}
	float vbrquality = (float) ihuconfig.getVBRQuality();
	int dtx = ihuconfig.getDTX();
	int th = thSlider->minValue();
	if (dtx)
	{
		thSlider->setEnabled(TRUE);
		threshold->setEnabled(TRUE);
		th = ihuconfig.getThreshold();
	}
	else
	{
		thSlider->setEnabled(FALSE);
		threshold->setEnabled(FALSE);
	}
	thSlider->setValue(th);

	phone->setupRecorder(ihuconfig.getInputDriver(), ihuconfig.getInputInterface());
	phone->setupPlayer(ihuconfig.getOutputInterface(), ihuconfig.getPrePackets());
	playerDialog->setupPlayer(ihuconfig.getOutputInterface());

	phone->setup(ihuconfig.getSpeexMode(), ihuconfig.getQuality(), abr, vbr, vbrquality, ihuconfig.getComplexity(), ihuconfig.getVAD(), dtx, ihuconfig.getTXStop(), th, ihuconfig.getRingVolume());

	adrRefresh(ihuconfig.getADR());
	adrAction->setOn(ihuconfig.getADR());

	try
	{
		agcRefresh(ihuconfig.getAGC());
		agcAction->setOn(ihuconfig.getAGC());
	}
	catch (Error e)
	{
		agcAction->setOn(FALSE);
		ihuconfig.setAGC(false);
		showMessageCritical(e.getText());
	}

	try
	{
		logger->enable(ihuconfig.getLogFile());
	}
	catch (Error e)
	{
		showMessageCritical(e.getText());
	}

	autoanswer = ihuconfig.getAutoAnswer();
	autocrypt = ihuconfig.getCrypt();
	showkey = ihuconfig.getShowKey();
}

void Ihu::waitForCalls()
{
	waitButton->setOn(!waitButton->isOn());
	waitButtonClicked();
}

void Ihu::waitButtonClicked()
{
	if (waitButton->isOn())
	{
		try
		{
			phone->waitCalls(ihuconfig.getInPort(), ihuconfig.getUDP(), ihuconfig.getTCP());
			waitButton->setIconSet( QIcon( qPixmapFromMimeSource( "phone.png" ) ) );
			waitButton->setText( tr( "&Waiting for calls" ) );
			logViewer->addLog(logger->logStartReceiver());
		}
		catch (Error e)
		{
			abortWait(e.getText());
		}
	}
	else
	{
		phone->stopWaiting();
		waitButton->setIconSet( QIcon( qPixmapFromMimeSource( "phone_no.png" ) ) );
		waitButton->setText( tr( "&Wait for calls" ) );
		logViewer->addLog(logger->logStopReceiver());
	}
}

void Ihu::newCall()
{
	try
	{
		int id = phone->createCall();
		newCall(id);
	}
	catch (Error e)
	{
		abortCall(e.getCallId(), e.getText());
	}
}

void Ihu::newCall(int id)
{
	if (id >= 0)
	{
		QString callName = "Default";
		if (id > 0)
			callName = QString("Call%1").arg(id+1);
		callTab[id] = new CallTab(id, ihuconfig.getHosts(), ihuconfig.getMaxHosts(), callWidget, callName);
		connect( callTab[id], SIGNAL(callSignal(int, QString)), this, SLOT(call(int, QString)) );
		connect( callTab[id], SIGNAL(ringSignal(int, bool)), this, SLOT(ring(int, bool)) );
		connect( callTab[id], SIGNAL(answerSignal(int)), this, SLOT(answerCall(int)) );
		connect( callTab[id], SIGNAL(stopSignal(int)), this, SLOT(stopCall(int)) );
		connect( callTab[id], SIGNAL(muteMicSignal(int, bool)), this, SLOT(muteMic(int, bool)) );
		connect( callTab[id], SIGNAL(muteSpkSignal(int, bool)), this, SLOT(muteSpk(int, bool)) );
		callWidget->addTab((QWidget *)callTab[id], QIcon( qPixmapFromMimeSource( "phone.png" ) ), callName);
		if (autocrypt)
			crypt(id, true);
		message("New call created.");
	}
}

void Ihu::closeCall()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	int callId = current->getCallId();
	if (callId > 0)
	{
		phone->endCall(callId);
		callWidget->removePage(callWidget->currentPage());
		phone->deleteCall(callId);
		delete callTab[callId];
		callTab[callId] = NULL;
		message("Call closed.");
	}
}

void Ihu::call(QString host)
{
	call(0, host);
}

void Ihu::call(int id, QString host)
{
	try
	{
		if (host.isEmpty())
			throw Error(id, tr("No host specified!"));
		int callPort = ihuconfig.getOutPort();
		QString callHost = host;
		int tmpInd = host.findRev(':');
		if (tmpInd > 0)
		{
			callHost = host.left(tmpInd);
			callPort = host.right(host.length() - tmpInd - 1).toInt();
		}
		phone->call(id, callHost, callPort, ihuconfig.getProtocol());
		logViewer->addLog(logger->logOutgoingCall(host, phone->getCallerIp(id)));
		if(callTab[id])
		{
			callTab[id]->setRingButton(TRUE);
			callTab[id]->startCall();
			callWidget->setTabIconSet(callTab[id], qPixmapFromMimeSource( "ihu_talk.png" ) );
		}
		ihuconfig.addHost(host);
	}
	catch (Error e)
	{
		abortCall(e.getCallId(), e.getText());
	}
}

void Ihu::answerCall(int id)
{
	try
	{
		phone->answerCall(id);
		if (callTab[id])
			callTab[id]->startCall();
	}
	catch (Error e)
	{
		abortCall(id, e.getText());
	}
}

void Ihu::receivedCall(int id)
{
	if (callTab[id])
	{
		callTab[id]->receivedCall(phone->getCallerIp(id));
		callWidget->setTabIconSet(callTab[id], qPixmapFromMimeSource( "ihu_alarm.png" ) );
		if (autoanswer)
		{
			callTab[id]->startCall();
			answerCall(id);
		}
	}
	logViewer->addLog(logger->logReceivedCall(phone->getCallerIp(id), phone->getCallerName(id)));
}

void Ihu::connectedCall(int id)
{
	if (callTab[id])
	{
		callTab[id]->connectedCall();
		callTab[id]->message(QString("Connected with %1 (%2)").arg(phone->getCallerName(id)).arg(phone->getCallerIp(id)));
		callWidget->setTabIconSet(callTab[id], qPixmapFromMimeSource( "ihu_talk.png" ) );
	}
	logViewer->addLog(logger->logConnectedCall(phone->getCallerIp(id), phone->getCallerName(id)));
}

void Ihu::cancelCall(int id)
{
	if (callTab[id])
	{
		callTab[id]->stopCall();
		callWidget->setTabIconSet(callTab[id], qPixmapFromMimeSource( "phone.png" ) );
	}
	logViewer->addLog(logger->logStop(phone->getCallerIp(id), phone->getCallRX(id), phone->getCallTX(id)));
}

void Ihu::stopCall(int id)
{
	phone->endCall(id);
	if (callTab[id])
	{
		callTab[id]->message("Closing communication...");
		callWidget->setTabIconSet(callTab[id], qPixmapFromMimeSource( "phone.png" ) );
	}
}

void Ihu::abortCall(int id, QString text)
{
	if (id>=0)
	{
		if (callTab[id])
			callTab[id]->abortCall();
		logViewer->addLog(logger->logAbortedCall(phone->getCallerIp(id), text));
	}
	showMessageCritical(text);
}

void Ihu::abortWait(QString text)
{
	raise();
	waitButton->setOn(FALSE);
	waitButtonClicked();
	showMessageCritical(text);
}

void Ihu::abortAll(QString text)
{
	raise();
	phone->abortAll();
	showMessageCritical(text);
}

void Ihu::showMessageCritical(QString text)
{
	QMessageBox::critical(0, "IHU Error", QString("Error: %1").arg(text));
	message("Aborted");
}

void Ihu::showWarning(QString text)
{
	QString msg = QString("Warning: %1").arg(text); 
	QMessageBox::warning(0, "IHU Warning", msg);
	message(msg);
}

void Ihu::closeEvent(QCloseEvent *e)
{
	if (!trayIcon || closing)
	{
		e->accept();
	}
	else
	{
		e->ignore();
		hide();
	}
}

void Ihu::quit()
{
	phone->endAll();
	phone->stopWaiting();

	timer->stop();

	closing = true;

	close();
}

void Ihu::message(QString text)
{
	skipStat = 2;
	statusbar->message(text, STAT_TIME*3);
}

void Ihu::message(int id, QString text)
{
	if (callTab[id])
		callTab[id]->message(text);
}

void Ihu::warning(QString text)
{
	message(text);
	logViewer->addLog(logger->log(text));
}

void Ihu::warning(int id, QString text)
{
	QString newMsg = QString("%1 - %2").arg(phone->getCallerIp(id)).arg(text);
	message(newMsg);
	logViewer->addLog(logger->log(newMsg));
}

void Ihu::showLogViewer()
{
	logViewer->show();
}

void Ihu::settings()
{
	Settings *settings = new Settings(ihuconfig, this);
	int ret = settings->exec();
	if (ret == QDialog::Accepted)
	{
		applySettings();
		if (phone->isListening())
		{
			phone->stopWaiting();
			waitButtonClicked();
		}
		try
		{
			ihuconfig.writeConfig();
			message("Settings updated.");
		}
		catch (Error e)
		{
			showWarning(e.getText());
		}
	}
	delete settings;
}

void Ihu::ring(int id, bool on)
{
	phone->ring(id, on);
}

void Ihu::muteMic(int id, bool on)
{
	phone->muteRecorder(id, on);
}

void Ihu::muteSpk(int id, bool on)
{
	phone->mutePlayer(id, on);
}

void Ihu::playFile(QString fileName)
{
	showPlayer();
	playerDialog->playFile(fileName);
}

void Ihu::cryptOn()
{
	cryptAction->setOn(TRUE);
	crypt();
	autocrypt = true;
}

void Ihu::crypt()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	crypt(current->getCallId(), cryptAction->isOn());
}

void Ihu::crypt(int id, bool on)
{
	bool ok = true;
	if (on)
	{
		if (!ihuconfig.getRandom())
		{
			if (ihuconfig.getPasswd().isEmpty())
			{
				ok = changeKey(id);
			}
			else
			{
				QString text = ihuconfig.getPasswd();
				phone->enableCrypt(id, (char *) text.ascii(), text.length());
			}
		}
		else
		{
			int len = ihuconfig.getKeyLen()/8;
			phone->enableRandomCrypt(id, len);
		}
		if (ok)
		{
			if (callTab[id])
				callTab[id]->crypt(true);
			message(tr("Encryption enabled."));
		}
	}
	else
	{
		phone->disableCrypt(id);
		if (callTab[id])
			callTab[id]->crypt(false);
		message(tr("Encryption disabled."));
	}
}

void Ihu::toCrypt(int id)
{
	if (!ihuconfig.getRandom() && !ihuconfig.getPasswd().isEmpty())
	{
		QString text = ihuconfig.getPasswd();
		phone->enableCrypt(id, (char *) text.ascii(), text.length());
	}
	else
	{
		int len = ihuconfig.getKeyLen()/8;
		phone->enableRandomCrypt(id, len);
	}
	if (callTab[id])
		callTab[id]->crypt(true);
}

void Ihu::changeKey()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	changeKey(current->getCallId());
}

bool Ihu::changeKey(int id)
{
	bool ok = false;
	try
	{
		if (ihuconfig.getRandom())
		{
			int len = ihuconfig.getKeyLen()/8;
			phone->enableRandomCrypt(id, len);
			ok = true;
		}
		else
		{
			QString callName = "";
			if (callTab[id])
				callName = callTab[id]->getCallName();
			QString text = QInputDialog::getText(QString("%1: Encryption passphrase change").arg(callName), QString("%1: Enter the encryption passphrase (max 56 chars, please use at least 30 chars):").arg(callName), QLineEdit::Password, QString::null, &ok, this );
			if (ok)
			{
				ok = false;
				if (!text.isEmpty())
				{
					phone->enableCrypt(id, (char *) text.ascii(), text.length());
					ok = true;
				}
			}
		}
		if (ok)
		{
			message(tr("Encryption key changed successfully."));
			if (callTab[id])
				callTab[id]->crypt(true);
		}
	}
	catch (Error e)
	{
		abortCall(id, e.getText());
	}
	return ok;
}

void Ihu::setDecryptionKey()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	setDecryptionKey(current->getCallId());
}

void Ihu::setDecryptionKey(int id)
{
	bool ok;
	QString callName = "";
	if (callTab[id])
		callName = callTab[id]->getCallName();
	QString text = QInputDialog::getText(QString("%1: Decryption Passphrase").arg(callName), QString("%1: Enter the decryption passphrase, leave blank to reset.").arg(callName), QLineEdit::Password, QString::null, &ok, this );
	if (ok)
	{
		phone->disableDecrypt(id);
		if (!text.isEmpty())
		{
			phone->enableDecrypt(id, (char *) text.ascii(), text.length());
			message(tr("Decryption key changed successfully."));
		}
	}
}

void Ihu::agcRefresh(bool on)
{
	float step = (float) (ihuconfig.getAGCStep()/100.f);
	float level = (float) (ihuconfig.getAGCLevel());
	phone->setupAgc(on, ihuconfig.getAGCHw(), ihuconfig.getAGCSw(), step, level, ihuconfig.getAGCControl());
}

void Ihu::agcOn()
{
	bool on = agcAction->isOn();
	try
	{
		agcRefresh(on);
		QString tempmsg = on ? IHU_ENABLED : IHU_DISABLED;
		ihuconfig.setAGC(on);
		message(QString("Automatic Gain Control (AGC) %1.").arg(tempmsg));
	}
	catch (Error e)
	{
		agcAction->setOn(FALSE);
		showMessageCritical(e.getText());
	}
}

void Ihu::adrRefresh(bool on)
{
	float mindelay = (float) (ihuconfig.getADRMinDelay()/1000.f);
	float maxdelay = (float) (ihuconfig.getADRMaxDelay()/1000.f);
	float stretch = (float) ihuconfig.getADRStretch();
	phone->setupAdr(on, mindelay, maxdelay, stretch);
}

void Ihu::adrOn()
{
	bool on = adrAction->isOn();
	adrRefresh(on);
	QString tempmsg = on ? IHU_ENABLED : IHU_DISABLED;
	message(tr("Audio Delay Reduction (ADR) %1.").arg(tempmsg));
	ihuconfig.setADR(on);
}

void Ihu::recorderStatus(bool on)
{
	QString tempmsg = on ? IHU_DISABLED : IHU_ENABLED;
	try
	{
		if (on)
			muteMicButton->setIconSet( QIcon( qPixmapFromMimeSource( "mic_mute.png" ) ) );
		else
			muteMicButton->setIconSet( QIcon( qPixmapFromMimeSource( "mic.png" ) ) );
		phone->disableRecorder(on);
		message(QString("Audio input globally %1.").arg(tempmsg));
	}
	catch (Error e)
	{
		abortAll(e.getText());
	}
}

void Ihu::playerStatus(bool on)
{
	QString tempmsg = on ? IHU_DISABLED : IHU_ENABLED;
	try
	{
		if (on)
			muteSpkButton->setIconSet( QIcon( qPixmapFromMimeSource( "speaker_mute.png" ) ) );
		else 
			muteSpkButton->setIconSet( QIcon( qPixmapFromMimeSource( "speaker.png" ) ) );
		phone->disablePlayer(on);
		message(QString("Audio output globally %1.").arg(tempmsg));
	}
	catch (Error e)
	{
		abortAll(e.getText());
	}
}

void Ihu::thSliderChanged(int val)
{
	phone->setThreshold(val);
	threshold->setText(QString("TX threshold: %1 dB").arg( val ));
	ihuconfig.setThreshold(val);
}

void Ihu::enableRecorder(bool on)
{
	soundLabel->setEnabled(on);
	soundLevel->setEnabled(on);
}

void Ihu::enablePlayer(bool on)
{
	delayLabel->setEnabled(on);
}

void Ihu::statistics()
{
	QString tempMsg;
	float traffic;
	float total = 0.f;

	for (int i=0; i<maxcalls; i++)
	{
		if (callTab[i])
		{
			traffic = (float) phone->getCallTraffic(i);
			traffic /= STAT_TIME;
			total += traffic;
			callTab[i]->statistics(traffic, phone->getCallerName(i));
			callTab[i]->ledEnable(phone->isTXActive(i), phone->isRXActive(i));
		}
	}

	QString statTraffic = QString("%1 KB/s").arg(total, 2, 'f', 1 );
	trafficLabel->setText(statTraffic);

	tempMsg = QString("Active calls: %1 - Missed: %2").arg(phone->getCalls()).arg(phone->getConnections());
	if (phone->isListening())
		tempMsg += QString(" (Waiting for calls)");

	if (skipStat > 0)
		skipStat--;
	else
		statusbar->message(tempMsg);

	int peak = phone->getPeak();
	soundLabel->setText( QString( "Recording Level: %1 dB" ).arg(peak) );
	if (peak <= -96)
		peak = 0;
	else
	if (peak >= 0)
		peak = 100;
	else
		peak = (peak+96);
	soundLevel->setProgress(peak);

	delayLabel->setText( QString("%1 s").arg(phone->getDelay(), 3, 'f', 2 ) );

	if (trayIcon)
	{
		if (phone->getConnections() > 0)
			changeTrayIcon(IHU_ICON_ALARM);
		else
		if (phone->getCalls() > 0)
			changeTrayIcon(IHU_ICON_TALK);
		else
		if (phone->isListening())
			changeTrayIcon(IHU_ICON_WAIT);
		else
			changeTrayIcon(IHU_ICON_NORMAL);
		trayIcon->setToolTip( QString ("IHU: %1 active calls - %2").arg(phone->getCalls()).arg(statTraffic) );
	}
}

void Ihu::helpContents()
{
	QMessageBox::information( this, "Help on IHU",
	"Please read the manual inside the IHU package, or visit:\n\n"
	"http://ihu.sourceforge.net/doc/manual.html");
}

void Ihu::disableIn()
{
	muteMicButton->toggle();
}

void Ihu::disableOut()
{
	muteSpkButton->toggle();
}

void Ihu::toggleVisibility()
{
	if (isVisible())
		hide();
	else
		show();
}

QMenu *Ihu::appendTrayMenu()
{
	QMenu* trayMenu = new QMenu(this);
	trayMenu->setCheckable(TRUE);

	for (int i=0; i<maxcalls; i++)
	{
		if (callTab[i])
		{
			QMenu* callMenu = new QMenu(trayMenu);
			callMenu->insertItem( QIcon( qPixmapFromMimeSource( "call.png" ) ), callTab[i]->getCallButtonText(), callTab[i], SLOT( callButtonClicked() ), 0, 1 );
			callMenu->setItemEnabled(1, callTab[i]->isCallButtonEnabled());
			callMenu->insertItem( QIcon( qPixmapFromMimeSource( "hangup.png" ) ), callTab[i]->getStopButtonText(), callTab[i], SLOT( stopButtonClicked() ), 0, 2 );
			callMenu->setItemEnabled(2, callTab[i]->isStopButtonEnabled());
			trayMenu->insertItem( callWidget->tabIconSet(callTab[i]), callWidget->tabLabel(callTab[i]), callMenu );
		}
	}

	trayMenu->insertSeparator();
	trayMenu->insertItem( waitButton->text() , this, SLOT( waitForCalls() ), 0, 3);
	trayMenu->setItemChecked(3, waitButton->isOn());

	trayMenu->insertSeparator();
	trayMenu->insertItem( tr("&Quit"), this, SLOT( quit() ) );
	return trayMenu;
}

void Ihu::changeTrayIcon(icon_type newicon)
{
	if (trayIcon)
	{
		if (newicon != icon_status)
		{
			icon_status = newicon;
			switch(icon_status)
			{
				case IHU_ICON_NORMAL:
					trayIcon->setIcon( QIcon( qPixmapFromMimeSource( "ihu_tray.png" ) ) );
					break;
				case IHU_ICON_WAIT:
					trayIcon->setIcon( QIcon( qPixmapFromMimeSource( "ihu_wait.png" ) ) );
					break;
				case IHU_ICON_ALARM:
					trayIcon->setIcon( QIcon( qPixmapFromMimeSource( "ihu_alarm.png" ) ) );
					break;
				case IHU_ICON_TALK:
					trayIcon->setIcon( QIcon( qPixmapFromMimeSource( "ihu_talk.png" ) ) );
					break;
			}
		}
	}
}

void Ihu::trayIconActionActivated(const QSystemTrayIcon::ActivationReason &reason)
{
	switch( reason )
	{
		case QSystemTrayIcon::Trigger:
			toggleVisibility();
			break;
		default:
			;
	}
}

void Ihu::receivedNewKey(int callId, QString text)
{
	if (showkey)
	{
		QString keyMsg = QString("New decryption key: %1").arg(text);
		message(callId, keyMsg);
		logViewer->addLog(logger->log(QString("%1 (%2) - %3").arg(phone->getCallerIp(callId)).arg(phone->getCallerName(callId)).arg(keyMsg)));
	}
}

void Ihu::dumpRXEnable()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	int callId = current->getCallId();
	QString filename = "";
	if (dumpRXAction->isOn())
	{
		filename = getFileName();
		if (filename.isEmpty())
			dumpRXAction->setOn(FALSE);
		else
			message(QString("Recording RX stream to %1").arg(filename));
	}
	else
		message(QString("Recording RX stream disabled"));
	phone->dumpRXStream(callId, filename);
}

void Ihu::dumpTXEnable()
{
	CallTab *current = (CallTab *) callWidget->currentPage();
	int callId = current->getCallId();
	QString filename = "";
	if (dumpTXAction->isOn())
	{
		filename = getFileName();
		if (filename.isEmpty())
			dumpTXAction->setOn(FALSE);
		else
			message(QString("Recording TX stream to %1").arg(filename));
	}
	else
		message(QString("Recording TX stream disabled"));
	phone->dumpTXStream(callId, filename);
}

QString Ihu::getFileName()
{
	QString name=Q3FileDialog::getSaveFileName("","*"IHU_EXT, this, 0, "Save stream to file...");
	if (!name.isEmpty())
	{
		if (!name.endsWith(IHU_EXT))
			name += IHU_EXT;
	}
	return name;
}

void Ihu::menuUpdate()
{
	skipStat = 1;
	CallTab *current = (CallTab *) callWidget->currentPage();
	int callId = current->getCallId();
	cryptAction->setOn(current->isCrypted());
	changeKeyAction->setEnabled(current->isCrypted());
	dumpRXAction->setOn(phone->isDumpingRX(callId));
	dumpTXAction->setOn(phone->isDumpingTX(callId));
	if (callId > 0)
		closeCallAction->setEnabled(TRUE);
	else
		closeCallAction->setEnabled(FALSE);
}

void Ihu::showPlayer()
{
	playerDialog->show();
}

void Ihu::clearHistory()
{
	for (int i=0; i<maxcalls; i++)
	{
		if (callTab[i])
			callTab[i]->clearHosts();
	}
	ihuconfig.clearHosts();
	message("Host History Cleared");
}

void Ihu::clearMissed()
{
	phone->clearConnections();
}

void Ihu::helpAbout()
{
	QMessageBox::about( this, "About IHU", "I  Hear  U  v. "VERSION" ("VDATE") written by Matteo Trotta\n\n"
	"Copyright (C) 2003-2008 Matteo Trotta. All rights reserved.\n"
	"Distributed under the terms of GNU General Public License.\n\n"
	"Using Speex - Copyright (C) Jean-Marc Valin\n"
	"Using SoundTouch - Copyright (C) Olli Parviainen\n\n"
	"Visit home page at http://ihu.sourceforge.net\n"
	);
}
