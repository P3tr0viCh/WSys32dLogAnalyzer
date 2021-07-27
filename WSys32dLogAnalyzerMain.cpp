// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <float.h>

#include <DateUtils.hpp>

#include "WSys32dLogAnalyzerMain.h"

#include "UtilsStr.h"
#include "UtilsMisc.h"
#include "UtilsFileIni.h"

#include "UtilsFilesStr.h"

#include "AboutFrm.h"

#define WD30_LOG_DATETIME 23

// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMain *Main;

// ---------------------------------------------------------------------------
__fastcall TMain::TMain(TComponent * Owner) : TForm(Owner) {
}

// ---------------------------------------------------------------------------
void __fastcall TMain::FormCreate(TObject *Sender) {
	FormatSettings.DecimalSeparator = '.';

	Types = new TStringList();
	Scales = new TStringList();

	UpdateCaptions(-1, -1);
	UpdateStatusBar(NULL, NULL, NULL);

	TFileIni* FileIni = TFileIni::GetNewInstance();

	try {
		FileIni->ReadFormBounds(this);

		ZerosMaxDelta = abs(FileIni->ReadInteger("MaxDelta", "Zeros", 100));
		TemperaturesMaxDelta =
			abs(FileIni->ReadInteger("MaxDelta", "Temperatures", 5));
	}
	__finally {
		delete FileIni;
	}

	if (ParamCount() > 0) {
		TFileName FileName = ParamStr(1);

		if (FileExists(FileName)) {
			OpenLog(FileName);
		}
		else {
			MsgBoxErr(Format(LoadStr(IDS_ERROR_FILE_NOT_FOUND),
				ARRAYOFCONST((FileName))));
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMain::FormDestroy(TObject *Sender) {
	TFileIni* FileIni = TFileIni::GetNewInstance();

	try {
		FileIni->WriteFormBounds(this);
	}
	__finally {
		delete FileIni;
	}

	Scales->Free();
	Types->Free();
}

// ---------------------------------------------------------------------------
String FToS(float F) {
	return FloatToStrF(F, ffFixed, 4, 1);
}

// ---------------------------------------------------------------------------
float SToF(String S) {
	return StrToFloat(S);
}

// ---------------------------------------------------------------------------
String DTToS(TDateTime DT) {
	return FormatDateTime("yyyy.MM.dd hh:mm:ss", DT);
}

// ---------------------------------------------------------------------------
TDateTime SToDT(String DT) {
	String Year = DT.SubString(1, 4);
	String Month = DT.SubString(6, 2);
	String Day = DT.SubString(9, 2);
	String Hour = DT.SubString(12, 2);
	String Min = DT.SubString(15, 2);
	String Sec = DT.SubString(18, 2);

	return EncodeDateTime(StrToInt(Year), StrToInt(Month), StrToInt(Day),
		StrToInt(Hour), StrToInt(Min), StrToInt(Sec), 0);
}

// ---------------------------------------------------------------------------
TListItem* TMain::SetListItemZeros(int Index, String DateTime,
	TStringList * Zeros) {
	TListItem * ListItem = Index < 0 ? lvZeros->Items->Add() :
		lvZeros->Items->Item[Index];

	ListItem->Caption = DTToS(StrToDateTime(DateTime));
	ListItem->SubItems->Clear();

	for (int i = 0; i < Zeros->Count; i++) {
		ListItem->SubItems->Add(Zeros->Strings[i]);
		ListItem->SubItems->Add("");
	}

	return ListItem;
}

// ---------------------------------------------------------------------------
TListItem* TMain::SetListItemTemperatures(int Index, String DateTime,
	TStringList * Temperatures) {
	TListItem * ListItem = Index < 0 ? lvTemperatures->Items->Add() :
		lvTemperatures->Items->Item[Index];

	ListItem->Caption = DTToS(StrToDateTime(DateTime));
	ListItem->SubItems->Clear();

	int C = 0;

	float T, Delta = 0.0, Avr = 0.0, Max = -100, Min = 100;

	for (int i = 0; i < Temperatures->Count; i++) {
		try {
			T = SToF(Temperatures->Strings[i]);
		}
		catch (...) {
			T = 0.0;
		}

		if (T == 0.0) {
			continue;
		}

		C++;

		Avr += T;

		if (T > Max) {
			Max = T;
		}
		if (T < Min) {
			Min = T;
		}
	}

	if (C > 0) {
		Avr /= C;
	}

	if (Avr != 0.0) {
		Delta = Max - Min;
	}

	ListItem->SubItems->Add(FToS(Avr));
	ListItem->SubItems->Add(FToS(Delta));

	ListItem->Cut = Delta > TemperaturesMaxDelta;

	for (int i = 0; i < Temperatures->Count; i++) {
		ListItem->SubItems->Add(Temperatures->Strings[i]);
	}

	return ListItem;
}

// ---------------------------------------------------------------------------
void ListViewAddColumn(TListView* ListView, String Caption, int Width) {
	TListColumn* ListColumn = ListView->Columns->Add();
	ListColumn->Caption = Caption;
	ListColumn->Width = Width;
}

// ---------------------------------------------------------------------------
void TMain::UpdateZerosColumnCount(int Count) {
	lvZeros->Columns->Clear();

	ListViewAddColumn(lvZeros, "Дата и время", 180);

	int ZeroCol = 1;

	for (int i = 0; i < Count * 2; i++) {
		if (i & 1) {
			ListViewAddColumn(lvZeros, L"Δ", 60);
		}
		else {
			ListViewAddColumn(lvZeros, "M" + IntToStr(ZeroCol), 60);
			ZeroCol++;
		}
	}
}

// ---------------------------------------------------------------------------
void TMain::UpdateTemperaturesColumnCount(int Count) {
	lvTemperatures->Columns->Clear();

	ListViewAddColumn(lvTemperatures, "Дата и время", 180);

	ListViewAddColumn(lvTemperatures, "Tср", 80);
	ListViewAddColumn(lvTemperatures, L"Δ", 80);
	for (int i = 0; i < 4; i++) {
		ListViewAddColumn(lvTemperatures, "T" + IntToStr(i + 1), 80);
	}
}

// ---------------------------------------------------------------------------
void TMain::Analyze(TStrings * LogStrings) {
	String LogString;

	char X;

	String Type, ScaleNum;
	String S;
	String D;
	String V;

	int ZerosCount = 0;

	int P, P1;
	int Index;

	TStringList * Strings = new TStringList();

	try {
		for (int i = 0; i < LogStrings->Count; i++) {
			Application->ProcessMessages();

			Strings->Clear();

			S = LogStrings->Strings[i];

			if (S.IsEmpty()) {
				continue;
			}

			if (S.Length() < WD30_LOG_DATETIME + 2) {
				continue;
			}

			X = S[WD30_LOG_DATETIME + 2];

			// ----------- Нули ------------------------------------------------
			if (X == 'Z') {
				D = S.SubString(1, WD30_LOG_DATETIME);

				S.Delete(1, WD30_LOG_DATETIME + 2);

				while (!S.IsEmpty()) {
					V = S.SubString(1, 7);

					Strings->Add(Trim(V));

					S.Delete(1, 7);
				}

				if (Strings->Count > ZerosCount) {
					ZerosCount = Strings->Count;
				}

				SetListItemZeros(-1, D, Strings);

				continue;
			}

			// ----------- Температуры -----------------------------------------
			if (X == 'T') {
				if (S.Length() < WD30_LOG_DATETIME + 4) {
					continue;
				}

				if (S[WD30_LOG_DATETIME + 3] != ' ') {
					continue;
				}

				if (S[WD30_LOG_DATETIME + 4] == ' ') {
					continue;
				}

				D = S.SubString(1, WD30_LOG_DATETIME);

				S.Delete(1, WD30_LOG_DATETIME + 3);

				Index = 1;

				S = S + " ";

				while (!S.IsEmpty()) {
					P = S.Pos(" ");

					V = S.SubString(1, P - 1);

					Strings->Add(Trim(V));

					Index++;

					if (Index > 4) {
						break;
					}

					S.Delete(1, P);
				}

				SetListItemTemperatures(-1, D, Strings);

				continue;
			}

			// ----------- Тип и номер весов -----------------------------------
			if (X == '{') {
				P = S.Pos("s/n");

				if (P == 0) {
					continue;
				}

				ScaleNum = S.SubString(P + 4, S.Length() - P - 4);

				P1 = S.Pos("ВД");

				if (P1 > 0) {
					Type = S.SubString(P1, P - P1 - 9);
				}

				continue;
			}
		} // for
	}
	__finally {
		Strings->Free();
	}

	if (IsEmpty(Type)) {
		Type = "н/д";
	}
	if (IsEmpty(ScaleNum)) {
		ScaleNum = "н/д";
	}

	if (Types->IndexOf(Type) == -1) {
		Types->Add(Type);
	}
	if (Scales->IndexOf(ScaleNum) == -1) {
		Scales->Add(ScaleNum);
	}

	UpdateZerosColumnCount(ZerosCount);
	UpdateTemperaturesColumnCount(4);
}

// ---------------------------------------------------------------------------
void TMain::CalcZerosDeltas() {
	int Zero, ZeroPrev, Delta, AbsDelta;

	String SZero, SZeroPrev;

	bool IsMaxDelta;

	for (int Row = 0; Row < lvZeros->Items->Count - 1; Row++) {
		IsMaxDelta = false;

		for (int Col = 0; Col < lvZeros->Columns->Count - 1; Col++) {
			if (Col & 1) {
				continue;
			}

			if ((Col > lvZeros->Items->Item[Row]->SubItems->Count - 1) ||
				(Col > lvZeros->Items->Item[Row + 1]->SubItems->Count - 1)) {
				continue;
			}

			SZero = lvZeros->Items->Item[Row]->SubItems->Strings[Col];
			SZeroPrev = lvZeros->Items->Item[Row + 1]->SubItems->Strings[Col];

			if (IsEmpty(SZero) || IsEmpty(SZeroPrev)) {
				continue;
			}

			Zero = StrToInt(SZero);
			ZeroPrev = StrToInt(SZeroPrev);

			Delta = Zero - ZeroPrev;

			AbsDelta = abs(Delta);

			if (!IsMaxDelta && AbsDelta > ZerosMaxDelta) {
				IsMaxDelta = true;
			}

			lvZeros->Items->Item[Row]->SubItems->Strings[Col + 1] =
				(Delta == 0 ? "" : Delta < 0 ? "-" : "+") + IntToStr(AbsDelta);
		}

		lvZeros->Items->Item[Row]->Cut = IsMaxDelta;
	}
}

// ---------------------------------------------------------------------------
int TMain::GetZerosErrorCount() {
	int ErrorCount = 0;
	for (int i = 0; i < lvZeros->Items->Count; i++) {
		if (lvZeros->Items->Item[i]->Cut) {
			ErrorCount++;
		}
	}
	return ErrorCount;
}

// ---------------------------------------------------------------------------
int TMain::GetTemperaturesErrorCount() {
	int ErrorCount = 0;
	for (int i = 0; i < lvTemperatures->Items->Count; i++) {
		if (lvTemperatures->Items->Item[i]->Cut) {
			ErrorCount++;
		}
	}
	return ErrorCount;
}

// ---------------------------------------------------------------------------
void TMain::OpenLogs(TStrings * FileNames) {
	UpdateCaptions(-1, -1);
	UpdateStatusBar(NULL, NULL, NULL);

	ShowWaitCursor();

	Types->Clear();
	Scales->Clear();

	lvZeros->Clear();
	lvTemperatures->Clear();

	lvZeros->Items->BeginUpdate();
	lvTemperatures->Items->BeginUpdate();

	TStrings* LogStrings = new TStringList();

	try {
		int Count = FileNames->Count;

		StatusBar->SimplePanel = Count > 1;

		for (int i = 0; i < Count; i++) {
			LogStrings->LoadFromFile(FileNames->Strings[i]);

			Analyze(LogStrings);

			StatusBar->SimpleText =
				Format("Чтение: %d%%", ARRAYOFCONST((Percent(i + 1, Count))));

			ProcMess;
		}

		StatusBar->SimpleText = "Сортировка...";
		lvZeros->AlphaSort();
		lvTemperatures->AlphaSort();

		StatusBar->SimpleText = "Расчёт отклонений...";
		CalcZerosDeltas();

		UpdateCaptions(GetZerosErrorCount(), GetTemperaturesErrorCount());
		UpdateStatusBar(Types, Scales, FileNames);
	}
	__finally {
		LogStrings->Free();

		lvTemperatures->Items->EndUpdate();
		lvZeros->Items->EndUpdate();

		StatusBar->SimpleText = "";
		StatusBar->SimplePanel = false;

		RestoreCursor();
	}
}

// ---------------------------------------------------------------------------
void TMain::OpenLog(TFileName FileName) {
	TStrings * FileNames = new TStringList();
	try {
		FileNames->Add(FileName);
		OpenLogs(FileNames);
	}
	__finally {
		FileNames->Free();
	}
}

const STATUSBAR_TYPES = 0;
const STATUSBAR_SCALES = 1;
const STATUSBAR_FILENAMES = 2;

// ---------------------------------------------------------------------------
void TMain::UpdateStatusBar(TStrings * ATypes, TStrings * AScales,
	TStrings * AFileNames) {
	String STypes, SScales, SFileNames;

	if (ATypes == NULL || ATypes->Count == 0) {
		STypes = "";
	}
	else {
		if (ATypes->Count == 1) {
			STypes = ATypes->Strings[0];
		}
		else {
			STypes = Format("%s и ещё %d",
				ARRAYOFCONST((ATypes->Strings[0], ATypes->Count - 1)));
		}
	}

	if (AScales == NULL || AScales->Count == 0) {
		SScales = "";
	}
	else {
		if (AScales->Count == 1) {
			SScales = Format("№ %s", ARRAYOFCONST((AScales->Strings[0])));
		}
		else {
			if (AScales->Count == 2) {
				SScales =
					Format("№№ %s, %s",
					ARRAYOFCONST((AScales->Strings[0], AScales->Strings[1])));
			}
			else {
				SScales = Format("№№ %s, %s и ещё %d",
					ARRAYOFCONST((AScales->Strings[0], AScales->Strings[1],
					AScales->Count - 2)));
			}
		}
	}

	if (AFileNames == NULL || AFileNames->Count == 0) {
		SFileNames = "";
	}
	else {
		if (AFileNames->Count == 1) {
			SFileNames = ExtractFileName(AFileNames->Strings[0]);
		}
		else {
			if (AFileNames->Count == 2) {
				SFileNames =
					Format("%s, %s",
					ARRAYOFCONST((ExtractFileName(AFileNames->Strings[0]),
					ExtractFileName(AFileNames->Strings[1]))));
			}
			else {
				SFileNames =
					Format("%s, %s и ещё %d",
					ARRAYOFCONST((ExtractFileName(AFileNames->Strings[0]),
					ExtractFileName(AFileNames->Strings[1]),
					AFileNames->Count - 2)));
			}
		}
	}

	StatusBar->Panels->Items[STATUSBAR_TYPES]->Text = STypes;
	StatusBar->Panels->Items[STATUSBAR_SCALES]->Text = SScales;
	StatusBar->Panels->Items[STATUSBAR_FILENAMES]->Text = SFileNames;
}

// ---------------------------------------------------------------------------
void TMain::UpdateCaptions(int ZerosErrorCount, int TemperaturesErrorCount) {
	String Zeros = "";
	String Temperatures = "";

	if (ZerosErrorCount >= 0) {
		if (ZerosErrorCount == 0) {
			Zeros += "Ошибок нет";
		}
		else {
			Zeros += "Ошибок: " + IntToStr(ZerosErrorCount);
		}
	}
	if (TemperaturesErrorCount >= 0) {
		if (TemperaturesErrorCount == 0) {
			Temperatures += "Ошибок нет";
		}
		else {
			Temperatures += "Ошибок: " + IntToStr(TemperaturesErrorCount);
		}
	}

	lblZeros->Caption = Zeros;
	lblTemperatures->Caption = Temperatures;
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miFileCloseClick(TObject * Sender) {
	Close();
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miFileOpenLogClick(TObject * Sender) {
	OpenDialog->FileName = "";

	if (OpenDialog->Execute()) {
		OpenLogs(OpenDialog->Files);
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miHelpAboutClick(TObject * Sender) {
	ShowAbout(16);
}

// ---------------------------------------------------------------------------
void __fastcall TMain::lvTemperaturesCustomDrawSubItem(TCustomListView * Sender,
	TListItem * Item, int SubItem, TCustomDrawState State, bool &DefaultDraw) {
	DefaultDraw = true;

	lvTemperatures->Canvas->Font->Style = SubItem == 2 ?
		TFontStyles() << fsBold : TFontStyles();

	lvTemperatures->Canvas->Font->Color = Item->Cut ? clRed : clWindowText;
}

// ---------------------------------------------------------------------------
void __fastcall TMain::lvZerosCustomDrawSubItem(TCustomListView * Sender,
	TListItem * Item, int SubItem, TCustomDrawState State, bool &DefaultDraw) {
	DefaultDraw = true;

	lvZeros->Canvas->Font->Style = Odd(SubItem) ? TFontStyles() :
		TFontStyles() << fsBold;

	lvZeros->Canvas->Font->Color = clNone; // <<<=== Без этой строки баг
	lvZeros->Canvas->Font->Color = clWindowText;

	if (Item->Cut && SubItem <= Item->SubItems->Count) {
		if (abs(StrToIntDef(Odd(SubItem) ? Item->SubItems->Strings[SubItem] :
			Item->SubItems->Strings[SubItem - 1], 0)) > ZerosMaxDelta) {
			lvZeros->Canvas->Font->Color = clRed;
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMain::lvZerosCompare(TObject * Sender, TListItem * Item1,
	TListItem * Item2, int Data, int &Compare) {
	Compare = SToDT(Item1->Caption) < SToDT(Item2->Caption);
}

// ---------------------------------------------------------------------------
void ListItemSelectAndShow(TListItem * ListItem) {
	ListItem->Focused = true;
	ListItem->Selected = true;
	ListItem->MakeVisible(false);
}

// ---------------------------------------------------------------------------
void TMain::GotoError(bool Next) {
	TListView* ListView;

	if (ActiveControl == lvZeros) {
		ListView = lvZeros;
	}
	else {
		if (ActiveControl == lvTemperatures) {
			ListView = lvTemperatures;
		}
		else {
			return;
		}
	}

	if (ListView->Items->Count == 0) {
		return;
	}

	if (!ListView->Selected) {
		if (ListView->ItemFocused) {
			ListView->Selected = ListView->ItemFocused;
		}
		else {
			ListView->Selected = ListView->Items->Item[0];
		}
	}

	if (Next) {
		for (int i = ListView->Selected->Index + 1;
		i < ListView->Items->Count; i++) {
			if (ListView->Items->Item[i]->Cut) {
				ListItemSelectAndShow(ListView->Items->Item[i]);
				break;
			}
		}
	}
	else {
		for (int i = ListView->Selected->Index - 1; i >= 0; i--) {
			if (ListView->Items->Item[i]->Cut) {
				ListItemSelectAndShow(ListView->Items->Item[i]);
				break;
			}
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miDataGotoNextErrorClick(TObject * Sender) {
	GotoError(true);
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miDataGotoPrevErrorClick(TObject * Sender) {
	GotoError(false);
}

// ---------------------------------------------------------------------------
void __fastcall TMain::miDataFindDateTimeClick(TObject * Sender) {
	TListView* ListViewFrom;
	TListView* ListViewTo;

	if (ActiveControl == lvZeros) {
		ListViewFrom = lvZeros;
		ListViewTo = lvTemperatures;
	}
	else {
		if (ActiveControl == lvTemperatures) {
			ListViewFrom = lvTemperatures;
			ListViewTo = lvZeros;
		}
		else {
			return;
		}
	}

	if (ListViewFrom->Items->Count == 0) {
		return;
	}

	if (!ListViewFrom->Selected) {
		if (ListViewFrom->ItemFocused) {
			ListViewFrom->Selected = ListViewFrom->ItemFocused;
		}
		else {
			ListViewFrom->Selected = ListViewFrom->Items->Item[0];
		}
	}

	TDateTime DateTime = SToDT(ListViewFrom->Selected->Caption);

	for (int i = 0; i < ListViewTo->Items->Count; i++) {
		if (SecondsBetween(SToDT(ListViewTo->Items->Item[i]->Caption),
			DateTime) < 10) {
			ListItemSelectAndShow(ListViewTo->Items->Item[i]);
			break;
		}
	}
}

// ---------------------------------------------------------------------------
void __fastcall TMain::lvZerosDblClick(TObject * Sender) {
	miDataFindDateTime->Click();
}
// ---------------------------------------------------------------------------
