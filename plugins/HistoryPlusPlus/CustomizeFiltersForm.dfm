object fmCustomizeFilters: TfmCustomizeFilters
  Left = 227
  Top = 70
  BorderStyle = bsDialog
  Caption = 'Customize Filters'
  ClientHeight = 466
  ClientWidth = 370
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poOwnerFormCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object paClient: TPanel
    Left = 0
    Top = 0
    Width = 370
    Height = 466
    Align = alClient
    BevelOuter = bvNone
    BorderWidth = 4
    TabOrder = 0
    DesignSize = (
      370
      466)
    object bnCancel: TButton
      Left = 89
      Top = 433
      Width = 75
      Height = 25
      Anchors = [akLeft, akBottom]
      Cancel = True
      Caption = '&Cancel'
      TabOrder = 0
      OnClick = bnCancelClick
    end
    object bnOK: TButton
      Left = 8
      Top = 433
      Width = 75
      Height = 25
      Anchors = [akLeft, akBottom]
      Caption = '&OK'
      Default = True
      TabOrder = 2
      OnClick = bnOKClick
    end
    object bnReset: TButton
      Left = 231
      Top = 433
      Width = 131
      Height = 25
      Anchors = [akRight, akBottom]
      Caption = '&Reset to Default'
      TabOrder = 1
      OnClick = bnResetClick
    end
  end
end
