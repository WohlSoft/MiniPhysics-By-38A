VERSION 5.00
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Example"
   ClientHeight    =   4545
   ClientLeft      =   45
   ClientTop       =   390
   ClientWidth     =   5625
   BeginProperty Font 
      Name            =   "Tahoma"
      Size            =   8.25
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4545
   ScaleWidth      =   5625
   StartUpPosition =   2  'CenterScreen
   Begin VB.Timer Timer1 
      Interval        =   20
      Left            =   2760
      Top             =   1800
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Declare Function CreateCompatibleDC Lib "gdi32" (ByVal hdc As Long) As Long
Private Declare Function DeleteDC Lib "gdi32" (ByVal hdc As Long) As Long
Private Declare Function SelectObject Lib "gdi32" (ByVal hdc As Long, ByVal hObject As Long) As Long
Private Declare Function BitBlt Lib "gdi32" (ByVal hDestDC As Long, ByVal x As Long, ByVal y As Long, ByVal nWidth As Long, ByVal nHeight As Long, ByVal hSrcDC As Long, ByVal xSrc As Long, ByVal ySrc As Long, ByVal dwRop As Long) As Long
Private Declare Function GetAsyncKeyState Lib "user32" (ByVal vKey As Long) As Integer

Private Const SRCCOPY = &HCC0020 ' (DWORD) dest = source
Private Const SRCPAINT = &HEE0086        ' (DWORD) dest = source OR dest
Private Const SRCAND = &H8800C6  ' (DWORD) dest = source AND dest
Dim dc(1 To 9) As Long

Private Enum shapes
    SL_Rect = 1
    SL_RightBottom = 2
    SL_LeftBottom = 4
    SL_LeftTop = 6
    SL_RightTop = 8
End Enum

Private Type obj
    id As Long
    dx As Double
    dy As Double
    x As Double
    y As Double
    w As Double
    h As Double
    spx As Double
    spy As Double
    st As Boolean
End Type

Dim objs() As obj
Dim pl As obj
Const vbKeyLeft As Long = 37
Const vbKeyRight As Long = 39
Const vbKeyDown As Long = 40
Const vbKeyUp As Long = 38

Private Sub SetObj(x As Long, y As Long, id As Long, cid As Long)
With objs(cid)
    .x = x * 32
    .y = y * 32
    .w = 32
    .h = 32
    .id = id
End With
End Sub
Private Sub Form_Load()
Dim i&

For i = 1 To 9
    dc(i) = CreateCompatibleDC(Me.hdc)
    Call SelectObject(dc(i), LoadPicture(App.Path & "\gfx\" & i & ".gif"))
Next

ReDim objs(29)
    Call SetObj(7, 6, SL_RightBottom, 1)
    Call SetObj(8, 5, SL_RightBottom, 2)
    Call SetObj(1, 5, SL_LeftBottom, 3)
    Call SetObj(2, 6, SL_LeftBottom, 4)
    Call SetObj(1, 2, SL_LeftTop, 5)
    Call SetObj(2, 1, SL_LeftTop, 6)
    Call SetObj(7, 1, SL_RightTop, 7)
    Call SetObj(8, 2, SL_RightTop, 8)
    Call SetObj(2, 0, SL_Rect, 9)
    Call SetObj(3, 0, SL_Rect, 10)
    Call SetObj(4, 0, SL_Rect, 11)
    Call SetObj(6, 0, SL_Rect, 12)
    Call SetObj(2, 7, SL_Rect, 13)
    Call SetObj(3, 7, SL_Rect, 14)
    Call SetObj(4, 7, SL_Rect, 15)
    Call SetObj(6, 7, SL_Rect, 16)
    Call SetObj(0, 2, SL_Rect, 17)
    Call SetObj(0, 3, SL_Rect, 18)
    Call SetObj(0, 4, SL_Rect, 19)
    Call SetObj(0, 5, SL_Rect, 20)
    Call SetObj(9, 2, SL_Rect, 21)
    Call SetObj(10, 3, SL_Rect, 22)
    Call SetObj(10, 4, SL_Rect, 23)
    Call SetObj(9, 5, SL_Rect, 24)
    Call SetObj(1, 1, SL_Rect, 25)
    Call SetObj(1, 6, SL_Rect, 26)
    Call SetObj(8, 1, SL_Rect, 27)
    Call SetObj(8, 6, SL_Rect, 28)
    
    Call SetObj(4, 4, SL_Rect, 29)
    
    With objs(29)
        .spx = 0.8
    End With
    
    Me.Width = Me.Width - Me.ScaleWidth + 20 * 32 * 8
    Me.Height = Me.Height - Me.ScaleHeight + 15 * 32 * 8
    pl.x = 3.5 * 32
    pl.y = 4 * 32
    pl.dx = pl.x
    pl.dy = pl.y
    pl.w = 32
    pl.h = 32
End Sub

Private Sub Form_Unload(Cancel As Integer)
Dim i&
For i = 1 To 9
    Call DeleteDC(dc(i))
Next
End Sub
Private Function pt(x1 As Double, y1 As Double, w1 As Double, h1 As Double, x2 As Double, y2 As Double, w2 As Double, h2 As Double) As Boolean
If (y1 + h1 > y2) And (y2 + h2 > y1) Then pt = (x1 + w1 > x2) And (x2 + w2 > x1)
End Function

Private Sub Timer1_Timer()
Dim i&, lk As Boolean, rk As Boolean, ck&, tm&, td&, cn&, k!

With objs(29)
    .dx = .x
    .x = .x + .spx
    If (.x > 6 * 32 Or .x < 2 * 32) Then
        .spx = .spx * -1
    End If
End With

With pl
    lk = GetAsyncKeyState(vbKeyLeft) < 0
    rk = GetAsyncKeyState(vbKeyRight) < 0
    If lk Xor rk Then
        If lk And .spx > -6 Then .spx = .spx - 0.4
        If rk And .spx < 6 Then .spx = .spx + 0.4
    ElseIf lk = False And rk = False Then
        If Abs(.spx) > 0.4 Then .spx = .spx - Sgn(.spx) * 0.4 Else .spx = 0
    End If
    If .spy < 8 And .st = False Then .spy = .spy + 0.4
    If .st And GetAsyncKeyState(vbKeyUp) < 0 Then .spy = -10 '8
    .st = False
    .dx = .x
    .dy = .y
    .x = .x + .spx
    .y = .y + .spy
End With

tm = 0
td = 0

For i = 1 To 29
    cn = 0
    If (pl.x + pl.w > objs(i).x) And (objs(i).x + objs(i).w > pl.x) Then
        If pl.y + pl.h = objs(i).y Then GoTo tipa
    End If
    
tipc:
    If pt(pl.x, pl.y, pl.w, pl.h, objs(i).x, objs(i).y, objs(i).w, objs(i).h) Then
        lk = pt(pl.x, pl.dy, pl.w, pl.h, objs(i).x, objs(i).y - objs(i).spy, objs(i).w, objs(i).h)
        rk = pt(pl.dx, pl.y, pl.w, pl.h, objs(i).x - objs(i).spx, objs(i).y, objs(i).w, objs(i).h)
        If lk Xor rk Then
            
            If lk = False Then
tipa:
                If pl.y + pl.h / 2 < objs(i).y + objs(i).h / 2 Then
                    'top
                    If objs(i).id = SL_Rect Or objs(i).id = SL_LeftTop Or objs(i).id = SL_RightTop Then
tipd:
                        pl.y = objs(i).y - pl.h
                        pl.spy = objs(i).spy
                        pl.st = True
                        cn = 1
                    End If
                Else
                    'bottom
                    If objs(i).id = SL_Rect Or objs(i).id = SL_LeftBottom Or objs(i).id = SL_RightBottom Then
tipe:
                        pl.y = objs(i).y + objs(i).h
                        pl.spy = objs(i).spy
                        cn = 2
                    End If
                End If
            Else
tipb:
                If pl.x + pl.w / 2 < objs(i).x + objs(i).w / 2 Then
                    'left
                    If objs(i).id = SL_Rect Or objs(i).id = SL_LeftBottom Or objs(i).id = SL_LeftTop Then
                        pl.x = objs(i).x - pl.w
                        pl.spx = objs(i).spx
                        cn = 3
                    End If
                Else
                    'right
                    If objs(i).id = SL_Rect Or objs(i).id = SL_RightBottom Or objs(i).id = SL_RightTop Then
                        pl.x = objs(i).x + objs(i).w
                        pl.spx = objs(i).spx
                        cn = 4
                    End If
                End If
            End If
            tm = -1
TipF:
            If cn = 0 Then
                k = objs(i).h / objs(i).w
                Select Case objs(i).id
                Case SL_LeftBottom
                    If pl.x <= objs(i).x Then
                        If pl.y + pl.h > objs(i).y Then GoTo tipd
                    ElseIf pl.y + pl.h > objs(i).y + (pl.x - objs(i).x) * k Then
                        pl.y = objs(i).y + (pl.x - objs(i).x) * k - pl.h
                        pl.spy = objs(i).spy
                        If pl.spx > 0 Then pl.spy = pl.spx * k
                        pl.st = True
                        cn = 1
                    End If
                Case SL_RightBottom
                    If pl.x + pl.w >= objs(i).x + objs(i).w Then
                        If pl.y + pl.h > objs(i).y Then GoTo tipd
                    ElseIf pl.y + pl.h > objs(i).y + (objs(i).x + objs(i).w - pl.x - pl.w) * k Then
                        pl.y = objs(i).y + (objs(i).x + objs(i).w - pl.x - pl.w) * k - pl.h
                        pl.spy = objs(i).spy
                        If pl.spx < 0 Then pl.spy = -pl.spx * k
                        pl.st = True
                        cn = 1
                    End If
                Case SL_LeftTop
                    If pl.x <= objs(i).x Then
                        If pl.y < objs(i).y + objs(i).h Then GoTo tipe
                    ElseIf pl.y < objs(i).y + objs(i).h - (pl.x - objs(i).x) * k Then
                        pl.y = objs(i).y + objs(i).h - (pl.x - objs(i).x) * k
                        pl.spy = objs(i).spy
                        cn = 2
                    End If
                Case SL_RightTop
                    If pl.x + pl.w >= objs(i).x + objs(i).w Then
                        If pl.y < objs(i).y + objs(i).h Then GoTo tipe
                    ElseIf pl.y < objs(i).y + objs(i).h - (objs(i).x + objs(i).w - pl.x - pl.w) * k Then
                        pl.y = objs(i).y + objs(i).h - (objs(i).x + objs(i).w - pl.x - pl.w) * k
                        pl.spy = objs(i).spy
                        cn = 2
                    End If
                End Select
            End If
        Else
            If objs(i).id <> SL_Rect Then GoTo TipF
            If lk = False And rk = False Then
                If tm = i Then
                    If Abs(pl.spy) > Abs(pl.spx) Then
                        GoTo tipa
                    Else
                        GoTo tipb
                    End If
                Else
                    If tm <> -1 Then tm = i
                End If
            End If
        End If
    End If
    If td = 1 Then tm = 0: Exit For
Next
If tm > 0 Then
    i = tm
    td = 1
    GoTo tipc
End If
Me.Cls
For i = 1 To 29
    If objs(i).id = 1 Then
        Call BitBlt(Me.hdc, CLng(objs(i).x), CLng(objs(i).y), CLng(objs(i).w), CLng(objs(i).h), dc(1), 0, 0, SRCCOPY)
    Else
        Call BitBlt(Me.hdc, CLng(objs(i).x), CLng(objs(i).y), CLng(objs(i).w), CLng(objs(i).h), dc(objs(i).id + 1), 0, 0, SRCAND)
        Call BitBlt(Me.hdc, CLng(objs(i).x), CLng(objs(i).y), CLng(objs(i).w), CLng(objs(i).h), dc(objs(i).id), 0, 0, SRCPAINT)
    End If
Next
Call BitBlt(Me.hdc, CLng(pl.x), CLng(pl.y), CLng(pl.w), CLng(pl.h), dc(1), 0, 0, SRCCOPY)
Me.Refresh
End Sub
