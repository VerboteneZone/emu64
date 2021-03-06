//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: main_window.cpp                       //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 11.12.2018                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#include "./main_window.h"
#include "./ui_main_window.h"

MainWindow::MainWindow(QWidget *parent,customSplashScreen* splash,QTextStream *log) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    c64(NULL)
{
    this->splash = splash;
    this->log = log;

    isFirstPaintEvent = false;

    this->setGeometry(0,0,0,0);
    this->show();
    this->hide();

    /// Haputfenster UI setzen ///
    ui->setupUi(this);

#ifdef _WIN32
    setWindowTitle("Emu64 Version " + QString(VERSION_STRING) + " --- [Windows " + QString(ARCHITECTURE_STRING) + "]");
#endif

#ifdef __linux__
    setWindowTitle("Emu64 Version " + QString(VERSION_STRING) + " --- [Linux " + QString(ARCHITECTURE_STRING) + "]");
#endif
}

MainWindow::~MainWindow()
{
    //////////// Save to INI ////////////
    if(ini != NULL)
    {
        ini->beginGroup("MainWindow");
        ini->setValue("Geometry",saveGeometry());
        ini->setValue("State",saveState());
        ini->setValue("ScreenshotCounter",ScreenshotNumber);
        ini->setValue("LastAutoloadDir",lastAutoloadPath);
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

        int x,y,w,h;
        c64->GetWindowPos(&x, &y);
        c64->GetWindowSize(&w, &h);

        ini->beginGroup("C64Screen");
        ini->setValue("PosX",x);
        ini->setValue("PosY",y);
        ini->setValue("SizeW",w);
        ini->setValue("SizeH",h);
        ini->endGroup();

        C64_KEYS* c64_key_table = c64->GetC64KeyTable();
        int c64_key_table_size = c64->GetC64KeyTableSize();

        ini->beginGroup("C64KeyMapping");

        for(int i=0; i<c64_key_table_size; i++)
        {
            QString name = "C64_KEY_" + QVariant(i).toString();
            ini->setValue(name,(int)c64_key_table[i].SDLKeyCode);
        }

        ini->endGroup();
    }
    /////////////////////////////////////

    /// WindowKlassen schließen ///

    delete setup_window;
    delete c64;

    delete videopal;
    delete info_window;
    delete tv_setup_window;
    delete floppy_window;
    delete tape_window;
    delete c64_keyboard_window;
    delete crt_window;
    delete debugger_window;
    delete speed_window;
    delete show_c64keymap_window;
    delete video_capture_window;
    delete sid_dump_window;

    delete ui;
    delete ini;
    LogText(trUtf8(">> Es wurden alle Klassen wieder entfernt\n").toLatin1().data());

    LogText(trUtf8("\n>> Emu64 wurde sauber beendet...").toLatin1().data());
    delete log;

    // Temporäre Dateien löschen
    QFile tmpfile(tmpPath + "/tmp.prg");

    if(tmpfile.exists())
    {
        tmpfile.remove();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    if(!isFirstPaintEvent)
    {
        isFirstPaintEvent = true;
        if(c64 != NULL) c64->SetFocusToC64Window();
    }
}

void MainWindow::OnInit()
{
    // Alle Pfade setzen //

    configPath = QDir::homePath() + "/.config/emu64";

#ifdef _WIN32
    if(custom_dataPath == "")
        dataPath = QApplication::applicationDirPath();
    else
        dataPath = custom_dataPath;
#endif
#ifdef __linux__
    if(custom_dataPath == "")
    {
        dataPath = DATA_PATH;
        dataPath += "/share/emu64";
    }
    else
        dataPath = custom_dataPath;
#endif
    LogText((QString(">> Data Path = ") + dataPath + QString("\n")).toUtf8().data());

    tmpPath = QDir::tempPath();
    LogText((QString(">> TEMP Path = ") + tmpPath + QString("\n")).toUtf8().data());

    if(splash != NULL)
    {
        splash->setMessageRect(QRect(0,303,400, 20), Qt::AlignCenter);

        QFont splashFont;
        splashFont.setFamily("Arial");
        splashFont.setBold(true);
        splashFont.setPixelSize(10);
        splashFont.setStretch(125);

        splash->setFont(splashFont);
    }

    /// Translator installieren ///
    langPath = dataPath+"/languages";
    qApp->installTranslator(&qtTranslator);
    qApp->installTranslator(&appTranslator);
    QString SystemLocale = QLocale::system().name();       // "de_DE"
    SystemLocale.truncate(SystemLocale.lastIndexOf('_'));  // "de"

    /// INI Dateiverwaltung erstellen ///
    SplashMessage(trUtf8("INI Dateiverwaltung wird initialisiert."),Qt::darkBlue);

    ini = new QSettings(configPath+"/emu64.ini",QSettings::IniFormat,this);

    LogText(QString(">> INI System wurde erzeugt: " + configPath + "/emu64.ini\n").toLatin1().data());
    LogText(QString(">> INI System was created: " + configPath + "/emu64.ini\n").toLatin1().data());

    SplashMessage(trUtf8("Sprachmenü wir erstellt."),Qt::darkBlue);
    ini->beginGroup("MainWindow");
    CreateLanguageMenu(ini->value("lang",SystemLocale).toString());

    lastAutoloadPath = ini->value("LastAutoloadDir",QDir::homePath()).toString();

    ini->endGroup();
    LogText(QString(trUtf8(">> Sprachmenü wurde erstellt.") + SystemLocale + "\n").toLatin1().data());


    SplashMessage(trUtf8("Translator wurde installiert."),Qt::darkBlue);
    LogText(QString(trUtf8(">> Translator wurde intsalliert: Systemsprache = ") + SystemLocale + "\n").toLatin1().data());
    setWindowIcon(QIcon(":/grafik/emu64.ico"));

    /// RomSetPath erstellen ///
    romsetPath = configPath + "/romsets/";

    QDir* dir = new QDir(romsetPath);

    SplashMessage(trUtf8("RomSet-Verzeichnis wird gesucht."),Qt::darkBlue);
    if(dir->exists())
    {
        LogText(trUtf8(">> Ein RomSet Verzeichnis ist vorhanden\n").toLatin1().data());
        RomSetEnable = true;
    }
    else
    {
        SplashMessage(trUtf8("RomSet-Verzeichnis wird erstellt."),Qt::darkBlue);
        LogText(trUtf8("<< Ein RomSet-Verzeichnis ist nicht vorhanden\n").toLatin1().data());
        if(dir->mkdir(romsetPath))
        {
            LogText(trUtf8(">> Ein neues RomSet-Verzeichnis wurde erstellt\n").toLatin1().data());
            RomSetEnable = true;
        }
        else
        {
            LogText(trUtf8("<< Ein neues RomSet-Verzeichnis konnte nicht erstellt werden\n").toLatin1().data());
            RomSetEnable = false;
        }
    }

    delete dir;

    if(RomSetEnable)
    {
        LogText(">> romsetPath = ");
        LogText(romsetPath.toLatin1().data());
        LogText("\n");
    }

    /// ScreenshotPath erstellen ///
    screenshotPath = configPath + "/screenshots/";

    dir = new QDir(screenshotPath);

    SplashMessage(trUtf8("Screenshotverzeichnis wird gesucht."),Qt::darkBlue);
    if(dir->exists())
    {
        LogText(trUtf8(">> Ein Screenshot Verzeichnis ist vorhanden\n").toLatin1().data());
        ScreenshotsEnable = true;
    }
    else
    {
        SplashMessage(trUtf8("Screenshotverzeichnis wird erstellt."),Qt::darkBlue);
        LogText(trUtf8("<< Ein Screenshot Verzeichnis ist nicht vorhanden\n").toLatin1().data());
        if(dir->mkdir(screenshotPath))
        {
            LogText(trUtf8(">> Ein neues Screenshot Verzeichnis wurde erstellt\n").toLatin1().data());
            ScreenshotsEnable = true;
        }
        else
        {
            LogText(trUtf8("<< Ein neues Screenshot Verzeichnis konnte nicht erstellt werden\n<< Keine Screenshots möglich !!").toLatin1().data());
            ScreenshotsEnable = false;
        }
    }

    delete dir;

    if(ScreenshotsEnable)
    {
        LogText(">> screenshotPath = ");
        LogText(screenshotPath.toLatin1().data());
        LogText("\n");
    }

    /// Klassen installieren ///
    SplashMessage(trUtf8("VideoPal Klasse wird initialisiert."),Qt::darkBlue);
    videopal = new VideoPalClass();
    LogText(trUtf8(">> VideoPal Klasse wurde installiert\n").toLatin1().data());

    /// C64 Klasse Installieren ... Das HERZ ///
    SplashMessage(trUtf8("C64 Klasse wird initialisiert."),Qt::darkBlue);
    int ret_error;
    c64 = new C64Class(&ret_error,videopal,bind(&MainWindow::LogText,this,_1),QString(dataPath).toLatin1().data());
    if(ret_error != 0)
    {
        ErrorMsg(trUtf8("Emu64 Fehler ..."),trUtf8("Fehler beim Installieren der C64 Klasse"))
        this->close();
    }

    SetC64ScreenTitle();

    /// Window Klassen erstellen ///
    /// Unter MAC sollte ohne übergabe des this Zeigers die Klasseb erstellt werden

    SplashMessage(trUtf8("InfoWindow wird erstellt."),Qt::darkBlue);
    info_window = new InfoWindow(this);
    LogText(trUtf8(">> InfoWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("TVSetupWindow wird erstellt."),Qt::darkBlue);
    tv_setup_window = new TVSetupWindow(this,c64,videopal,ini);
    LogText(trUtf8(">> TVSetupWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("FloppyWindow wird erstellt."),Qt::darkBlue);
    floppy_window = new FloppyWindow(this,ini,c64,tmpPath);
    LogText(trUtf8(">> FloppyWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("TapeWindow wird erstellt."),Qt::darkBlue);
    tape_window = new TapeWindow(this,ini,c64);
    LogText(trUtf8(">> TapeWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("C64KeyboardWindow wird erstellt."),Qt::darkBlue);
    c64_keyboard_window = new C64KeyboardWindow(this,ini,c64);
    LogText(trUtf8(">> C64KeyboardWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("CRTWindow wird erstellt."),Qt::darkBlue);
    crt_window = new CrtWindow(this,ini,c64);
    LogText(trUtf8(">> CrtWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("DebuggerWindow wird erstellt."),Qt::darkBlue);
    debugger_window = new DebuggerWindow(this,ini);
    LogText(trUtf8(">> DebuggerWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("SetupWindow wird erstellt."),Qt::darkBlue);
    setup_window = new SetupWindow(this,SLOT(OnChangeGrafikModi(bool,bool,bool,bool,bool)),videopal,ini,&romsetPath,&dataPath);
    connect(setup_window,SIGNAL(finished(int)),this,SLOT(OnSetupFished(int)));
    LogText(trUtf8(">> SetupWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("C64SpeedWindow wird erstellt."),Qt::darkBlue);
    speed_window = new C64SpeedWindow(this,ini);
    LogText(trUtf8(">> C64SpeedWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("ShowC64KeyMapWindow wird erstellt."),Qt::darkBlue);
    show_c64keymap_window = new ShowC64KeyMappingWindow(this,c64);
    LogText(trUtf8(">> ShowC64KeyMapWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("VideoCaptureWindow wird erstellt."),Qt::darkBlue);
    video_capture_window = new VideoCaptureWindow(this, c64);
    LogText(trUtf8(">> VideoCaptureWindow wurde erzeugt\n").toLatin1().data());

    SplashMessage(trUtf8("SidDumpWindow wird erstellt."),Qt::darkBlue);
    sid_dump_window = new SidDumpWindow(this, c64);
    LogText(trUtf8(">> SidDumpWindow wurde erzeugt\n").toLatin1().data());

    ini->beginGroup("MainWindow");
    SplashMessage(trUtf8("Screenshotnummer wird geladen."),Qt::darkBlue);
    ScreenshotNumber = (int)ini->value("ScreenshotCounter",0).toInt();
    ini->endGroup();
    LogText(trUtf8(">> Aktuelle Screenshotnummer wurde geladen\n").toLatin1().data());

    SplashMessage(trUtf8("SetupWindow wird mit INI abgeglichen."),Qt::darkBlue);
    setup_window->ReSetup();
    LogText(trUtf8(">> SetupWindow ReSetup wurde durchgeführt\n").toLatin1().data());

    //SplashMessage(trUtf8("SDL Window Titelleiste wird benannt."),Qt::darkBlue);
    //SDL_WM_SetCaption((const char*)trUtf8("C64 Bildschirm").toLatin1().data(),0);

    /// Debugger Window mit C64 verbinden ///
    SplashMessage(trUtf8("Debugger Window wird mit C64 Klasse verbunden."),Qt::darkBlue);
    debugger_window->SetC64Pointer(c64);
    LogText(trUtf8(">> DebuggerWindow wurde mit C64 Klasse verbunden\n").toLatin1().data());

    /// C64 Speed Window mit C64 verbinden ///
    SplashMessage(trUtf8("C64 Speed Window wird mit C64 Klasse verbunden."),Qt::darkBlue);
    speed_window->SetC64Pointer(c64);
    LogText(trUtf8(">> C64SpeedWindow wurde mit C64 Klasse verbunden\n").toLatin1().data());

    /// CRT LED mit CRT_Window verbinden ///
    SplashMessage(trUtf8("CRT LED mit CRT Window verbunden."),Qt::darkBlue);
    c64->crt->ChangeLED = bind(&CrtWindow::ChangeLED,crt_window,_1,_2);
    LogText(trUtf8(">> CRT LED wurde mit CrtWindow verbunden\n").toLatin1().data());

    /// C64 Systemroms laden ///
    SplashMessage(trUtf8("C64 Systemroms werden geladen."),Qt::darkBlue);
    if(!c64->LoadC64Roms((char*)QString(dataPath+"/roms/kernal.rom").toLatin1().data(),(char*)QString(dataPath+"/roms/basic.rom").toLatin1().data(),(char*)QString(dataPath+"/roms/char.rom").toLatin1().data()))
    {
        LogText((char*)"<< ERROR: Fehler beim laden der C64 Roms\n\t");
        LogText(QString(dataPath+"/roms/kernal.rom").toLatin1().data());LogText("\n\t");
        LogText(QString(dataPath+"/roms/basic.rom").toLatin1().data());LogText("\n\t");
        LogText(QString(dataPath+"/roms/char.rom").toLatin1().data());LogText("\n");
    }
    else LogText(trUtf8(">> Alle C64 Roms geladen\n").toLatin1().data());

    /// C64 Keyboard Matrix mit dem Virtual Keyboard verbinden ///
    SplashMessage(trUtf8("C64 Key-Matrix wird mit Virtual Keyboard verbunden."),Qt::darkBlue);
    c64_keyboard_window->KeyMatrixToPA = c64->KeyboardMatrixToPAExt;
    c64_keyboard_window->KeyMatrixToPB = c64->KeyboardMatrixToPBExt;
    LogText(trUtf8(">> C64 Key-Matrix wurde mit Virtual Keyboard verbunden\n").toLatin1().data());


    /// Tabelle für Floppy's Ertsellen ///
    SplashMessage(trUtf8("Tabelle für Floppy's wird erstellt."),Qt::darkBlue);
    ui->FloppyTabel->setRowCount(FloppyAnzahl);
    ui->FloppyTabel->setColumnCount(1);
    LogText(trUtf8(">> Tabelle für alle Floppy Laufwerke wurde erzeugt\n").toLatin1().data());

    for(int i=0; i<FloppyAnzahl; i++)
    {
        SplashMessage(trUtf8("Floppy: ") + QVariant(i).toString() + "wird in Tabelle eingefügt",Qt::darkBlue);
        ui->FloppyTabel->setRowHeight(i,24);
        WidgetFloppyStatus *w = new WidgetFloppyStatus(this,i,c64->floppy[i]);
        w->SetGeraeteID(i+8);
        ui->FloppyTabel->setCellWidget(i,0,w);
        ui->FloppyTabel->setColumnWidth(0,w->width());
        connect(w,SIGNAL(ChangeEnableFloppy()),debugger_window,SLOT(onChangeFloppyStatus()));
        connect(w,SIGNAL(LoadImage(int)),floppy_window,SLOT(OnChangeFloppyNummer(int)));
        connect(w,SIGNAL(RemoveImage(int)),floppy_window,SLOT(OnRemoveImage(int)));
    }
    LogText(trUtf8(">> Alle Floppy Lauferke in Tabelle eingetragen\n").toLatin1().data());

    /// Close Evend von C64 Klasse hierher leiten
    c64->CloseEventC64Screen = bind(&MainWindow::CloseC64Screeen,this);
    LogText(trUtf8(">> SDL Window Close Event mit MainWindow verknüpft\n").toLatin1().data());

    /// LimitCyclesEvent von C64 Klasse hierher leiten
    IsLimitCyclesEvent = false;
    c64->LimitCyclesEvent = bind(&MainWindow::LimitCyclesEvent,this);
    LogText(trUtf8(">> C64Class LimitCycelsEvent mit MainWindow verknüpft\n").toLatin1().data());

    /// DebugCartEvent von C64 Klasse hierher leiten
    IsDebugCartEvent = false;
    c64->DebugCartEvent = bind(&MainWindow::DebugCartEvent,this,_1);
    LogText(trUtf8(">> C64Class DebugCartEvent mit MainWindow verknüpft\n").toLatin1().data());

    connect(floppy_window,SIGNAL(ChangeFloppyImage(int)),this,SLOT(OnChangeFloppyImage(int)));
    LogText(trUtf8(">> ChangeFloppyImage Event mit MainWindow verknüpft\n").toLatin1().data());

    ////////// Load from INI ///////////
    SplashMessage(trUtf8("Einstellungen werden von INI geladen und gesetzt."),Qt::darkBlue);
    if(ini != NULL)
    {
        floppy_window->LoadIni();
        LogText(trUtf8(">> FloppyWindow LoadIni wurde ausgeführt\n").toLatin1().data());

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
        LogText(trUtf8(">> SetupWindow LoadIni wurde ausgeführt\n").toLatin1().data());

        /// CRT Ini erst jetzt laden ///
        crt_window->LoadIni();
        LogText(trUtf8(">> CrtWindow LoadIni wurde ausgeführt\n").toLatin1().data());

        /// TAPE Ini laden ///
        tape_window->LoadIni();
        LogText(trUtf8(">> TapeWindow LoadIni wurde ausgeführt\n").toLatin1().data());

        /// C64 Key Mapping aus INI laden ///

        C64_KEYS* c64_key_table = c64->GetC64KeyTable();
        int c64_key_table_size = c64->GetC64KeyTableSize();

        ini->beginGroup("C64KeyMapping");

        for(int i=0; i<c64_key_table_size; i++)
        {
            QString name = "C64_KEY_" + QVariant(i).toString();
            unsigned long sdl_key_code = ini->value(name,0xFFFFFFFF).toUInt();

            if(sdl_key_code != 0xFFFFFFFF)
            {
                c64_key_table[i].SDLKeyCode = sdl_key_code;
            }
        }

        LogText(trUtf8(">> C64 Tastatur Mapping aus INI geladen\n").toLatin1().data());

        ini->endGroup();
    }

    this->update();

    /// Screenshot Reset mit SetupWindow verbinden ///
    connect(setup_window,SIGNAL(ResetSreenshotCounter()),this,SLOT(OnResetScreenshotCounter()));


    ini->beginGroup("C64Screen");

    int x = ini->value("PosX",-1).toInt();
    int y = ini->value("PosY",-1).toInt();

    if((x>0) && (y>0))
    {
        c64->SetWindowPos(x,y);
    }

    int w = ini->value("SizeW",-1).toInt();
    int h = ini->value("SizeH",-1).toInt();

    if((w>0) && (h>0))
    {
        c64->SetWindowSize(w,h);
    }

    ini->endGroup();

    LogText(">> Emu64 wurde initialisiert\n");


    /////////////////////////////////////
    SplashMessage(trUtf8("C64 EMULATION WIRD NUN GESTARET."),Qt::darkBlue);
    if(ret_error == 0) c64->StartEmulation();
    else
    {
        this->close();
        return;
    }
    c64->HardReset();

    if(splash != NULL) splash->close();

    this->show();
}

void MainWindow::OnMessage(QStringList msg)
{
    vector<char*> list;

    if(c64 != NULL)
    {
        for(int i=0; i<msg.length(); i++)
        {
            char *p = new char[msg.at(i).size()+1];
            strcpy(p,msg.at(i).toAscii().data());
            list.push_back(p);
        }
        ExecuteCommandLine(list);
    }
}

void MainWindow::LogText(const char *log_text)
{
    if(log != NULL)
    {
        *log << log_text;
        log->flush();
    }
}

void MainWindow::CloseC64Screeen()
{
    on_actionBeenden_triggered();
}

void MainWindow::LimitCyclesEvent()
{
    IsLimitCyclesEvent = true;
    on_actionBeenden_triggered();
}

void MainWindow::DebugCartEvent(unsigned char value)
{
    DebugCartValue = value;
    IsDebugCartEvent = true;
    on_actionBeenden_triggered();
}

void MainWindow::SetCustomDataPath(QString path)
{
    custom_dataPath = path;
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

    if (event->type() == QEvent::Show)
    {
        qDebug("Show Event");
        c64->SetFocusToC64Window();
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
      action->setStatusTip(trUtf8("Wechseln zur Sprache: ") + lang);

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
    tape_window->RetranslateUi();
    crt_window->RetranslateUi();
    debugger_window->RetranslateUi();
    setup_window->RetranslateUi();
    speed_window->RetranslateUi();
    video_capture_window->RetranslateUi();

    SetC64ScreenTitle();
}

void MainWindow::SetC64ScreenTitle()
{
    c64->SetWindowTitle(trUtf8("C64 Bildschirm").toLatin1().data());
}

bool MainWindow::getSaveFileName(QWidget *parent, QString caption, QString filter, QString *fileName, QString *fileExt)
{
   if (fileName == NULL)      // "parent" is allowed to be NULL!
      return false;

   QFileDialog saveDialog(parent);
   saveDialog.setWindowTitle(caption);
   saveDialog.setAcceptMode(QFileDialog::AcceptSave);
   saveDialog.setConfirmOverwrite(false);
   saveDialog.setFilter(filter);
   saveDialog.selectFile(*fileName);
   saveDialog.setOptions(QFileDialog::DontUseNativeDialog);

   *fileName = "";

   if (!saveDialog.exec())
      return false;      // User pressed "Cancel"

   QStringList fileList = saveDialog.selectedFiles();
   if (fileList.count() != 1)
      return false;      // Should not happen, just to be sure

   QString tmpFileName = fileList.at(0);
   QString extension;

   QFileInfo fileInfo(tmpFileName);
   if (fileInfo.suffix().isEmpty()) {
      // Add the suffix selected by the user

      extension = saveDialog.selectedFilter();
      extension = extension.right(extension.size() - extension.indexOf("*.") - 2);
      extension = extension.left(extension.indexOf(")"));
      extension = extension.simplified();

      // If the filter specifies more than one extension, choose the first one
      if (extension.indexOf(" ") != -1)
         extension = extension.left(extension.indexOf(" "));

      tmpFileName = tmpFileName + QString(".") + extension;
      fileInfo.setFile(tmpFileName);
   }

   // Does the file already exist?
   if (QFile::exists(tmpFileName)) {

       extension = saveDialog.selectedFilter();
       extension = extension.right(extension.size() - extension.indexOf("*.") - 2);
       extension = extension.left(extension.indexOf(")"));
       extension = extension.simplified();

      int result = QMessageBox::question(parent, QObject::trUtf8("Überschreiben?"),
         QObject::trUtf8("Soll die Datei \"%1\" überschrieben werden?").arg(fileInfo.fileName()),
         QMessageBox::Yes,
         QMessageBox::No | QMessageBox::Default,
         QMessageBox::Cancel | QMessageBox::Escape);
      if (result == QMessageBox::Cancel)
         return false;
      else if (result == QMessageBox::No) {
         // Next chance for the user to select a filename
         if (!getSaveFileName(parent, caption, filter, &tmpFileName, &extension))
            // User decided to cancel, exit function here
            return false;
      // User clicked "Yes", so process the execution
      fileInfo.setFile(tmpFileName);
      }
   }

   *fileName = tmpFileName;
   *fileExt = extension;
   return true;
}

void MainWindow::ExecuteCommandLine(vector<char *> &arg)
{
    CommandLineClass *cmd_line = new CommandLineClass(arg.size(), arg.data(), "emu64",command_list, command_list_count);

    bool error;
    int lwnr,adr,val;
    char *filename;
    QFileInfo *fi;

    bool loop_break = false;

    for(int i=0; i<cmd_line->GetCommandCount() && !loop_break; i++)
    {
        int akt_command = cmd_line->GetCommand(i);
        switch(akt_command)
        {
        case CMD_ARG:
            break;
        case CMD_AUTOSTART:
            c64->LoadAutoRun(0,cmd_line->GetArg(i+1));
            break;
        case CMD_HARDRESET:
            c64->HardReset();
            break;
        case CMD_SOFTRESET:
            c64->SoftReset();
            break;
        case CMD_MOUNT_DISK:
            lwnr = cmd_line->GetArgInt(i+1, &error);
            filename = cmd_line->GetArg(i+2);

            if(!error)
            {
                if(lwnr >= 8 && lwnr <= 11)
                {
                    fi = new QFileInfo(filename);
                    if(fi->exists())
                    {
                        cout << "Laufwerksnummer: " << lwnr << endl;
                        cout << "Disk Image: " << fi->fileName().toAscii().data() << endl;

                        if(floppy_window->SetDiskImage(lwnr-8, fi->absoluteFilePath()))
                        {
                            WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(lwnr-8,0);
                            w->SetEnableFloppy(true);
                        }
                    }
                    else
                        cout << "Die angebene Datei existiert nicht." << endl;
                }
                else
                    cmd_line->OutErrorMsg("Laufwerksnummer muss zwischen 8 und 11 liegen!","--help");
            }
            break;
        case CMD_MOUNT_CRT:
            filename = cmd_line->GetArg(i+1);
            fi = new QFileInfo(filename);
            if(fi->exists())
            {
                if(crt_window->SetCrtImage(fi->absoluteFilePath()))
                {
                    crt_window->ConnectCrt();
                }
            }
            else
                cout << "Die angebene Datei existiert nicht." << endl;
            break;
        case CMD_UMOUNT_CRT:
            crt_window->DisconnectCrt();
            break;
        case CMD_WARP_MODE:
            c64->EnableWarpMode(true);
            break;
        case CMD_POKE_64:
            adr = cmd_line->GetArgInt(i+1, &error);
            if(error) break;

            val = cmd_line->GetArgInt(i+2, &error);
            if(error) break;

            if(!(adr >= 0x0000 && adr <= 0xffff))
            {
                cmd_line->OutErrorMsg("Die Adresse muss zwischen (0)0x0000 und (65535)0xffff liegen!","--help");
                break;
            }

            if(!(val >= 0x00 && val <= 0xff))
            {
                cmd_line->OutErrorMsg("Der zu schreibende Wert muss zwischen 0(0x00) und 255(0xff) liegen!","--help");
                break;
            }

            c64->WriteC64Byte((unsigned short)adr,(unsigned char)val);
            break;
        case CMD_LIMITCYCLES:
            val = cmd_line->GetArgInt(i+1, &error);
            if(error) break;

            if(!(val > 0))
            {
                cmd_line->OutErrorMsg("Die Anzahl der Zyklen müssen größer als 0 sein.","--help");
                break;
            }
            c64->SetLimitCycles(val);
            break;

        case CMD_EXIT_SCREENSHOT:
            c64->SetExitScreenshot(cmd_line->GetArg(i+1));
            break;

        case CMD_DEBUG_CART:
            c64->SetEnableDebugCart(true);
            break;

        case CMD_VIDEOCAPTURE:
            cout << "Video Record: " << cmd_line->GetArg(i+1) << endl;
            c64->StartVideoRecord(cmd_line->GetArg(i+1));
            break;
        }

        // ----------------------------------------------------

        // i korregieren (abhängig von der Anzahl der Argumente)
        if(akt_command != CMD_ARG)
            i += cmd_line->GetCommandArgCount(akt_command);
    }
}

void MainWindow::SplashMessage(const QString &message, const QColor &color)
{
    if(splash != NULL) splash->showStatusMessage(message,color);
}

void MainWindow::on_menu_main_info_triggered()
{
    SDL_version compiled;
    SDL_VERSION(&compiled);

    QString sdl_version = QVariant(compiled.major).toString() + "." + QVariant(compiled.minor).toString() + "." + QVariant(compiled.patch).toString();
    QString emu64_version = QString("Emu64 V") + QString(VERSION_STRING);

    info_window->SetEmu64VersionText(emu64_version);
    info_window->SetMoreInfoText("Qt Version: " + QString(qVersion()) + " / SDL Version: " + sdl_version + " / FFMpeg Version: " + QString(c64->GetAVVersion()));

    info_window->show();
}

void MainWindow::on_actionBeenden_triggered()
{
    if(!debugger_window->isHidden()) debugger_window->hide();
    showNormal();
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
    QDir auto_path = lastAutoloadPath;

    // Wenn das lestzte Autolad Verzeichnis nicht mehr existiert, wird das Homeverzeichnis gesetzt
    if(!auto_path.exists())
    {
        lastAutoloadPath = QDir::homePath();
    }

    QString filename = QFileDialog::getOpenFileName(this,trUtf8("C64 Dateien öffnen "),lastAutoloadPath,trUtf8("C64 Programm Dateien ") + "(*.prg *.c64 *.p00 *.t64 *.d64 *.g64 *.frz);;" + trUtf8("Alle Dateien ") + "(*.*)",0,QFileDialog::DontUseNativeDialog);
    if(filename != "")
    {
        // akutelles Autostart Verzeichnis abspeichern
        QFileInfo file_info = filename;
        lastAutoloadPath = file_info.absolutePath();

        if(c64->LoadAutoRun(0,filename.toLatin1().data()) == 0)
        {
            // Prüfen welche
            if(file_info.suffix().toUpper() == "D64")
            {
                WidgetFloppyStatus *w = (WidgetFloppyStatus*)ui->FloppyTabel->cellWidget(0,0);
                w->SetEnableFloppy(true);
                floppy_window->SetDiskImage(0,filename);
            }
        }
        c64->SetFocusToC64Window();
    }
}

void MainWindow::on_actionC64_Programme_direkt_laden_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,trUtf8("C64 Dateien öffnen "),"",trUtf8("C64 Programm Dateien ") + "(*.prg *.c64 *.p00 *.t64 *.frz);;" + trUtf8("Alle Dateien ") + "(*.*)",0,QFileDialog::DontUseNativeDialog);
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

void MainWindow::OnChangeGrafikModi(bool fullscreen, bool palmode, bool doublemode, bool bit32mode, bool filter)
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

void MainWindow::OnSetupFished(int result)
{
    if(result == 0)
        c64->StopRecJoystickMapping();
}

void MainWindow::on_actionScreenshot_triggered()
{    
    if(ScreenshotsEnable)
    {
        int format = setup_window->GetScreenshotFormat();
        switch (format)
        {
        case SCREENSHOT_FORMAT_BMP:
            c64->SaveScreenshot(QString(screenshotPath + "emu64_" + QVariant(ScreenshotNumber).toString() + ".bmp").toLatin1().data(),format);
            break;
        case SCREENSHOT_FORMAT_PNG:
            c64->SaveScreenshot(QString(screenshotPath + "emu64_" + QVariant(ScreenshotNumber).toString() + ".png").toLatin1().data(),format);
            break;
        default:
            break;
        }

        ScreenshotNumber++;
    }
    else QMessageBox::critical(this,trUtf8("Emu64 Fehler ..."),trUtf8("Es sind keine Screenshots möglich da Emu64 kein Screenshot Verzeichnis anlegen konnte.\nÜberprüfen Sie bitte die Rechte des Emu64 Verzeichnisses !"));
}

void MainWindow::on_actionC64_Geschwindigkeit_triggered()
{
    if(speed_window->isHidden()) speed_window->show();
    else speed_window->hide();
}

/// REU Tools ///

void MainWindow::on_actionREU_einstecken_triggered()
{
    c64->InsertREU();
    c64->SetFocusToC64Window();
}

void MainWindow::on_actionREU_entfernen_triggered()
{
    c64->RemoveREU();
    c64->SetFocusToC64Window();
}

void MainWindow::on_actionREU_laden_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,trUtf8("REU Inhalt laden"),QDir::homePath(),trUtf8("REU Image Dateien") + "(*.reu);;" + trUtf8("Alle Dateien") + "(*.*)",0,QFileDialog::DontUseNativeDialog);
    if(filename != "")
    {
        if(c64->LoadREUImage(filename.toLatin1().data()) != 0)
            QMessageBox::critical(this,trUtf8("Emu64 Fehler ..."),trUtf8("Beim laden des REU Images trat ein Fehler auf!"));
    }
}

void MainWindow::on_actionREU_speichern_triggered()
{
    QString filename;
    QString fileext;

    if(!getSaveFileName(this,trUtf8("REU Inhalt speichern"),trUtf8("REU Image Dateien") + "(*.reu);;" + trUtf8("Alle Dateien") + "(*.*)",&filename,&fileext))
        return;

    if(c64->SaveREUImage(filename.toLatin1().data()) != 0)
        QMessageBox::critical(this,trUtf8("Emu64 Fehler ..."),trUtf8("Beim laden des REU Images trat ein Fehler auf!"));
}

void MainWindow::on_actionREU_loeschen_triggered()
{
    if(QMessageBox::Yes == QMessageBox::question(this,trUtf8("REU Speicher löschen ..."),trUtf8("Möchten Sie den Inhalt des REU Speichers wirklich löschen?"),QMessageBox::Yes | QMessageBox::No))
    {
        c64->ClearREURam();
    }
}

/// GEO-RAM Tools ///

void MainWindow::on_actionGEO_einstecken_triggered()
{
    c64->InsertGEORAM();
    c64->SetFocusToC64Window();
}

void MainWindow::on_actionGEO_entfernen_triggered()
{
    c64->RemoveGEORAM();
    c64->SetFocusToC64Window();
}

void MainWindow::on_actionGEO_laden_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,trUtf8("GEORAM Inhalt laden"),QDir::homePath(),trUtf8("GEORAM Image Dateien") + "(*.img);;" + trUtf8("Alle Dateien") + "(*.*)",0,QFileDialog::DontUseNativeDialog);
    if(filename != "")
    {
        if(c64->LoadGEORAMImage(filename.toLatin1().data()) != 0)
            QMessageBox::critical(this,trUtf8("Emu64 Fehler ..."),trUtf8("Beim laden des GEORAM Images trat ein Fehler auf!"));
    }
}

void MainWindow::on_actionGEO_speichern_triggered()
{
    QString filename;
    QString fileext;

    if(!getSaveFileName(this,trUtf8("GEORAM Inhalt speichern"),trUtf8("GEORAM Image Dateien") + "(*.img);;" + trUtf8("Alle Dateien") + "(*.*)",&filename,&fileext))
        return;

    if(c64->SaveGEORAMImage(filename.toLatin1().data()) != 0)
        QMessageBox::critical(this,trUtf8("Emu64 Fehler ..."),trUtf8("Beim laden des GEORAM Images trat ein Fehler auf!"));
}

void MainWindow::on_actionGEO_loeschen_triggered()
{
    if(QMessageBox::Yes == QMessageBox::question(this,trUtf8("GEORAM Speicher löschen ..."),trUtf8("Möchten Sie den Inhalt des GEORAM Speichers wirklich löschen?"),QMessageBox::Yes | QMessageBox::No))
    {
        c64->ClearGEORAMRam();
    }
}

void MainWindow::on_actionVollbild_triggered()
{
    c64->SetFullscreen();
}

void MainWindow::on_actionBandlaufwerk_1530_triggered()
{
    if(tape_window->isHidden()) tape_window->show();
    else tape_window->hide();
}

void MainWindow::on_actionC64_Tastenbelegung_Show_triggered()
{
    show_c64keymap_window->UpdateText();
    show_c64keymap_window->show();
}

void MainWindow::on_actionVideo_Capture_triggered()
{
    video_capture_window->show();
}

void MainWindow::on_actionSID_Dump_triggered()
{
    sid_dump_window->show();
}
