#pragma once

#include <phosg/Encoding.hh>
#include <stdint.h>

namespace ResourceDASM {

struct Queue {
  be_uint16_t flags;
  be_uint32_t head;
  be_uint32_t tail;
} __attribute__((packed));

struct SystemParameters {
  uint8_t SPValid;
  uint8_t SPATalkA;
  uint8_t SPATalkB;
  uint8_t SPConfig;
  be_uint16_t SPPortA;
  be_uint16_t SPPortB;
  be_uint32_t SPAlarm;
  be_uint16_t SPFont;
  uint8_t SPKbd;
  uint8_t SPPrint;
  uint8_t SPVolCtl;
  uint8_t SPClikCaret;
  uint8_t SPMisc1;
  uint8_t SPMisc2_PCDeskPat;
} __attribute__((packed));

struct LowMemoryGlobals {
  // 0000
  be_uint32_t __m68k_reset_stack__;
  be_uint32_t __m68k_vec_reset__;
  be_uint32_t BusErrVct;
  be_uint32_t __m68k_vec_address_error__;
  // 0010
  be_uint32_t __m68k_vec_illegal__;
  be_uint32_t __m68k_vec_div_zero__;
  be_uint32_t __m68k_vec_chk__;
  be_uint32_t __m68k_vec_trapv__;
  // 0020
  be_uint32_t __m68k_vec_priv_violation__;
  be_uint32_t __m68k_vec_trace__;
  be_uint32_t __m68k_vec_a_trap__;
  be_uint32_t __m68k_vec_f_trap__;
  // 0030
  be_uint32_t unused1[3];
  be_uint32_t __m68k_vec_uninitialized__;
  be_uint32_t unused2[8];
  // 0060
  be_uint32_t __m68k_vec_spurious__;
  be_uint32_t __m68k_vec_via__;
  be_uint32_t __m68k_vec_scc__;
  be_uint32_t __m68k_vec_via_scc__;
  // 0070
  be_uint32_t __m68k_vec_switch__;
  be_uint32_t __m68k_vec_switch_via__;
  be_uint32_t __m68k_vec_switch_scc__;
  be_uint32_t __m68k_vec_switch_via_scc__;
  // 0080
  uint8_t unused3[0x80];
  // 0100
  be_int16_t MonkeyLives;
  be_uint16_t ScrVRes;
  be_uint16_t ScrHRes;
  be_uint16_t ScreenRow;
  be_uint32_t MemTop;
  be_uint32_t BufPtr;
  // 0110
  be_uint32_t StkLowPt;
  be_uint32_t HeapEnd;
  be_uint32_t TheZone;
  be_uint32_t UTableBase;
  // 0120
  be_uint32_t MacJump;
  be_uint32_t DskRtnAdr;
  be_uint32_t PollRtnAdr;
  uint8_t DskVerify;
  uint8_t LoadTrap;
  uint8_t MmInOK;
  uint8_t CPUFlag;
  // 0130
  be_uint32_t ApplLimit;
  be_uint32_t SonyVars;
  be_uint16_t PWMValue;
  be_uint32_t PollStack;
  // 013E
  be_uint32_t PollProc;
  be_int16_t DskErr;
  be_uint16_t SysEvtMask;
  be_uint32_t SysEvtBuf;
  Queue EventQueue;
  // 0154
  be_uint16_t EvtBufCnt;
  be_uint32_t RndSeed;
  be_uint16_t SysVersion;
  uint8_t SEvtEnb;
  uint8_t DSWndUpdate;
  uint8_t FontFlag;
  uint8_t IntFlag;
  // 0160
  Queue VBLQueue;
  be_uint32_t Ticks;
  be_uint32_t MBTicks;
  // 0172
  uint8_t MBState;
  uint8_t Tocks;
  be_uint64_t KeyMap;
  be_uint32_t KeypadMap;
  // 0180
  be_uint32_t unused4;
  be_uint16_t KeyLast;
  be_uint32_t KeyTime;
  be_uint32_t KeyRepTime;
  be_uint16_t KeyThresh;
  // 0190
  be_uint16_t KeyRepThresh;
  // 0192
  be_uint32_t Lvl1DT[8]; // TODO: type is probably wrong here
  // 01B2
  be_uint32_t Lvl2DT[8]; // TODO: type is probably wrong here
  // 01D2
  be_uint16_t UnitNtryCnt;
  be_uint32_t VIA;
  be_uint32_t SCCRd;
  be_uint32_t SCCWr;
  // 01E0
  be_uint32_t IWM;
  SystemParameters GetParam;
  // 01F8
  SystemParameters SysParam;
  // 020C
  be_uint32_t Time;
  be_uint16_t BootDrive;
  be_uint16_t JShell;
  be_int16_t SFSaveDisk;
  // Note: the following two fields are somtimes referred to as a single 32-bit
  // field called KbdVars
  be_uint16_t HiKeyLast;
  be_uint16_t KbdLast;
  be_uint32_t JKybdTask;
  uint8_t KbdType;
  uint8_t AlarmState;
  // 0220
  be_int16_t MemErr;
  be_uint32_t JFigTrkSpd;
  be_uint32_t JDiskPrime;
  be_uint32_t JRdAddr;
  be_uint32_t JRdData;
  // 0232
  be_uint32_t JWrData;
  be_uint32_t JSeek;
  be_uint32_t JSetupPoll;
  be_uint32_t JRecal;
  // 0242
  be_uint32_t JControl;
  be_uint32_t JWakeUp;
  be_uint32_t JReSeek;
  be_uint32_t JMakeSpdTbl;
  // 0252
  be_uint32_t JAdrDisk;
  be_uint32_t JSetSpeed;
  be_uint32_t NiblTbl;
  be_uint16_t FlEvtMask;
  // 0260
  uint8_t SdVolume;
  uint8_t SdEnable_Finder;
  // Note: the following 7 fields are sometimes referred to as a 32-byte array
  // called SoundVars
  be_uint32_t SoundPtr;
  be_uint32_t SoundBase;
  be_uint32_t SoundVBL[4]; // TODO: type is probably wrong here
  be_uint32_t SoundDCE;
  uint8_t SoundActive;
  uint8_t SoundLevel;
  be_uint16_t CurPitch;
  // 0282
  be_uint32_t Switcher;
  be_uint32_t SwitcherTPtr;
  be_uint32_t RSDHndl;
  be_uint16_t ROM85;
  // 0290
  uint8_t PortAUse;
  uint8_t PortBUse;
  be_uint64_t ScreenVars;
  be_uint32_t JGNEFilter;
  // 029E
  be_uint32_t Key1Trans;
  be_uint32_t Key2Trans;
  be_uint32_t SysZone;
  be_uint32_t ApplZone;
  // 02AE
  be_uint32_t ROMBase;
  be_uint32_t RAMBase;
  be_uint32_t ExpandMem;
  be_uint32_t DSAlertTab;
  // 02BE
  be_uint32_t ExtStsDT[4];
  uint8_t SCCASts;
  uint8_t SCCBSts;
  // 02D0
  // Note: the following 3 fields are sometimes referred to as 4 32-bit values
  // called SerialVars
  be_uint32_t unused5[2];
  be_uint32_t ABusVars;
  be_uint32_t ABusDCE;
  // 02E0
  char FinderName[0x10]; // p-string
  // 02F0
  be_uint32_t DoubleTime;
  be_uint32_t CaretTime;
  uint8_t ScrDmpEnb;
  uint8_t ScrDmpType;
  be_uint16_t TagData;
  be_uint32_t BufTgFNum;
  // 0300
  be_uint16_t BufTgFFlg;
  be_uint16_t BufTgFBkNum;
  be_uint32_t BufTgDate;
  Queue DrvQHdr;
  // 0312
  be_uint32_t PWMBuf2;
  be_uint32_t HpChk_MacPgm;
  be_uint32_t MaskBC_MaskHandle_MaskPtr_Lo3Bytes;
  be_uint32_t MinStack;
  // 0322
  be_uint32_t DefltStack;
  be_uint16_t MMDefFlags;
  be_uint32_t GZRootHnd;
  be_uint32_t GZRootPtr;
  // 0330
  be_uint32_t GZMoveHnd;
  be_uint32_t DSDrawProc;
  be_uint32_t EjectNotify;
  be_uint32_t IAZNotify;
  // 0340
  be_uint16_t CurDB;
  be_uint16_t NxtDB;
  be_uint16_t MaxDB;
  uint8_t FlushOnly;
  uint8_t RegRsrc;
  uint8_t FLckUnlck;
  uint8_t FrcSync;
  uint8_t NewMount;
  uint8_t NoEject;
  be_uint16_t DrMstrBlk;
  be_uint32_t FCBSPtr;
  // 0352
  be_uint32_t DefVCBPtr;
  Queue VCBQHdr;
  // 0360
  // Note: the head and tail fields of FSQHdr are sometimes referred to as
  // top-level variables names FSQHead and FSQTail
  Queue FSQHdr;
  be_uint32_t HFSStkTop;
  be_uint32_t HFSStkPtr;
  // 0372
  be_uint32_t WDCBsPtr;
  uint8_t HFSFlags;
  uint8_t CacheFlag;
  be_uint32_t SysBMCPtr;
  be_uint32_t SysVolCPtr;
  // 0380
  be_uint32_t SysCtlCPtr;
  be_uint16_t DefVRefNum;
  be_uint32_t PMSPPtr;
  be_uint64_t HFSTagData;
  // 0392
  be_int16_t HFSDSErr;
  be_uint32_t CacheVars;
  be_uint32_t CurDirStore;
  be_uint16_t CacheCom;
  be_uint32_t FmtDefaults;
  // 03A2
  be_int16_t ErCode;
  uint8_t Params[0x32]; // TODO: type here is probably wrong
  // 03D6
  be_uint64_t FSTemp8;
  be_uint32_t FSIOErr;
  // 03E2
  be_uint32_t FSQueueHook;
  be_uint32_t ExtFSHook;
  be_uint32_t DskSwtchHook;
  be_uint32_t ReqstVol;
  // 03F2
  be_uint32_t ToExtFS;
  be_uint16_t FSFCBLen;
  be_int16_t DSAlertRect_y1;
  be_int16_t DSAlertRect_x1;
  be_int16_t DSAlertRect_y2;
  be_int16_t DSAlertRect_x2;
  // 0400
  uint8_t unused6[0x400];
  // 0800
  be_uint32_t JHideCrsr;
  be_uint32_t JShowCrsr;
  be_uint32_t JShieldCrsr;
  be_uint32_t JScrnAddr;
  // 0810
  be_uint32_t JScrnSize;
  be_uint32_t JInitCrsr;
  be_uint32_t JSetCrsr;
  be_uint32_t JCrsrObscure;
  // 0820
  be_uint32_t JUpdateProc;
  be_uint32_t ScrnBase;
  be_uint32_t MTemp;
  be_uint32_t RawMouse;
  // 0830
  be_uint32_t Mouse;
  be_int16_t CrsrPin_y1;
  be_int16_t CrsrPin_x1;
  be_int16_t CrsrPin_y2;
  be_int16_t CrsrPin_x2;
  be_int16_t CrsrRect_y1;
  be_int16_t CrsrRect_x1;
  be_int16_t CrsrRect_y2;
  be_int16_t CrsrRect_x2;
  // 0844
  struct {
    be_uint16_t __lines__[0x10];
    be_uint16_t __mask_lines__[0x10];
    be_uint16_t __hotspot_x__;
    be_uint16_t __hotspot_y__;
  } __attribute__((packed)) TheCrsr;
  // 0888
  be_uint32_t CrsrAddr;
  be_uint32_t CrsrSave_JAllocCrsr_NewCrsrJTbl;
  // 0890
  be_uint32_t JSetCCrsr;
  be_uint32_t JOpcodeProc;
  be_uint32_t CrsrBase;
  be_uint32_t CrsrDevice;
  // 08A0
  be_uint32_t SrcDevice;
  be_uint32_t MainDevice;
  be_uint32_t DeviceList;
  be_uint32_t CrsrRow;
  // 08B0
  be_uint32_t QDColors;
  be_uint32_t unused7[6];
  uint8_t CrsrVis;
  uint8_t CrsrBusy;
  uint8_t CrsrNew;
  uint8_t CrsrCouple;
  // 08D0
  be_uint16_t CrsrState;
  uint8_t CrsrObscure;
  uint8_t CrsrScale;
  be_uint16_t unused8;
  be_uint32_t MouseMask;
  be_uint32_t MouseOffset;
  be_uint16_t JournalFlag;
  // 08E0
  be_uint32_t JSwapFont;
  be_uint32_t JFontInfo;
  be_uint32_t JournalRef;
  be_uint16_t CrsrThresh;
  be_uint32_t JCrsrTask;
  // 08F2
  uint8_t WWExist;
  uint8_t QDExist;
  be_uint32_t JFetch;
  be_uint32_t JStash;
  be_uint32_t JIODone;
  // 0900
  be_uint16_t CurApRefNum;
  uint8_t LaunchFlag;
  uint8_t FondState;
  be_uint32_t CurrentA5;
  be_uint32_t CurStackBase;
  be_uint32_t LoadFiller;
  // 0910
  char CurApName[0x20]; // p-string
  // 0930
  be_uint32_t SaveSegHandle;
  be_int16_t CurJTOffset;
  be_uint16_t CurPageOption;
  be_uint16_t HiliteMode;
  uint8_t LoaderPBlock[0x0A]; // TODO: type is probably wrong here
  // 0944
  be_int16_t PrintErr;
  // Note: the following two fields are sometimes referred to as ChooseBits
  uint8_t PrFlags;
  uint8_t PrType;
  uint8_t unused9[0x0A];
  // 0952
  be_uint16_t PrRefNum;
  uint8_t LastPGlobal[0x0C];
  // 0960
  // Note: the following 6 fields are sometimes referred to as a 32-byte array
  // names ScrapInfo or ScrapVars
  be_uint32_t ScrapSize;
  be_uint32_t ScrapHandle;
  be_uint16_t ScrapCount;
  be_uint16_t ScrapState;
  be_uint32_t ScrapName;
  char ScrapTag[0x10]; // p-string???
  // 0980
  be_uint32_t RomFont0_ScrapEnd;
  be_uint16_t AppFontID;
  uint8_t SaveFondFlags;
  uint8_t FMDefaultSize;
  be_uint16_t CurFMFamily;
  be_uint16_t CurFMSize;
  uint8_t CurFMFace;
  uint8_t CurFMNeedBits;
  be_uint16_t CurFMDevice;
  // 0990
  be_uint32_t CurFMNumer;
  be_uint32_t CurFMDenom;
  be_int16_t FOutError;
  be_uint32_t FOutFontHandle;
  uint8_t FOutBold;
  uint8_t FOutItalic;
  // 09A0
  uint8_t FOutULOffset;
  uint8_t FOutULShadow;
  uint8_t FOutULThick;
  uint8_t FOutShadow;
  uint8_t FOutExtra;
  uint8_t FOutAscent;
  uint8_t FOutDescent;
  uint8_t FOutWidMax;
  uint8_t FOutLeading;
  uint8_t FOutUnused;
  be_uint32_t FOutNumer;
  be_uint32_t FOutDenom;
  // 09B2
  be_uint32_t FMDotsPerInch;
  uint8_t FMStyleTab[0x18]; // TODO: type
  // 09CE
  be_uint64_t ToolScratch;
  be_uint32_t WindowList;
  be_uint16_t SaveUpdate;
  be_uint16_t PaintWhite;
  be_uint32_t WMgrPort;
  // 09E2
  be_uint32_t DeskPort;
  be_uint32_t OldStructure;
  be_uint32_t OldContent;
  be_uint32_t GrayRgn;
  // 09F2
  be_uint32_t SaveVisRgn;
  be_uint32_t DragHook;
  // Note: TempRect is also sometimes referred to as a single 64-bit value named
  // Scratch8
  be_int16_t TempRect_y1;
  be_int16_t TempRect_x1;
  be_int16_t TempRect_y2;
  be_int16_t TempRect_x2;
  // 0A02
  be_uint32_t OneOne;
  be_uint32_t MinusOne;
  be_uint16_t TopMenuItem;
  be_uint16_t AtMenuBottom;
  // 0A0E
  uint8_t IconBitmap[0x0E];
  be_uint32_t MenuList;
  // 0A20
  be_uint16_t MBarEnable;
  be_uint16_t CurDeKind;
  be_uint16_t MenuFlash;
  be_uint16_t TheMenu;
  be_uint32_t SavedHandle;
  be_uint32_t MBarHook;
  // 0A30
  be_uint32_t MenuHook;
  be_uint64_t DragPattern;
  be_uint64_t DeskPattern;
  // 0A44
  be_uint16_t DragFlag;
  be_uint32_t CurDragAction;
  uint8_t FPState[6];
  // 0A50
  be_uint32_t TopMapHndl;
  be_uint32_t SysMapHndl;
  be_uint16_t SysMap;
  be_uint16_t CurMap;
  be_uint16_t ResReadOnly;
  be_uint16_t ResLoad;
  // 0A60
  be_int16_t ResErr;
  uint8_t TaskLock;
  uint8_t FScaleDisable;
  be_uint32_t CurActivate;
  be_uint32_t CurDeactive;
  be_uint32_t DeskHook;
  // 0A70
  be_uint32_t TEDoText;
  be_uint32_t TERecal;
  uint8_t ApplScratch[12];
  // 0A84
  be_uint32_t GhostWindow;
  be_uint32_t CloseOrnHook;
  be_uint32_t ResumeProc_RestProc;
  // 0A90
  be_uint32_t SaveProc;
  be_uint32_t SaveSP;
  be_uint16_t ANumber;
  be_uint16_t ACount;
  be_uint32_t DABeeper;
  // 0AA0
  uint8_t DAStrings[0x10];
  // 0AB0
  be_uint16_t TEScrpLength;
  be_uint16_t unused10;
  be_uint32_t TEScrpHandle;
  uint8_t AppPacks[0x20];
  // 0AD8
  char SysResName[0x10]; // p-string
  be_uint32_t SoundGlue;
  be_uint32_t AppParmHandle;
  // 0AF0
  be_int16_t DSErrCode;
  be_uint32_t ResErrProc;
  be_uint32_t TEWdBreak;
  be_uint16_t DlgFont;
  be_uint32_t LastTGlobal;
  // 0B00
  be_uint32_t TrapAgain;
  be_uint16_t KeyMVars;
  be_uint32_t ROMMapHndl;
  be_uint32_t PWMBuf1;
  be_uint16_t BootMask;
  // 0B10
  be_uint32_t WidthPtr;
  be_uint32_t ATalkHk1;
  be_uint32_t LAPMgrPtr;
  be_uint32_t FourDHack;
  // 0B20
  uint8_t UnSwitchedFlags;
  uint8_t SwitchedFlags;
  be_uint16_t HWCfgFlags;
  be_uint16_t TimeSCSIDB;
  be_uint16_t Top2MenuItem;
  be_uint16_t At2MenuBottom;
  be_uint32_t WidthTabHandle;
  be_uint16_t SCSIDrvrs;
  // 0B30
  be_uint32_t TimeVars;
  be_uint16_t BtDskRfn;
  be_uint64_t BootTmp8;
  uint8_t NTSC;
  uint8_t T1Arbitrate;
  // 0B40
  be_uint32_t JDiskSel;
  be_uint32_t JSendCmd;
  be_uint32_t JDCDReset;
  be_uint32_t LastSPExtra;
  // 0B50
  be_uint32_t FileShareVars;
  be_uint32_t MenuDisable;
  be_uint32_t MBDFHndl;
  be_uint32_t MBSaveLoc;
  // 0B60
  be_uint32_t BNMQHdr;
  be_uint32_t BackgrounderVars;
  be_uint32_t MenuLayer;
  be_uint32_t OmegaSANE;
  // 0B70
  be_uint16_t unused11;
  uint8_t CarlByte;
  uint8_t SystemInfo;
  be_uint32_t unused12;
  be_uint32_t VMGlobals;
  be_uint32_t Twitcher2;
  // 0B80
  be_uint32_t RMgrHiVars;
  be_uint32_t HSCHndl;
  uint8_t PadRsrc[0x12];
  be_uint16_t ResOneDeep;
  be_uint16_t PadRsrc2;
  uint8_t RomMapInsert;
  uint8_t TmpResLoad;
  // 0BA0
  be_uint32_t IntlSpec;
  uint8_t RMgrPerm;
  uint8_t WordRedraw;
  be_uint16_t SysFontFam;
  be_uint16_t DefFontSize;
  be_uint16_t MBarHeight;
  be_uint16_t TESysJust;
  be_uint32_t HiHeapMark;
  // 0BB2
  uint8_t SegHiEnable;
  uint8_t FDevDisable;
  be_uint32_t CommToolboxGlob_CMVector;
  be_uint32_t unused13;
  be_uint32_t ShutDwnQHdr;
  // 0BC0
  be_uint16_t NewUnused;
  be_uint32_t LastFOND;
  be_uint16_t FONDID;
  uint8_t App2Packs[0x20];
  be_uint32_t MAErrProc;
  be_uint32_t MASuperTab;
  // 0BF0
  be_uint32_t MimeGlobs;
  uint8_t FractEnable;
  uint8_t UsedFWidth;
  be_uint32_t FScaleHFact;
  be_uint32_t FScaleVFact;
  uint8_t SCCIOPFlag;
  uint8_t MacJmpFlag;
  // 0C00
  be_uint32_t SCSIBase;
  be_uint32_t SCSIDMA;
  be_uint32_t SCSIHsk;
  be_uint32_t SCSIGlobals;
  // 0C10
  be_int16_t RGBBlack_r;
  be_int16_t RGBBlack_g;
  be_int16_t RGBBlack_b;
  be_int16_t RGBWhite_r;
  be_int16_t RGBWhite_g;
  be_int16_t RGBWhite_b;
  be_uint32_t FMSynth;
  // 0C20
  be_uint16_t RowBits;
  be_uint16_t ColLines;
  be_uint32_t ScreenBytes;
  be_uint32_t IOPMgrVars;
  uint8_t NMIFlag;
  uint8_t VidType;
  uint8_t VidMode;
  uint8_t SCSIPoll;
  // 0C30
  uint8_t SEVarBase[0x3C];
  be_uint32_t MacsBugSP;
  // 0C70
  be_uint32_t MacsBugPC;
  be_uint32_t MacsBugSR;
  uint8_t unused14[0x38];
  // 0CB0
  uint8_t MMUFlags;
  uint8_t MMUType;
  uint8_t MMU32bit;
  uint8_t MMUFluff_MachineType;
  be_uint32_t MMUTbl24_MMUTbl;
  be_uint32_t MMUTbl32_MMUTblSize;
  be_uint32_t SInfoPtr;
  // 0CC0
  be_uint32_t ASCBase;
  be_uint32_t SMGlobals;
  be_uint32_t TheGDevice;
  be_uint32_t CQDGlobals;
  // 0CD0
  be_uint32_t AuxWinHead;
  be_uint32_t AuxCtlHead;
  be_uint32_t DeskCPat;
  be_uint32_t SetOSDefKey;
  // 0CE0
  be_uint64_t LastBinPat;
  be_uint16_t DeskPatEnable;
  be_uint16_t TimeVIADB;
  be_uint32_t VIA2Base;
  // 0CF0
  be_uint64_t VMVectors;
  be_uint32_t ADBBase;
  be_uint32_t WarmStart;
  // 0D00
  be_uint16_t TimeDBRA;
  be_uint16_t TimeSCCDB;
  be_uint32_t SlotQDT;
  be_uint32_t SlotPrTbl;
  be_uint32_t SlotVBLQ;
  // 0D10
  be_uint32_t ScrnVBLPtr;
  be_uint32_t SlotTICKS;
  be_uint32_t PowerMgrVars;
  be_uint32_t AGBHandle;
  // 0D20
  be_uint32_t TableSeed;
  be_uint32_t SRsrcTblPtr;
  be_uint32_t JVBLTask;
  be_uint32_t WMgrCPort;
  // 0D30
  be_uint16_t VertRRate;
  be_uint32_t SynListHandle;
  be_uint32_t LastFore;
  be_uint32_t LastBG;
  be_uint16_t LastMode;
  // 0D40
  be_uint16_t LastDepth;
  uint8_t FMExist;
  uint8_t SavedHilite;
  be_uint64_t unused15;
  be_uint32_t ShieldDepth;
  // 0D50
  be_uint32_t MenuCInfo;
  be_uint32_t MBProcHndl;
  be_uint32_t unused16;
  be_uint32_t MBFlash;
  // 0D60
  be_uint16_t ChunkyDepth;
  be_uint32_t CrsrPtr;
  be_uint32_t PortList;
  be_uint32_t MickeyBytes;
  be_int16_t QDErr;
  // 0D70
  be_uint32_t VIA2DT[8];
  // 0D90
  be_uint16_t SInitFlags;
  // Note: the internal fields of DTQueue are also referred to as top-level
  // variables named DTQFlags, DTskQHdr, and DTskQTail
  Queue DTQueue;
  be_uint32_t JDTInstall;
  // 0DA0
  be_int16_t HiliteRGB_r;
  be_int16_t HiliteRGB_g;
  be_int16_t HiliteRGB_b;
  be_uint16_t OldTimeSCSIDB;
  be_uint32_t DSCtrAdj;
  be_uint32_t IconTLAddr;
  // 0DB0
  be_uint32_t VideoInfoOK;
  be_uint32_t EndSRTPtr;
  be_uint32_t SDMJmpTblPtr;
  be_uint32_t JSwapMMU;
  // 0DC0
  be_uint32_t SdmBusErr;
  be_uint32_t LastTxGDevice;
  be_uint32_t PMgrHandle;
  be_uint32_t LayerPalette;
  // 0DD0
  be_uint32_t AddrMapFlags;
  be_uint32_t UnivROMFlags;
  be_uint32_t UniversalInfoPtr;
  be_uint32_t BootGlobPtr;
  // 0DE0
  be_uint32_t EgretGlobals;
  be_uint32_t SaneTrapAddr;
  be_uint32_t Warhol;
  // 0DEC
  uint8_t unused17[0x1014];
  // 1E00
  uint8_t MemVectors24[0xE0];
  uint8_t Mem2Vectors24[0x10];
  be_uint32_t Phys2Log;
  be_uint32_t RealMemTop;
  be_uint32_t PhysMemTop;
  be_uint32_t MMFlags;
  // 1F00
  uint8_t MemVectors32[0xB8];
  be_uint32_t DrawCrsrVector;
  be_uint32_t EraseCrsrVector;
  // 1FC0
  be_uint32_t PSCIntTbl;
  be_uint32_t DSPGlobals;
  be_uint32_t FP040Vects;
  be_uint32_t FPBSUNVec;
  // 1FD0
  be_uint32_t FPUNFLVec;
  be_uint32_t FPOPERRVec;
  be_uint32_t FPOVFLVec;
  be_uint32_t FPSNANVec;
  // 1FE0
  uint8_t Mem2Vectors32[0x10];
  // 1FF0
  be_uint32_t SCSI2Base;
  be_uint32_t LockMemCt;
  be_uint32_t DockingGlobals;
  be_uint32_t unused18;
  // 2000
  uint8_t VectorPtr[0x400];
  be_uint32_t BasesValid1;
  be_uint32_t BasesValid2;
  be_uint32_t ExtValid1;
  be_uint32_t ExtValid2;
  // 2410
} __attribute__((packed));

const char* name_for_lowmem_global(uint32_t addr);

} // namespace ResourceDASM
