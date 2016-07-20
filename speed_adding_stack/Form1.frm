VERSION 5.00
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Speed Stack"
   ClientHeight    =   6765
   ClientLeft      =   30
   ClientTop       =   390
   ClientWidth     =   9705
   BeginProperty Font 
      Name            =   "Tahoma"
      Size            =   7.5
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   451
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   647
   StartUpPosition =   3  'Windows Default
   Begin VB.Timer Timer1 
      Interval        =   10
      Left            =   4560
      Top             =   480
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Const FloorY As Double = 400
Private Type phyobject
    X As Double
    Y As Double
    oldx As Double
    oldy As Double
    w As Double
    h As Double
    speedx As Double
    speedy As Double
    actived As Boolean
    stand As Boolean
    sourcespeedx As Double
End Type
Dim objects(100) As phyobject
Private Sub Form_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim i As Long
    For i = 0 To 100
        If objects(i).actived = False Then
        With objects(i)
            .actived = True
            .X = X
            .oldx = X
            .Y = 10
            .oldy = 10
            .speedx = 0
            .speedy = 0
            .stand = False
            .w = 32 + Rnd * 96
            .h = 32
            .sourcespeedx = 0.5 + Rnd
        End With
        Exit Sub
    End If
    Next
End Sub
Private Function recttouch(X As Double, Y As Double, w As Double, h As Double, x2 As Double, y2 As Double, w2 As Double, h2 As Double) As Boolean
    If (X + w > x2 And x2 + w2 > X And Y + h > y2 And y2 + h2 > Y) Then recttouch = True
End Function
Private Sub Timer1_Timer()
    
    Dim i As Long, j As Long
    Me.Cls
    Me.Print "Click the screen to create object"
    Me.Line (0, FloorY)-(Me.ScaleWidth, FloorY), RGB(0, 0, 0)
    For i = 0 To 100
        If objects(i).actived Then
            With objects(i)
                If .stand = False Then
                    .speedx = .sourcespeedx
                    .speedy = .speedy + 0.2
                End If
                .stand = False
                .X = .X + .speedx
                .Y = .Y + .speedy
                If .Y + .h > FloorY Then
                    .Y = FloorY - .h
                    .stand = True
                    .speedy = 0
                    .speedx = .sourcespeedx
                End If
                For j = 0 To 100
                    If objects(j).actived And j <> i Then
                        If recttouch(.X, .Y, .w, .h, objects(j).X, objects(j).Y, objects(j).w, objects(j).h) Then
                            If recttouch(.X, .oldy, .w, .h, objects(j).X, objects(j).Y, objects(j).w, objects(j).h) = False Then
                                If .stand = False Then
                                    If .Y + .h / 2 < objects(j).Y + objects(j).h / 2 Then
                                        .Y = objects(j).Y - .h
                                        .stand = True
                                        .speedy = 0
                                        .speedx = .sourcespeedx + objects(j).speedx
                                    Else
                                        .Y = objects(j).Y + objects(j).h
                                    End If
                                End If
                            ElseIf recttouch(.oldx, .Y, .w, .h, objects(j).X, objects(j).Y, objects(j).w, objects(j).h) = False Then
                                If .X + .w / 2 < objects(j).X + objects(j).w / 2 Then
                                    .X = objects(j).X - .w
                                Else
                                    .X = objects(j).X + objects(j).w
                                End If
                            End If
                        End If
                    End If
                Next
                Me.Line (.X, .Y)-(.X + .w, .Y + .h), RGB(0, 0, 0), B
                Me.Line (.X, .Y)-(.X + .w, .Y + .h), RGB(0, 0, 0)
                Me.Line (.X + .w, .Y)-(.X, .Y + .h), RGB(0, 0, 0)
                .oldx = .X
                .oldy = .Y
                If recttouch(.X, .Y, .w, .h, 0, 0, Me.ScaleWidth, Me.ScaleHeight) = False Then
                    .actived = False
                End If
            End With
        End If
    Next
    Me.Refresh
End Sub

