﻿// ---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <float.h>

#include <DateUtils.hpp>

#include <boost/regex.hpp>

#include "WSys32dLogAnalyzerMain.h"

#include "UtilsStr.h"
#include "UtilsMisc.h"
#include "UtilsFileIni.h"

#include "UtilsFilesStr.h"

#include "AboutFrm.h"

// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMain *Main;

const wchar_t* CRegexStart = L".+START.+ (.+)-\\d+ [(]SCALES s[\\/]n (\\d+)[)]";
const wchar_t* CRegexStop = L".+STOP.+";

const wchar_t* CRegexZeros =
	L"(\\d{2}\\.\\d{2}\\.\\d{4}.\\d{2}\\:\\d{2}\\:\\d{2}\\.\\d{3})[ ]Z[ ](.+)";
const wchar_t* CRegexTemperatures =
	L"(\\d{2}\\.\\d{2}\\.\\d{4}.\\d{2}\\:\\d{2}\\:\\d{2}\\.\\d{3})[ ]T[ ](.+)";

const wchar_t* CRegexIntNumber = L"[-]?\\d+";
const wchar_t* CRegexFloatNumber = L"[-]?\\d+[.]\\d+";

// ---------------------------------------------------------------------------
__fastcall TMain::TMain(TComponent * Owner) : TForm(Owner) {
}

// ---------------------------------------------------------------------------
void __fastcall TMain::FormCreate(TObject *Sender) {
	FormatSettings.DecimalSeparator = '.';

	Types = new TStringList();
	Scales = new TStringList();

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
	std::vector<String>Zeros) {
	TListItem* ListItem = Index < 0 ? lvZeros->Items->Add() :
		lvZeros->Items->Item[Index];

	ListItem->Caption = DTToS(StrToDateTime(DateTime));
	ListItem->SubItems->Clear();

	int Count = Zeros.size();

	if (Count > 0) {
		for (std::vector<String>::iterator Zero = Zeros.begin();
		Zero != Zeros.end(); Zero++) {
			ListItem->SubItems->Add(*Zero);
			ListItem->SubItems->Add("");
		}
	}

	return ListItem;
}

// ---------------------------------------------------------------------------
TListItem* TMain::SetListItemTemperatures(int Index, String DateTime,
	std::vector<String>Temperatures) {
	TListItem* ListItem = Index < 0 ? lvTemperatures->Items->Add() :
		lvTemperatures->Items->Item[Index];

	ListItem->Caption = DTToS(StrToDateTime(DateTime));
	ListItem->SubItems->Clear();

	int Count = Temperatures.size();

	if (Count > 0) {
		if (Count > 4) {
			Count = 4;
		}

		float T, Avr = 0, Max = FLT_MIN, Min = FLT_MAX;

		for (int i = 0; i < Count; i++) {
			T = SToF(Temperatures[i]);
			Avr += T;

			if (T > Max) {
				Max = T;
			}
			if (T < Min) {
				Min = T;
			}
		}
		Avr /= Count;

		ListItem->SubItems->Add(FToS(Avr));

		float Delta = fabs(Max - Min);

		ListItem->SubItems->Add(FToS(Delta));

		ListItem->Cut = Delta > TemperaturesMaxDelta;

		for (std::vector<String>::iterator Temperature = Temperatures.begin();
		Temperature != Temperatures.end(); Temperature++) {
			ListItem->SubItems->Add(*Temperature);
		}
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
			ListViewAddColumn(lvZeros, "Z" + IntToStr(ZeroCol), 60);
			ZeroCol++;
		}
	}
}

// ---------------------------------------------------------------------------
void TMain::UpdateTemperaturesColumnCount(int Count) {
	lvTemperatures->Columns->Clear();

	ListViewAddColumn(lvTemperatures, "Дата и время", 180);

	ListViewAddColumn(lvTemperatures, "Tср", 80);
	ListViewAddColumn(lvTemperatures, L"Δм", 80);
	for (int i = 0; i < 4; i++) {
		ListViewAddColumn(lvTemperatures, "T" + IntToStr(i + 1), 80);
	}
}

// ---------------------------------------------------------------------------
void TMain::Analyze(TStrings* LogStrings) {
	boost::wregex RegexStart(CRegexStart);
	boost::wregex RegexStop(CRegexStop);
	boost::wregex RegexZeros(CRegexZeros);
	boost::wregex RegexTemperatures(CRegexTemperatures);
	boost::wregex RegexIntNumber(CRegexIntNumber);
	boost::wregex RegexFloatNumber(CRegexFloatNumber);

	std::wstring LogString, S;

	boost::wsmatch Match;

	String Type, ScaleNum;
	String DateTime;
	std::vector<String>VectorStrings;

	bool InBlock = false;

	int ZerosCount = 0;

	for (int i = 0; i < LogStrings->Count; i++) {
		Application->ProcessMessages();

		LogString = LogStrings->Strings[i].c_str();

		if (boost::regex_match(LogString, Match, RegexStart)) {
			InBlock = true;

			if (IsEmpty(Type)) {
				Type = Match[1].str().c_str();
				ScaleNum = Match[2].str().c_str();
			}

			continue;
		}

		if (boost::regex_match(LogString, Match, RegexStop)) {
			InBlock = false;

			continue;
		}

		if (!InBlock) {
			continue;
		}

		if (boost::regex_match(LogString, Match, RegexZeros)) {
			DateTime = Match[1].str().c_str();

			S = Match[2].str();

			boost::wsregex_token_iterator iter(S.begin(), S.end(),
				RegexIntNumber, 0);
			boost::wsregex_token_iterator end;

			VectorStrings.clear();

			for (; iter != end; iter++) {
				VectorStrings.push_back(((std::wstring)*iter).c_str());
			}

			if ((int)VectorStrings.size() > ZerosCount) {
				ZerosCount = VectorStrings.size();
			}

			SetListItemZeros(-1, DateTime, VectorStrings);

			continue;
		}

		if (boost::regex_match(LogString, Match, RegexTemperatures)) {
			DateTime = Match[1].str().c_str();

			S = Match[2].str();

			boost::wsregex_token_iterator iter(S.begin(), S.end(),
				RegexFloatNumber, 0);
			boost::wsregex_token_iterator end;

			VectorStrings.clear();

			for (; iter != end; iter++) {
				VectorStrings.push_back(((std::wstring)*iter).c_str());
			}

			SetListItemTemperatures(-1, DateTime, VectorStrings);

			continue;
		}
	} // for

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
void TMain::OpenLogs(TStrings * FileNames) {
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
				Format("Анализ: %d%%", ARRAYOFCONST((Percent(i + 1, Count))));
		}

		UpdateStatusBar(Types, Scales, FileNames);

		lvZeros->AlphaSort();
		lvTemperatures->AlphaSort();

		CalcZerosDeltas();
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
void ListItemSelectAndShow(TListItem* ListItem) {
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
void __fastcall TMain::miDataFindDateTimeClick(TObject *Sender) {
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
void __fastcall TMain::lvZerosDblClick(TObject *Sender) {
	miDataFindDateTime->Click();
}
// ---------------------------------------------------------------------------