//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: main_window.cpp                       //
//                                              //
// Dieser Sourcecode ist Copyright gesch�tzt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte �nderung am 16.02.2013                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "main_window.h"
#include "ui_main_window.h"
#include "version.h"

#include <QDebug>
#include <QDir>

MainWindow::MainWindow(QWidget *parent,customSplashScreen* _splash,QTextStream *_log) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    c64(0)
{
    splash = _splash;
    log = _log;

    /// Haputfenster UI setzen ///
    ui->setupUi(this);

#ifdef _WIN32
    setWindowTitle("Emu64 Version " + QString(str_emu64_version) + " --- [Windows]");
#endif

#ifdef __linux__
    setWindowTitle("Emu64 Version " + QString(str_emu64_version) + " --- [Linux]");
#endif
}

MainWindow::~MainWindow()
{
    showNormal();
    //////////// Save to INI ////////////
    if(ini != NULL)
    {
        ini->beginGroup("MainWindow");
        ini->setValue("Geometry",saveGeometry());
        ini->setValue("State",saveState());
        ini->setValue("ScreenshotCounter",ScreenshotNumber);
        ini->endGroup();

        char group_name[32];
        bool value;

        for(int i=0; i<FloppyAnzahl; i++)
        {
            sprintf(group_name,"Floppy1541_%2.2X",i+8);
            ini->beginGroup(group_name);
            WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(i,0);
            value = w->GetEnableFloppy();
            ini->setValue("Enabled",value);
            ini->setValue("VolumeMode",w->GetFloppyVolume());
            ini->endGroup();
        }
    }
    /////////////////////////////////////

    /// WindowKlassen schlie�en ///

    delete setup_window;
    delete c64;

    delete videopal;
    delete info_window;
    delete tv_setup_window;
    delete floppy_window;
    delete c64_keyboard_window;
    delete crt_window;
    delete debugger_window;
    delete speed_window;

    delete ui;
    delete ini;
    LogText(tr(">> Es wurden alle Klassen wieder entfernt\n").toLatin1().data());

    LogText(tr("\n>> Emu64 wurde sauber beendet...").toLatin1().data());
    delete log;
}

void MainWindow::OnInit()
{
    splash->setMessageRect(QRect(0,303,400, 20), Qt::AlignCenter);

    QFont splashFont;
    splashFont.setFamily("Arial");
    splashFont.setBold(true);
    splashFont.setPixelSize(10);
    splashFont.setStretch(125);

    splash->setFont(splashFont);

    /// ApplicationPath holen und abspeichern ///
    appPath = QApplication::applicationDirPath();

    /// Translator installieren ///
    langPath = appPath+"/languages";
    qApp->installTranslator(&qtTranslator);
    qApp->installTranslator(&appTranslator);
    QString SystemLocale = QLocale::system().name();       // "de_DE"
    SystemLocale.truncate(SystemLocale.lastIndexOf('_'));  // "de"

    /// INI Dateiverwaltung erstellen ///
    splash->showStatusMessage(tr("INI Dateiverwaltung wird initialisiert."),Qt::darkBlue);
    ini = new QSettings(appPath+"/emu64.ini",QSettings::IniFormat,this);
    LogText(QString(">> INI System wurde erzeugt: " + appPath+"/emu64.ini\n").toLatin1().data());
    LogText(QString(">> INI System was created: " + appPath+"/emu64.ini\n").toLatin1().data());

    splash->showStatusMessage(tr("Sprachmen� wir erstellt."),Qt::darkBlue);
    ini->beginGroup("MainWindow");
    CreateLanguageMenu(ini->value("lang",SystemLocale).toString());
    ini->endGroup();
    LogText(QString(tr(">> Sprachmen� wurde erstellt.") + SystemLocale + "\n").toLatin1().data());


    splash->showStatusMessage(tr("Translator wurde installiert."),Qt::darkBlue);
    LogText(QString(tr(">> Translator wurde intsalliert: Systemsprache = ") + SystemLocale + "\n").toLatin1().data());
    setWindowIcon(QIcon(":/grafik/emu64.ico"));

    /// ScreenshotPath erstellen ///
    screenshotPath = appPath + "/screenshots/";

    QDir* dir = new QDir(screenshotPath);

    splash->showStatusMessage(tr("Screenshotverzeichnis wird gesucht."),Qt::darkBlue);
    if(dir->exists())
    {
        LogText(tr(">> Ein Screenshot Verzeichnis ist vorhanden\n").toLatin1().data());
        ScreenshotsEnable = true;
    }
    else
    {
        splash->showStatusMessage(tr("Screenshotverzeichnis wird erstellt."),Qt::darkBlue);
        LogText(tr("<< Ein Screenshot Verzeichnis ist nicht vorhanden\n").toLatin1().data());
        if(dir->mkdir(screenshotPath))
        {
            LogText(tr(">> Ein neues Screenshot Verzeichnis wurde erstellt\n").toLatin1().data());
            ScreenshotsEnable = true;
        }
        else
        {
            LogText(tr("<< Ein neues Screenshot Verzeichnis konnte nicht erstellt werden\n<< Keine Screenshots m�glich !!").toLatin1().data());
            ScreenshotsEnable = false;
        }
    }

    if(ScreenshotsEnable)
    {
        LogText(">> screenshotPath = ");
        LogText(screenshotPath.toLatin1().data());
        LogText("\n");
    }

    /// Klassen installieren ///
    splash->showStatusMessage(tr("VideoPal Klasse wird initialisiert."),Qt::darkBlue);
    videopal = new VideoPalClass();
    LogText(tr(">> VideoPal Klasse wurde installiert\n").toLatin1().data());

    /// C64 Klasse Installieren ... Das HERZ ///
    splash->showStatusMessage(tr("C64 Klasse wird initialisiert."),Qt::darkBlue);
    int ret_error;
    c64 = new C64Class(&ret_error,videopal,true,bind(&MainWindow::LogText,this,_1),QString(appPath + "/gfx/").toLatin1().data());
    if(ret_error != 0)
    {
        ErrorMsg(tr("Emu64 Fehler ..."),tr("Fehler beim Installieren der C64 Klasse"))
        this->close();
    }

    /// Window Klassen erstellen ///
    /// Unter MAC sollte ohne �bergabe des this Zeigers die Klasseb erstellt werden

    splash->showStatusMessage(tr("InfoWindow wird erstellt."),Qt::darkBlue);
    info_window = new InfoWindow(this);
    LogText(tr(">> InfoWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("TVSetupWindow wird erstellt."),Qt::darkBlue);
    tv_setup_window = new TVSetupWindow(this,c64,videopal,ini);
    LogText(tr(">> TVSetupWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("FloppyWindow wird erstellt."),Qt::darkBlue);
    floppy_window = new FloppyWindow(this,ini);
    LogText(tr(">> FloppyWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("C64KeyboardWindow wird erstellt."),Qt::darkBlue);
    c64_keyboard_window = new C64KeyboardWindow(this,ini);
    LogText(tr(">> C64KeyboardWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("CRTWindow wird erstellt."),Qt::darkBlue);
    crt_window = new CrtWindow(this,ini);
    LogText(tr(">> CrtWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("DebuggerWindow wird erstellt."),Qt::darkBlue);
    debugger_window = new DebuggerWindow(this,ini);
    LogText(tr(">> DebuggerWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("SetupWindow wird erstellt."),Qt::darkBlue);
    setup_window = new SetupWindow(this,SLOT(onChangeGrafikModi(bool,bool,bool,bool,bool)),videopal,ini);
    LogText(tr(">> SetupWindow wurde erzeugt\n").toLatin1().data());

    splash->showStatusMessage(tr("C64SpeedWindow wird erstellt."),Qt::darkBlue);
    speed_window = new C64SpeedWindow(this,ini);
    LogText(tr(">> C64SpeedWindow wurde erzeugt\n").toLatin1().data());

    ini->beginGroup("MainWindow");
    splash->showStatusMessage(tr("Screenshotnummer wir geladen."),Qt::darkBlue);
    ScreenshotNumber = (int)ini->value("ScreenshotCounter",0).toInt();
    ini->endGroup();



    splash->showStatusMessage(tr("SetupWindow wird mit INI abgeglichen."),Qt::darkBlue);
    setup_window->ReSetup();

    splash->showStatusMessage(tr("SDL Window Titelleiste wird benannt."),Qt::darkBlue);
    SDL_WM_SetCaption((const char*)tr("C64 Bildschirm").toLatin1().data(),0);

    /// Debugger Window mit C64 verbinden ///
    splash->showStatusMessage(tr("Debugger Window wird mit C64 Klasse verbunden."),Qt::darkBlue);
    debugger_window->SetC64Pointer(c64);

    /// C64 Speed Window mit C64 verbinden ///
    splash->showStatusMessage(tr("C64 Speed Window wird mit C64 Klasse verbunden."),Qt::darkBlue);
    speed_window->SetC64Pointer(c64);

    /// CRT Klasse mit CRT Window verbinden ///
    splash->showStatusMessage(tr("CRT Window wird mit CRT Klasee verbunden."),Qt::darkBlue);
    crt_window->crt = c64->crt;
    crt_window->c64 = c64;
    c64->crt->ChangeLED = bind(&CrtWindow::ChangeLED,crt_window,_1,_2);

    /// C64 Systemroms laden ///
    splash->showStatusMessage(tr("C64 Systemroms werden geladen."),Qt::darkBlue);
    if(!c64->LoadC64Roms((char*)QString(appPath+"/roms/kernal.rom").toLatin1().data(),(char*)QString(appPath+"/roms/basic.rom").toLatin1().data(),(char*)QString(appPath+"/roms/char.rom").toLatin1().data()))
    {
        LogText((char*)"<< ERROR: Fehler beim laden der C64 Roms\n\t");
        LogText(QString(appPath+"/roms/kernal.rom").toLatin1().data());LogText("\n\t");
        LogText(QString(appPath+"/roms/basic.rom").toLatin1().data());LogText("\n\t");
        LogText(QString(appPath+"/roms/char.rom").toLatin1().data());LogText("\n");
    }

    /// C64 Keyboard Matrix mit dem Virtual Keyboard verbinden ///
    splash->showStatusMessage(tr("C64 Key-Matrix wird mit Virtual Keyboard verbunden."),Qt::darkBlue);
    c64_keyboard_window->KeyMatrixToPA = c64->KeyboardMatrixToPAExt;
    c64_keyboard_window->KeyMatrixToPB = c64->KeyboardMatrixToPBExt;

    /// Tabelle f�r Floppy's Ertsellen ///
    splash->showStatusMessage(tr("Tabelle f�r Floppy's wird erstellt."),Qt::darkBlue);
    ui->FloppyTabel->setRowCount(FloppyAnzahl);
    ui->FloppyTabel->setColumnCount(1);

    for(int i=0; i<FloppyAnzahl; i++)
    {
        splash->showStatusMessage(tr("Floppy: ") + QVariant(i).toString() + "wird in Tabelle eingef�gt",Qt::darkBlue);
        ui->FloppyTabel->setRowHeight(i,24);
        WidgetFloppyStatus *w = new WidgetFloppyStatus(this,i,c64->floppy[i]);
        w->SetGeraeteID(i+8);
        ui->FloppyTabel->setCellWidget(i,0,w);
        ui->FloppyTabel->setColumnWidth(0,w->width());
        connect(w,SIGNAL(ChangeEnableFloppy()),debugger_window,SLOT(onChangeFloppyStatus()));
        connect(w,SIGNAL(LoadImage(int)),floppy_window,SLOT(OnChangeFloppyNummer(int)));
        connect(w,SIGNAL(RemoveImage(int)),floppy_window,SLOT(OnRemoveImage(int)));
    }

    connect(floppy_window,SIGNAL(ChangeFloppyImage(int)),this,SLOT(OnChangeFloppyImage(int)));

    ////////// Load from INI ///////////
    splash->showStatusMessage(tr("Einstellungen werden von INI geladen und gesetzt."),Qt::darkBlue);
    if(ini != NULL)
    {
        floppy_window->LoadIni();

        ini->beginGroup("MainWindow");
        if(ini->contains("Geometry")) restoreGeometry(ini->value("Geometry").toByteArray());
        if(ini->contains("State")) restoreState(ini->value("State").toByteArray());
        ini->endGroup();

        char group_name[32];

        for(int i=0; i<FloppyAnzahl; i++)
        {
            sprintf(group_name,"Floppy1541_%2.2X",i+8);
            ini->beginGroup(group_name);
            WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(i,0);

            w->SetAktFilename(floppy_window->GetAktFilename(i),floppy_window->GetAktD64Name(i));
            c64->LoadDiskImage(i,floppy_window->GetAktFilename(i).toLatin1().data());

            w->SetEnableFloppy(ini->value("Enabled",false).toBool());
            w->SetFloppyVolume(ini->value("VolumeMode",2).toInt());
            ini->endGroup();
        }

        /// SETUP Ini laden ///
        setup_window->LoadINI(c64);

        /// CRT Ini erst jetzt laden ///
        crt_window->LoadIni();
    }
    this->update();

    /// Screenshot Reset mit SetupWindow verbinden ///
    connect(setup_window,SIGNAL(ResetSreenshotCounter()),this,SLOT(OnResetScreenshotCounter()));

    /////////////////////////////////////
    splash->showStatusMessage(tr("C64 EMULATION WIRD NUN GESTARET."),Qt::darkBlue);
    if(ret_error == 0) c64->StartEmulation();
    else
    {
        this->close();
        return;
    }
    c64->HardReset();

    splash->close();
    this->show();
}

void MainWindow::OnMessage(QStringList msg)
{
    /*
    for(int i=0;i<msg.length();i++)
        QMessageBox::information(this,"Komandozeile",msg[i]);
    */
}

void MainWindow::LogText(char *log_text)
{
    if(log != NULL)
    {
        *log << log_text;
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LocaleChange)
    {
      QString locale = QLocale::system().name();
      locale.truncate(locale.lastIndexOf('_'));

      qtTranslator.load("qt_" + locale, langPath);
      appTranslator.load("emu64_" + locale, langPath);
      RetranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::CreateLanguageMenu(QString defaultLocale)
{
   QActionGroup* langGroup = new QActionGroup(ui->menuSprache);
   langGroup->setExclusive(true);

   connect(langGroup, SIGNAL(triggered(QAction *)),this, SLOT(slotLanguageChanged(QAction *)));

    QDir dir(langPath);
    QStringList fileNames;

    fileNames = dir.entryList(QStringList("emu64_*.qm"));

   for (int i = 0; i < fileNames.size(); ++i)
   {
      // get locale extracted by filename
      QString locale;
      locale = fileNames[i];                      // "emu64_de.qm"
      locale.truncate(locale.lastIndexOf('.'));   // "emu64_de"
      locale.remove(0, locale.indexOf('_') + 1);  // "de"

      QString lang = QLocale::languageToString(QLocale(locale).language());

      if(lang == "German") lang = "Deutsch";

      QAction *action = new QAction(lang, this);
      action->setCheckable(true);
      action->setData(locale);

      QString iconfile = fileNames[i];
      iconfile.truncate(iconfile.lastIndexOf('.'));
      iconfile = dir.filePath(iconfile + ".png");
      action->setIcon(QIcon(iconfile));
      action->setIconVisibleInMenu(true);
      action->setStatusTip(tr("Wechseln zur Sprache: ") + lang);

      ui->menuSprache->addAction(action);
      langGroup->addAction(action);

      // set default translators and language checked
      if (defaultLocale == locale)
      {
         appTranslator.load("emu64_" + action->data().toString(), langPath);
         qtTranslator.load("qt_" + action->data().toString(), langPath);
         action->setChecked(true);
         ui->retranslateUi(this);
      }
    }
}

void MainWindow::slotLanguageChanged(QAction* action)
{
   appTranslator.load("emu64_" + action->data().toString(), langPath);
   qtTranslator.load("qt_" + action->data().toString(), langPath);
   ini->beginGroup("MainWindow");
   ini->setValue("lang",action->data().toString());
   ini->endGroup();
   RetranslateUi();
}

void MainWindow::RetranslateUi()
{
    ui->retranslateUi(this);

    for(int i=0; i<FloppyAnzahl; i++)
    {
        WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(i,0);
        if(w != NULL) w->RetranslateUi();
    }

    info_window->RetranslateUi();
    tv_setup_window->RetranslateUi();
    c64_keyboard_window->RetranslateUi();
    floppy_window->RetranslateUi();
    crt_window->RetranslateUi();
    debugger_window->RetranslateUi();
    setup_window->RetranslateUi();
    speed_window->RetranslateUi();

    SDL_WM_SetCaption(tr("C64 Bildschirm").toLatin1().data(),0);
}

void MainWindow::on_menu_main_info_triggered()
{
    SDL_version compiled;
    SDL_VERSION(&compiled);

    QString sdl_version = QVariant(compiled.major).toString() + "." + QVariant(compiled.minor).toString() + "." + QVariant(compiled.patch).toString();
    QString emu64_version = QString("Emu64 V") + QString(str_emu64_version);

    info_window->SetEmu64VersionText(emu64_version);
    info_window->SetMoreInfoText("Qt Version: " + QString(qVersion()) + " / SDL Version: " + sdl_version);


    info_window->show();
}

void MainWindow::keyPressEvent(QKeyEvent*)
{
    /*
    for(int i=0;i<64;i++)
    {
        if(C64KeyTable[i].nativeVirtualKey == event->nativeVirtualKey())
        {
            c64->KeyEvent(C64KeyTable[i].MatrixCode,KEY_DOWN);
        }
    }
    */
}

void MainWindow::keyReleaseEvent(QKeyEvent*)
{
    /*
    for(int i=0;i<64;i++)
    {
        if(C64KeyTable[i].nativeVirtualKey == event->nativeVirtualKey())
        {
            c64->KeyEvent(C64KeyTable[i].MatrixCode,KEY_UP);
        }
    }
    */
}

void MainWindow::on_actionBeenden_triggered()
{
    close();
}

void MainWindow::on_actionSoftreset_triggered()
{
    c64->SoftReset();
}

void MainWindow::on_actionHardreset_triggered()
{
    c64->HardReset();
}

void MainWindow::on_actionAutostart_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,tr("C64 Dateien �ffnen "),"",tr("C64 Programm Dateien") + "(*.prg *.p00 *.t64 *.d64 *.g64 *.frz);;" + tr("Alle Dateien") + "(*.*)");
    if(filename != "")
    {
        c64->LoadAutoRun(0,filename.toLatin1().data());
    }
}

void MainWindow::on_actionC64_Programme_direkt_laden_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,tr("C64 Dateien �ffnen "),"",tr("C64 Programm Dateien") + "(*.prg *.p00 *.t64 *.frz);;" + tr("Alle Dateien") + "(*.*)");
    if(filename != "")
    {
        c64->LoadPRG(filename.toLatin1().data(),0);
    }
}


void MainWindow::on_actionTV_Video_Einstellungen_triggered()
{
    tv_setup_window->show();
}

void MainWindow::on_actionFloppy_1541_II_triggered()
{
    if(floppy_window->isHidden()) floppy_window->show();
    else floppy_window->hide();
}

void MainWindow::on_actionVirtuelle_C64_Tastatur_triggered()
{
    c64_keyboard_window->Recording = false;
    c64_keyboard_window->show();
    c64_keyboard_window->update();
}

void MainWindow::on_actionC64_Tastaturbelegung_ndern_triggered()
{
    c64_keyboard_window->Recording = true;
    c64_keyboard_window->show();
    c64_keyboard_window->update();
}

void MainWindow::on_actionSteckmodule_triggered()
{
    if(crt_window->isHidden()) crt_window->show();
    else crt_window->hide();
}

void MainWindow::on_actionDebugger_Disassembler_triggered()
{
    if(debugger_window->isHidden()) debugger_window->show();
    else debugger_window->hide();
}

void MainWindow::on_actionEmu64_Einstellungen_triggered()
{
    if(setup_window->isHidden())
    {
        c64->JoystickNewScan();
        setup_window->show();
    }
    else setup_window->hide();
}

void MainWindow::onChangeGrafikModi(bool fullscreen, bool palmode, bool doublemode, bool bit32mode, bool filter)
{
    if(!fullscreen)
    {
        if(c64 != 0) c64->SetGrafikModi(bit32mode,doublemode,palmode,filter,0,0);
    }
}

void MainWindow::OnChangeFloppyImage(int floppynr)
{
    WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(floppynr,0);
    w->SetAktFilename(floppy_window->GetAktFilename(floppynr),floppy_window->GetAktD64Name(floppynr));
    c64->LoadDiskImage(floppynr,floppy_window->GetAktFilename(floppynr).toLatin1().data());
}

void MainWindow::OnResetScreenshotCounter()
{
    ScreenshotNumber = 0;
}

void MainWindow::on_actionScreenshot_triggered()
{
    if(ScreenshotsEnable)
    {
        c64->SaveScreenshot(QString(screenshotPath + "emu64_" + QVariant(ScreenshotNumber).toString() + ".bmp").toLatin1().data());
        ScreenshotNumber++;
    }
    else QMessageBox::critical(this,tr("Emu64 Fehler ..."),tr("Es sind keine Screenshots m�glich da Emu64 kein Screenshot Verzeichnis anlegen konnte.\n�berpr�fen Sie bitte die Rechte des Emu64 Verzeichnisses !"));
}

void MainWindow::on_actionC64_Geschwindigkeit_triggered()
{
    if(speed_window->isHidden()) speed_window->show();
    else speed_window->hide();
}
