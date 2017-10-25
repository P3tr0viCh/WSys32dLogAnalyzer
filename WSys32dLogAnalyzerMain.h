// ---------------------------------------------------------------------------

#ifndef WSys32dLogAnalyzerMainH
#define WSys32dLogAnalyzerMainH
// ---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ExtCtrls.hpp>

// ---------------------------------------------------------------------------
class TMain : public TForm {
__published: // IDE-managed Components

	TStatusBar *StatusBar;
	TMainMenu *MainMenu;
	TMenuItem *miMainFile;
	TMenuItem *miFileClose;
	TMenuItem *miSeparator01;
	TMenuItem *miFileOpenLog;
	TOpenDialog *OpenDialog;
	TMenuItem *miMainHelp;
	TMenuItem *miHelpAbout;
	TMenuItem *miMainData;
	TMenuItem *miDataGotoNextError;
	TMenuItem *miDataGotoPrevError;
	TMenuItem *miSeparator02;
	TMenuItem *miDataFindDateTime;
	TListView *lvZeros;
	TListView *lvTemperatures;
	TSplitter *Splitter;
	TPanel *pnlZeros;
	TPanel *pnlBottom;
	TPanel *pnlTemperatures;
	TLabel *lblZerosCaption;
	TLabel *lblZeros;
	TLabel *lblTemperaturesCaption;
	TLabel *lblTemperatures;

	void __fastcall miFileCloseClick(TObject *Sender);
	void __fastcall miFileOpenLogClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall miHelpAboutClick(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall lvTemperaturesCustomDrawSubItem(TCustomListView *Sender,
		TListItem *Item, int SubItem, TCustomDrawState State,
		bool &DefaultDraw);
	void __fastcall lvZerosCustomDrawSubItem(TCustomListView *Sender,
		TListItem *Item, int SubItem, TCustomDrawState State,
		bool &DefaultDraw);
	void __fastcall lvZerosCompare(TObject *Sender, TListItem *Item1,
		TListItem *Item2, int Data, int &Compare);
	void __fastcall miDataGotoNextErrorClick(TObject *Sender);
	void __fastcall miDataGotoPrevErrorClick(TObject *Sender);
	void __fastcall miDataFindDateTimeClick(TObject *Sender);
	void __fastcall lvZerosDblClick(TObject *Sender);

private: // User declarations

	int ZerosMaxDelta;
	int TemperaturesMaxDelta;

	TStrings* Types;
	TStrings* Scales;

	void UpdateZerosColumnCount(int Count);
	void UpdateTemperaturesColumnCount(int Count);

	TListItem* SetListItemZeros(int Index, String DateTime,
		std::vector<String>Zeros);
	TListItem* SetListItemTemperatures(int Index, String DateTime,
		std::vector<String>Temperatures);

	void Analyze(TStrings* LogStrings);
	void CalcZerosDeltas();

	int GetZerosErrorCount();
	int GetTemperaturesErrorCount();

	void OpenLog(TFileName FileName);
	void OpenLogs(TStrings* FileNames);

	void GotoError(bool Next);

	void UpdateStatusBar(TStrings* ATypes, TStrings* AScales,
		TStrings* AFileNames);
	void UpdateCaptions(int ZerosErrorCount, int TemperaturesErrorCount);

public: // User declarations
	__fastcall TMain(TComponent* Owner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TMain *Main;
// ---------------------------------------------------------------------------
#endif
