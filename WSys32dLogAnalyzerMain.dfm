object Main: TMain
  Left = 0
  Top = 0
  Caption = #1040#1085#1072#1083#1080#1079' '#1083#1086#1075#1086#1074' WSys32d'
  ClientHeight = 453
  ClientWidth = 683
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -16
  Font.Name = 'Segoe UI'
  Font.Style = []
  Menu = MainMenu
  OldCreateOrder = False
  Position = poDesigned
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 21
  object Splitter: TSplitter
    Left = 0
    Top = 271
    Width = 683
    Height = 4
    Cursor = crVSplit
    Align = alBottom
    ResizeStyle = rsUpdate
    ExplicitTop = 277
  end
  object lvTemperatures: TListView
    Left = 0
    Top = 275
    Width = 683
    Height = 154
    Align = alBottom
    Columns = <>
    ReadOnly = True
    RowSelect = True
    TabOrder = 1
    ViewStyle = vsReport
    OnCompare = lvZerosCompare
    OnCustomDrawSubItem = lvTemperaturesCustomDrawSubItem
    OnDblClick = lvZerosDblClick
  end
  object StatusBar: TStatusBar
    Left = 0
    Top = 429
    Width = 683
    Height = 24
    Panels = <
      item
        Text = 'type'
        Width = 150
      end
      item
        Text = 's/n'
        Width = 120
      end
      item
        Text = 'filename'
        Width = 50
      end>
    ParentFont = True
    UseSystemFont = False
  end
  object lvZeros: TListView
    Left = 0
    Top = 0
    Width = 683
    Height = 271
    Align = alClient
    Columns = <>
    ReadOnly = True
    RowSelect = True
    TabOrder = 0
    ViewStyle = vsReport
    OnCompare = lvZerosCompare
    OnCustomDrawSubItem = lvZerosCustomDrawSubItem
    OnDblClick = lvZerosDblClick
  end
  object MainMenu: TMainMenu
    Left = 64
    Top = 96
    object miMainFile: TMenuItem
      Caption = #1060#1072#1081#1083
      object miFileOpenLog: TMenuItem
        Caption = #1054#1090#1082#1088#1099#1090#1100' '#1083#1086#1075'...'
        ShortCut = 16463
        OnClick = miFileOpenLogClick
      end
      object miSeparator01: TMenuItem
        Caption = '-'
      end
      object miFileClose: TMenuItem
        Caption = #1047#1072#1082#1088#1099#1090#1100
        ShortCut = 32883
        OnClick = miFileCloseClick
      end
    end
    object miMainData: TMenuItem
      Caption = #1044#1072#1085#1085#1099#1077
      object miDataGotoNextError: TMenuItem
        Caption = #1055#1077#1088#1077#1081#1090#1080' '#1082' '#1089#1083#1077#1076#1091#1102#1097#1077#1081' '#1086#1096#1080#1073#1082#1077
        ShortCut = 114
        OnClick = miDataGotoNextErrorClick
      end
      object miDataGotoPrevError: TMenuItem
        Caption = #1055#1077#1088#1077#1081#1090#1080' '#1082' '#1087#1088#1077#1076#1099#1076#1091#1097#1077#1081' '#1086#1096#1080#1073#1082#1077
        ShortCut = 115
        OnClick = miDataGotoPrevErrorClick
      end
      object miSeparator02: TMenuItem
        Caption = '-'
      end
      object miDataFindDateTime: TMenuItem
        Caption = #1057#1086#1087#1086#1089#1090#1072#1074#1080#1090#1100' '#1076#1072#1090#1091' '#1080' '#1074#1088#1077#1084#1103
        ShortCut = 117
        OnClick = miDataFindDateTimeClick
      end
    end
    object miMainHelp: TMenuItem
      Caption = #1057#1087#1088#1072#1074#1082#1072
      object miHelpAbout: TMenuItem
        Caption = #1054' '#1087#1088#1086#1075#1088#1072#1084#1084#1077'...'
        OnClick = miHelpAboutClick
      end
    end
  end
  object OpenDialog: TOpenDialog
    DefaultExt = 'log'
    Filter = 
      #1051#1086#1075#1080' '#1087#1088#1086#1074#1077#1089#1086#1082' (WDyn*.log)|WDyn*.log|'#1042#1089#1077' '#1083#1086#1075#1080' (*.log)|*.log|'#1042#1089#1077' '#1092 +
      #1072#1081#1083#1099'|*.*'
    Options = [ofAllowMultiSelect, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 176
    Top = 96
  end
end
