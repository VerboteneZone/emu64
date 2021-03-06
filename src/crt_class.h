//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: crt_class.h                           //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 11.02.2018                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef CRT_CLASS_H
#define CRT_CLASS_H

#include "structs.h"
#include "am29f040_class.h"
#include "mk7pla.h"

#include "tr1/functional"
using namespace std::tr1;
using namespace std::tr1::placeholders;

class CRTClass
{
public:
    CRTClass();
    ~CRTClass();

    // Funktionen
    int* GetFlash040Dirty(int nr);
    unsigned char* GetFlash040Byte(int nr);
    int LoadCRTImage(char* filename);
    void RemoveCRTImage(void);
    int CreateNewEasyFlashImage(char* filename,char* crt_name);
    int WriteEasyFlashImage(char* filename);
    int GetCRTInfo(char* filename,CRT_INFO_STRUCT* crtinfo);
    void SetEasyFlashJumper(bool enabled);
    void Reset(void);
    void Freeze(void);
    unsigned long conv_dword(unsigned long wert);

    void WriteIO1(unsigned short adresse,unsigned char wert);
    unsigned char ReadIO1(unsigned short adresse);
    void WriteIO2(unsigned short adresse,unsigned char wert);
    unsigned char ReadIO2(unsigned short adresse);
    void WriteRom1(unsigned short adresse,unsigned char wert);
    void WriteRom2(unsigned short adresse,unsigned char wert);
    void WriteRom3(unsigned short adresse,unsigned char wert);
    unsigned char ReadRom1(unsigned short adresse);
    unsigned char ReadRom2(unsigned short adresse);
    unsigned char ReadRom3(unsigned short adresse);

    function<void(void)> ChangeMemMapProc;
    function<void(int)> CpuTriggerInterrupt;
    function<void(int)> CpuClearInterrupt;
    function<void(int,bool)> ChangeLED;

    // Variablen
    bool            *EXROM;
    bool            *GAME;

    unsigned char   *ROM_LO;		// 0x8000
    unsigned char   *ROM_HI;		// 0xA000 oder 0xE000

    unsigned char   CRT_ROM_BANK1[256*0x2000];		// Alle ChipRoms für 0x8000	(max 64 x 0x2000)
    unsigned char   CRT_ROM_BANK2[256*0x2000];		// Alle ChipRoms für 0xA000 oder 0xE000 (max 64 x 0x2000)

    unsigned char   CRT_ROM_BANK1_TMP[256*0x2000];	// Alle ChipRoms für 0x8000	(max 64 x 0x2000)
    unsigned char   CRT_ROM_BANK2_TMP[256*0x2000];	// Alle ChipRoms für 0xA000 oder 0xE000 (max 64 x 0x2000)

    unsigned char   *RAM_C64;	// Kompletter C64 RAM

    bool            LED_00;             // LED FCIII
    bool            LED_00_OLD;
    bool            LED_01;             // LED EF
    bool            LED_01_OLD;

    private:
    // Variablen

    bool            CRT_EXROM;
    bool            CRT_GAME;

    bool            CRTInsert;
    unsigned short  CRTTyp;

    //// EASY FLASH (32)
    bool            EasyFlashJumper;
    unsigned char   EasyFlashRam[256];		// Speziller Speicher für das EasyFlash Cartridge
    unsigned char   EasyFlashBankReg;		// Bank Register
    AM29F040Class   *am29f040Hi;
    AM29F040Class   *am29f040Lo;

    int             RomLBank;

    //// Action Replay 4/5/6 ////
    unsigned char   ARRegister;             // Inhalt des zuletzt geschriebenen $DExx Wert
    bool            ARFreez;
    bool            ActionReplayAktiv;
    bool            EnableActionReplayRam;
    unsigned char   ActionReplayRam[0x2000];    // 8KB

private:
    void ResetAllLEDS(void);
    void SetMemLogicAR(unsigned short adresse);

    unsigned char pla_adresse;
};

#endif // CRT_CLASS_H
