bits 64
default rel

segment .data
   fmt_int   DB "%lld", 0
   fmt_float DB "%lf", 0
   fmt_string DB "%s", 0
   fmt_address DB "0x%p", 0
   string_false DB "false", 0
   string_true  DB "true", 0
   string_assert_fail  DB "Assertion failed at line %d", 10, 0
   __KeyboardKey.NULL DB ".NULL", 0
   __KeyboardKey.APOSTROPHE DB ".APOSTROPHE", 0
   __KeyboardKey.COMMA DB ".COMMA", 0
   __KeyboardKey.MINUS DB ".MINUS", 0
   __KeyboardKey.PERIOD DB ".PERIOD", 0
   __KeyboardKey.SLASH DB ".SLASH", 0
   __KeyboardKey.ZERO DB ".ZERO", 0
   __KeyboardKey.ONE DB ".ONE", 0
   __KeyboardKey.TWO DB ".TWO", 0
   __KeyboardKey.THREE DB ".THREE", 0
   __KeyboardKey.FOUR DB ".FOUR", 0
   __KeyboardKey.FIVE DB ".FIVE", 0
   __KeyboardKey.SIX DB ".SIX", 0
   __KeyboardKey.SEVEN DB ".SEVEN", 0
   __KeyboardKey.EIGHT DB ".EIGHT", 0
   __KeyboardKey.NINE DB ".NINE", 0
   __KeyboardKey.SEMICOLON DB ".SEMICOLON", 0
   __KeyboardKey.EQUAL DB ".EQUAL", 0
   __KeyboardKey.A DB ".A", 0
   __KeyboardKey.B DB ".B", 0
   __KeyboardKey.C DB ".C", 0
   __KeyboardKey.D DB ".D", 0
   __KeyboardKey.E DB ".E", 0
   __KeyboardKey.F DB ".F", 0
   __KeyboardKey.G DB ".G", 0
   __KeyboardKey.H DB ".H", 0
   __KeyboardKey.I DB ".I", 0
   __KeyboardKey.J DB ".J", 0
   __KeyboardKey.K DB ".K", 0
   __KeyboardKey.L DB ".L", 0
   __KeyboardKey.M DB ".M", 0
   __KeyboardKey.N DB ".N", 0
   __KeyboardKey.O DB ".O", 0
   __KeyboardKey.P DB ".P", 0
   __KeyboardKey.Q DB ".Q", 0
   __KeyboardKey.R DB ".R", 0
   __KeyboardKey.S DB ".S", 0
   __KeyboardKey.T DB ".T", 0
   __KeyboardKey.U DB ".U", 0
   __KeyboardKey.V DB ".V", 0
   __KeyboardKey.W DB ".W", 0
   __KeyboardKey.X DB ".X", 0
   __KeyboardKey.Y DB ".Y", 0
   __KeyboardKey.Z DB ".Z", 0
   __KeyboardKey.LEFT_BRACKET DB ".LEFT_BRACKET", 0
   __KeyboardKey.BACKSLASH DB ".BACKSLASH", 0
   __KeyboardKey.RIGHT_BRACKET DB ".RIGHT_BRACKET", 0
   __KeyboardKey.GRAVE DB ".GRAVE", 0
   __KeyboardKey.SPACE DB ".SPACE", 0
   __KeyboardKey.ESCAPE DB ".ESCAPE", 0
   __KeyboardKey.ENTER DB ".ENTER", 0
   __KeyboardKey.TAB DB ".TAB", 0
   __KeyboardKey.BACKSPACE DB ".BACKSPACE", 0
   __KeyboardKey.INSERT DB ".INSERT", 0
   __KeyboardKey.DELETE DB ".DELETE", 0
   __KeyboardKey.RIGHT DB ".RIGHT", 0
   __KeyboardKey.LEFT DB ".LEFT", 0
   __KeyboardKey.DOWN DB ".DOWN", 0
   __KeyboardKey.UP DB ".UP", 0
   __KeyboardKey.PAGE_UP DB ".PAGE_UP", 0
   __KeyboardKey.PAGE_DOWN DB ".PAGE_DOWN", 0
   __KeyboardKey.HOME DB ".HOME", 0
   __KeyboardKey.END DB ".END", 0
   __KeyboardKey.CAPS_LOCK DB ".CAPS_LOCK", 0
   __KeyboardKey.SCROLL_LOCK DB ".SCROLL_LOCK", 0
   __KeyboardKey.NUM_LOCK DB ".NUM_LOCK", 0
   __KeyboardKey.PRINT_SCREEN DB ".PRINT_SCREEN", 0
   __KeyboardKey.PAUSE DB ".PAUSE", 0
   __KeyboardKey.F1 DB ".F1", 0
   __KeyboardKey.F2 DB ".F2", 0
   __KeyboardKey.F3 DB ".F3", 0
   __KeyboardKey.F4 DB ".F4", 0
   __KeyboardKey.F5 DB ".F5", 0
   __KeyboardKey.F6 DB ".F6", 0
   __KeyboardKey.F7 DB ".F7", 0
   __KeyboardKey.F8 DB ".F8", 0
   __KeyboardKey.F9 DB ".F9", 0
   __KeyboardKey.F10 DB ".F10", 0
   __KeyboardKey.F11 DB ".F11", 0
   __KeyboardKey.F12 DB ".F12", 0
   __KeyboardKey.LEFT_SHIFT DB ".LEFT_SHIFT", 0
   __KeyboardKey.LEFT_CONTROL DB ".LEFT_CONTROL", 0
   __KeyboardKey.LEFT_ALT DB ".LEFT_ALT", 0
   __KeyboardKey.LEFT_SUPER DB ".LEFT_SUPER", 0
   __KeyboardKey.RIGHT_SHIFT DB ".RIGHT_SHIFT", 0
   __KeyboardKey.RIGHT_CONTROL DB ".RIGHT_CONTROL", 0
   __KeyboardKey.RIGHT_ALT DB ".RIGHT_ALT", 0
   __KeyboardKey.RIGHT_SUPER DB ".RIGHT_SUPER", 0
   __KeyboardKey.KB_MENU DB ".KB_MENU", 0
   __KeyboardKey.KP_0 DB ".KP_0", 0
   __KeyboardKey.KP_1 DB ".KP_1", 0
   __KeyboardKey.KP_2 DB ".KP_2", 0
   __KeyboardKey.KP_3 DB ".KP_3", 0
   __KeyboardKey.KP_4 DB ".KP_4", 0
   __KeyboardKey.KP_5 DB ".KP_5", 0
   __KeyboardKey.KP_6 DB ".KP_6", 0
   __KeyboardKey.KP_7 DB ".KP_7", 0
   __KeyboardKey.KP_8 DB ".KP_8", 0
   __KeyboardKey.KP_9 DB ".KP_9", 0
   __KeyboardKey.KP_DECIMAL DB ".KP_DECIMAL", 0
   __KeyboardKey.KP_DIVIDE DB ".KP_DIVIDE", 0
   __KeyboardKey.KP_MULTIPLY DB ".KP_MULTIPLY", 0
   __KeyboardKey.KP_SUBTRACT DB ".KP_SUBTRACT", 0
   __KeyboardKey.KP_ADD DB ".KP_ADD", 0
   __KeyboardKey.KP_ENTER DB ".KP_ENTER", 0
   __KeyboardKey.KP_EQUAL DB ".KP_EQUAL", 0
   CS0 DB "Super Breakout", 0 
   CS1 DB "Super Breakout", 0 
   CF2 DD 0.5000000
   CF3 DD 0.5000000
   CF4 DD 400.0000000
   CF5 DD 400.0000000
   CF6 DD 0.5000000
   CF7 DD 0.5000000
   CF8 DD 70.0000000
   CF9 DD 20.0000000
   CF10 DD 0.0000000
   CF11 DD 400.0000000
   CF12 DD 0.0000000
   CF13 DD 0.0000000
   CF14 DD 0.5000000
   CF15 DD 0.5000000
   CS16 DB `0x%p`, 10, 0 
segment .text
   global main
   extern ExitProcess
   extern printf
   extern sprintf
   extern malloc
   extern calloc
   extern free
   extern memset
   extern memcpy
   extern InitWindow
   extern CloseWindow
   extern WindowShouldClose
   extern BeginDrawing
   extern EndDrawing
   extern ClearBackground
   extern DrawFPS
   extern DrawText
   extern DrawCircle
   extern DrawRectangle
   extern CheckCollisionRecs
   extern MeasureText
   extern SetTargetFPS
   extern GetFrameTime
   extern GetTime
   extern IsKeyPressed
   extern IsKeyDown
   extern GetKeyPressed
   extern round


assert:
   cmp		cl, 0
   jz 		assert_fail
   ret
assert_fail:
   mov		rcx, string_assert_fail
   call		printf
   mov		rcx, 1
   call		ExitProcess

get_enum_string_KeyboardKey:
   mov		r8, 0
   cmp		rdx, r8
   jz			enum_case_0
   mov		r8, 39
   cmp		rdx, r8
   jz			enum_case_1
   mov		r8, 44
   cmp		rdx, r8
   jz			enum_case_2
   mov		r8, 45
   cmp		rdx, r8
   jz			enum_case_3
   mov		r8, 46
   cmp		rdx, r8
   jz			enum_case_4
   mov		r8, 47
   cmp		rdx, r8
   jz			enum_case_5
   mov		r8, 48
   cmp		rdx, r8
   jz			enum_case_6
   mov		r8, 49
   cmp		rdx, r8
   jz			enum_case_7
   mov		r8, 50
   cmp		rdx, r8
   jz			enum_case_8
   mov		r8, 51
   cmp		rdx, r8
   jz			enum_case_9
   mov		r8, 52
   cmp		rdx, r8
   jz			enum_case_10
   mov		r8, 53
   cmp		rdx, r8
   jz			enum_case_11
   mov		r8, 54
   cmp		rdx, r8
   jz			enum_case_12
   mov		r8, 55
   cmp		rdx, r8
   jz			enum_case_13
   mov		r8, 56
   cmp		rdx, r8
   jz			enum_case_14
   mov		r8, 57
   cmp		rdx, r8
   jz			enum_case_15
   mov		r8, 59
   cmp		rdx, r8
   jz			enum_case_16
   mov		r8, 61
   cmp		rdx, r8
   jz			enum_case_17
   mov		r8, 65
   cmp		rdx, r8
   jz			enum_case_18
   mov		r8, 66
   cmp		rdx, r8
   jz			enum_case_19
   mov		r8, 67
   cmp		rdx, r8
   jz			enum_case_20
   mov		r8, 68
   cmp		rdx, r8
   jz			enum_case_21
   mov		r8, 69
   cmp		rdx, r8
   jz			enum_case_22
   mov		r8, 70
   cmp		rdx, r8
   jz			enum_case_23
   mov		r8, 71
   cmp		rdx, r8
   jz			enum_case_24
   mov		r8, 72
   cmp		rdx, r8
   jz			enum_case_25
   mov		r8, 73
   cmp		rdx, r8
   jz			enum_case_26
   mov		r8, 74
   cmp		rdx, r8
   jz			enum_case_27
   mov		r8, 75
   cmp		rdx, r8
   jz			enum_case_28
   mov		r8, 76
   cmp		rdx, r8
   jz			enum_case_29
   mov		r8, 77
   cmp		rdx, r8
   jz			enum_case_30
   mov		r8, 78
   cmp		rdx, r8
   jz			enum_case_31
   mov		r8, 79
   cmp		rdx, r8
   jz			enum_case_32
   mov		r8, 80
   cmp		rdx, r8
   jz			enum_case_33
   mov		r8, 81
   cmp		rdx, r8
   jz			enum_case_34
   mov		r8, 82
   cmp		rdx, r8
   jz			enum_case_35
   mov		r8, 83
   cmp		rdx, r8
   jz			enum_case_36
   mov		r8, 84
   cmp		rdx, r8
   jz			enum_case_37
   mov		r8, 85
   cmp		rdx, r8
   jz			enum_case_38
   mov		r8, 86
   cmp		rdx, r8
   jz			enum_case_39
   mov		r8, 87
   cmp		rdx, r8
   jz			enum_case_40
   mov		r8, 88
   cmp		rdx, r8
   jz			enum_case_41
   mov		r8, 89
   cmp		rdx, r8
   jz			enum_case_42
   mov		r8, 90
   cmp		rdx, r8
   jz			enum_case_43
   mov		r8, 91
   cmp		rdx, r8
   jz			enum_case_44
   mov		r8, 92
   cmp		rdx, r8
   jz			enum_case_45
   mov		r8, 93
   cmp		rdx, r8
   jz			enum_case_46
   mov		r8, 96
   cmp		rdx, r8
   jz			enum_case_47
   mov		r8, 32
   cmp		rdx, r8
   jz			enum_case_48
   mov		r8, 256
   cmp		rdx, r8
   jz			enum_case_49
   mov		r8, 257
   cmp		rdx, r8
   jz			enum_case_50
   mov		r8, 258
   cmp		rdx, r8
   jz			enum_case_51
   mov		r8, 259
   cmp		rdx, r8
   jz			enum_case_52
   mov		r8, 260
   cmp		rdx, r8
   jz			enum_case_53
   mov		r8, 261
   cmp		rdx, r8
   jz			enum_case_54
   mov		r8, 262
   cmp		rdx, r8
   jz			enum_case_55
   mov		r8, 263
   cmp		rdx, r8
   jz			enum_case_56
   mov		r8, 264
   cmp		rdx, r8
   jz			enum_case_57
   mov		r8, 265
   cmp		rdx, r8
   jz			enum_case_58
   mov		r8, 266
   cmp		rdx, r8
   jz			enum_case_59
   mov		r8, 267
   cmp		rdx, r8
   jz			enum_case_60
   mov		r8, 268
   cmp		rdx, r8
   jz			enum_case_61
   mov		r8, 269
   cmp		rdx, r8
   jz			enum_case_62
   mov		r8, 280
   cmp		rdx, r8
   jz			enum_case_63
   mov		r8, 281
   cmp		rdx, r8
   jz			enum_case_64
   mov		r8, 282
   cmp		rdx, r8
   jz			enum_case_65
   mov		r8, 283
   cmp		rdx, r8
   jz			enum_case_66
   mov		r8, 284
   cmp		rdx, r8
   jz			enum_case_67
   mov		r8, 290
   cmp		rdx, r8
   jz			enum_case_68
   mov		r8, 291
   cmp		rdx, r8
   jz			enum_case_69
   mov		r8, 292
   cmp		rdx, r8
   jz			enum_case_70
   mov		r8, 293
   cmp		rdx, r8
   jz			enum_case_71
   mov		r8, 294
   cmp		rdx, r8
   jz			enum_case_72
   mov		r8, 295
   cmp		rdx, r8
   jz			enum_case_73
   mov		r8, 296
   cmp		rdx, r8
   jz			enum_case_74
   mov		r8, 297
   cmp		rdx, r8
   jz			enum_case_75
   mov		r8, 298
   cmp		rdx, r8
   jz			enum_case_76
   mov		r8, 299
   cmp		rdx, r8
   jz			enum_case_77
   mov		r8, 300
   cmp		rdx, r8
   jz			enum_case_78
   mov		r8, 301
   cmp		rdx, r8
   jz			enum_case_79
   mov		r8, 340
   cmp		rdx, r8
   jz			enum_case_80
   mov		r8, 341
   cmp		rdx, r8
   jz			enum_case_81
   mov		r8, 342
   cmp		rdx, r8
   jz			enum_case_82
   mov		r8, 343
   cmp		rdx, r8
   jz			enum_case_83
   mov		r8, 344
   cmp		rdx, r8
   jz			enum_case_84
   mov		r8, 345
   cmp		rdx, r8
   jz			enum_case_85
   mov		r8, 346
   cmp		rdx, r8
   jz			enum_case_86
   mov		r8, 347
   cmp		rdx, r8
   jz			enum_case_87
   mov		r8, 348
   cmp		rdx, r8
   jz			enum_case_88
   mov		r8, 320
   cmp		rdx, r8
   jz			enum_case_89
   mov		r8, 321
   cmp		rdx, r8
   jz			enum_case_90
   mov		r8, 322
   cmp		rdx, r8
   jz			enum_case_91
   mov		r8, 323
   cmp		rdx, r8
   jz			enum_case_92
   mov		r8, 324
   cmp		rdx, r8
   jz			enum_case_93
   mov		r8, 325
   cmp		rdx, r8
   jz			enum_case_94
   mov		r8, 326
   cmp		rdx, r8
   jz			enum_case_95
   mov		r8, 327
   cmp		rdx, r8
   jz			enum_case_96
   mov		r8, 328
   cmp		rdx, r8
   jz			enum_case_97
   mov		r8, 329
   cmp		rdx, r8
   jz			enum_case_98
   mov		r8, 330
   cmp		rdx, r8
   jz			enum_case_99
   mov		r8, 331
   cmp		rdx, r8
   jz			enum_case_100
   mov		r8, 332
   cmp		rdx, r8
   jz			enum_case_101
   mov		r8, 333
   cmp		rdx, r8
   jz			enum_case_102
   mov		r8, 334
   cmp		rdx, r8
   jz			enum_case_103
   mov		r8, 335
   cmp		rdx, r8
   jz			enum_case_104
   mov		r8, 336
   cmp		rdx, r8
   jz			enum_case_105
   push		rcx
   mov		r8, rdx
   mov		rdx, fmt_int
   push		r8
   push		rdx
   push		rcx
   call		sprintf
   pop		rax
   pop		rbx
   pop		rbx
   pop		rbx
   ret
enum_case_0:
   mov		rax, __KeyboardKey.NULL
   ret
enum_case_1:
   mov		rax, __KeyboardKey.APOSTROPHE
   ret
enum_case_2:
   mov		rax, __KeyboardKey.COMMA
   ret
enum_case_3:
   mov		rax, __KeyboardKey.MINUS
   ret
enum_case_4:
   mov		rax, __KeyboardKey.PERIOD
   ret
enum_case_5:
   mov		rax, __KeyboardKey.SLASH
   ret
enum_case_6:
   mov		rax, __KeyboardKey.ZERO
   ret
enum_case_7:
   mov		rax, __KeyboardKey.ONE
   ret
enum_case_8:
   mov		rax, __KeyboardKey.TWO
   ret
enum_case_9:
   mov		rax, __KeyboardKey.THREE
   ret
enum_case_10:
   mov		rax, __KeyboardKey.FOUR
   ret
enum_case_11:
   mov		rax, __KeyboardKey.FIVE
   ret
enum_case_12:
   mov		rax, __KeyboardKey.SIX
   ret
enum_case_13:
   mov		rax, __KeyboardKey.SEVEN
   ret
enum_case_14:
   mov		rax, __KeyboardKey.EIGHT
   ret
enum_case_15:
   mov		rax, __KeyboardKey.NINE
   ret
enum_case_16:
   mov		rax, __KeyboardKey.SEMICOLON
   ret
enum_case_17:
   mov		rax, __KeyboardKey.EQUAL
   ret
enum_case_18:
   mov		rax, __KeyboardKey.A
   ret
enum_case_19:
   mov		rax, __KeyboardKey.B
   ret
enum_case_20:
   mov		rax, __KeyboardKey.C
   ret
enum_case_21:
   mov		rax, __KeyboardKey.D
   ret
enum_case_22:
   mov		rax, __KeyboardKey.E
   ret
enum_case_23:
   mov		rax, __KeyboardKey.F
   ret
enum_case_24:
   mov		rax, __KeyboardKey.G
   ret
enum_case_25:
   mov		rax, __KeyboardKey.H
   ret
enum_case_26:
   mov		rax, __KeyboardKey.I
   ret
enum_case_27:
   mov		rax, __KeyboardKey.J
   ret
enum_case_28:
   mov		rax, __KeyboardKey.K
   ret
enum_case_29:
   mov		rax, __KeyboardKey.L
   ret
enum_case_30:
   mov		rax, __KeyboardKey.M
   ret
enum_case_31:
   mov		rax, __KeyboardKey.N
   ret
enum_case_32:
   mov		rax, __KeyboardKey.O
   ret
enum_case_33:
   mov		rax, __KeyboardKey.P
   ret
enum_case_34:
   mov		rax, __KeyboardKey.Q
   ret
enum_case_35:
   mov		rax, __KeyboardKey.R
   ret
enum_case_36:
   mov		rax, __KeyboardKey.S
   ret
enum_case_37:
   mov		rax, __KeyboardKey.T
   ret
enum_case_38:
   mov		rax, __KeyboardKey.U
   ret
enum_case_39:
   mov		rax, __KeyboardKey.V
   ret
enum_case_40:
   mov		rax, __KeyboardKey.W
   ret
enum_case_41:
   mov		rax, __KeyboardKey.X
   ret
enum_case_42:
   mov		rax, __KeyboardKey.Y
   ret
enum_case_43:
   mov		rax, __KeyboardKey.Z
   ret
enum_case_44:
   mov		rax, __KeyboardKey.LEFT_BRACKET
   ret
enum_case_45:
   mov		rax, __KeyboardKey.BACKSLASH
   ret
enum_case_46:
   mov		rax, __KeyboardKey.RIGHT_BRACKET
   ret
enum_case_47:
   mov		rax, __KeyboardKey.GRAVE
   ret
enum_case_48:
   mov		rax, __KeyboardKey.SPACE
   ret
enum_case_49:
   mov		rax, __KeyboardKey.ESCAPE
   ret
enum_case_50:
   mov		rax, __KeyboardKey.ENTER
   ret
enum_case_51:
   mov		rax, __KeyboardKey.TAB
   ret
enum_case_52:
   mov		rax, __KeyboardKey.BACKSPACE
   ret
enum_case_53:
   mov		rax, __KeyboardKey.INSERT
   ret
enum_case_54:
   mov		rax, __KeyboardKey.DELETE
   ret
enum_case_55:
   mov		rax, __KeyboardKey.RIGHT
   ret
enum_case_56:
   mov		rax, __KeyboardKey.LEFT
   ret
enum_case_57:
   mov		rax, __KeyboardKey.DOWN
   ret
enum_case_58:
   mov		rax, __KeyboardKey.UP
   ret
enum_case_59:
   mov		rax, __KeyboardKey.PAGE_UP
   ret
enum_case_60:
   mov		rax, __KeyboardKey.PAGE_DOWN
   ret
enum_case_61:
   mov		rax, __KeyboardKey.HOME
   ret
enum_case_62:
   mov		rax, __KeyboardKey.END
   ret
enum_case_63:
   mov		rax, __KeyboardKey.CAPS_LOCK
   ret
enum_case_64:
   mov		rax, __KeyboardKey.SCROLL_LOCK
   ret
enum_case_65:
   mov		rax, __KeyboardKey.NUM_LOCK
   ret
enum_case_66:
   mov		rax, __KeyboardKey.PRINT_SCREEN
   ret
enum_case_67:
   mov		rax, __KeyboardKey.PAUSE
   ret
enum_case_68:
   mov		rax, __KeyboardKey.F1
   ret
enum_case_69:
   mov		rax, __KeyboardKey.F2
   ret
enum_case_70:
   mov		rax, __KeyboardKey.F3
   ret
enum_case_71:
   mov		rax, __KeyboardKey.F4
   ret
enum_case_72:
   mov		rax, __KeyboardKey.F5
   ret
enum_case_73:
   mov		rax, __KeyboardKey.F6
   ret
enum_case_74:
   mov		rax, __KeyboardKey.F7
   ret
enum_case_75:
   mov		rax, __KeyboardKey.F8
   ret
enum_case_76:
   mov		rax, __KeyboardKey.F9
   ret
enum_case_77:
   mov		rax, __KeyboardKey.F10
   ret
enum_case_78:
   mov		rax, __KeyboardKey.F11
   ret
enum_case_79:
   mov		rax, __KeyboardKey.F12
   ret
enum_case_80:
   mov		rax, __KeyboardKey.LEFT_SHIFT
   ret
enum_case_81:
   mov		rax, __KeyboardKey.LEFT_CONTROL
   ret
enum_case_82:
   mov		rax, __KeyboardKey.LEFT_ALT
   ret
enum_case_83:
   mov		rax, __KeyboardKey.LEFT_SUPER
   ret
enum_case_84:
   mov		rax, __KeyboardKey.RIGHT_SHIFT
   ret
enum_case_85:
   mov		rax, __KeyboardKey.RIGHT_CONTROL
   ret
enum_case_86:
   mov		rax, __KeyboardKey.RIGHT_ALT
   ret
enum_case_87:
   mov		rax, __KeyboardKey.RIGHT_SUPER
   ret
enum_case_88:
   mov		rax, __KeyboardKey.KB_MENU
   ret
enum_case_89:
   mov		rax, __KeyboardKey.KP_0
   ret
enum_case_90:
   mov		rax, __KeyboardKey.KP_1
   ret
enum_case_91:
   mov		rax, __KeyboardKey.KP_2
   ret
enum_case_92:
   mov		rax, __KeyboardKey.KP_3
   ret
enum_case_93:
   mov		rax, __KeyboardKey.KP_4
   ret
enum_case_94:
   mov		rax, __KeyboardKey.KP_5
   ret
enum_case_95:
   mov		rax, __KeyboardKey.KP_6
   ret
enum_case_96:
   mov		rax, __KeyboardKey.KP_7
   ret
enum_case_97:
   mov		rax, __KeyboardKey.KP_8
   ret
enum_case_98:
   mov		rax, __KeyboardKey.KP_9
   ret
enum_case_99:
   mov		rax, __KeyboardKey.KP_DECIMAL
   ret
enum_case_100:
   mov		rax, __KeyboardKey.KP_DIVIDE
   ret
enum_case_101:
   mov		rax, __KeyboardKey.KP_MULTIPLY
   ret
enum_case_102:
   mov		rax, __KeyboardKey.KP_SUBTRACT
   ret
enum_case_103:
   mov		rax, __KeyboardKey.KP_ADD
   ret
enum_case_104:
   mov		rax, __KeyboardKey.KP_ENTER
   ret
enum_case_105:
   mov		rax, __KeyboardKey.KP_EQUAL
   ret

; bytes locals   : 32
; bytes temp     : 16
; bytes total    : 80
get_ball_rect:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   mov		-8[rbp], rcx	; Return 0
   mov		24[rbp], rdx 	; Save arg 0 
   ; Copy ball -> -32
   mov		rax, 24[rbp]
   lea		rbx, -32[rbp]
   mov		r8, 20
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   lea		rcx, -48[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		eax, -32[rbp]
   push		rax
   pop		rax
   mov		-48[rbp], eax
   mov		eax, -28[rbp]
   push		rax
   pop		rax
   mov		-44[rbp], eax
   mov		eax, -24[rbp]
   push		rax
   pop		rax
   mov		-40[rbp], eax
   mov		eax, -24[rbp]
   push		rax
   pop		rax
   mov		-36[rbp], eax
   lea		rax, -48[rbp]
   push		rax
   jmp		L107
L107:
   pop		rax
   mov		rbx, -8[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   mov		rax, -8[rbp]
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 32
; bytes temp     : 16
; bytes total    : 80
get_bar_rect:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 80
   mov		-8[rbp], rcx	; Return 0
   mov		24[rbp], rdx 	; Save arg 0 
   ; Copy bar -> -32
   mov		rax, 24[rbp]
   lea		rbx, -32[rbp]
   mov		r8, 24
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   lea		rcx, -48[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		eax, -32[rbp]
   push		rax
   pop		rax
   mov		-48[rbp], eax
   mov		eax, -28[rbp]
   push		rax
   pop		rax
   mov		-44[rbp], eax
   mov		eax, -24[rbp]
   push		rax
   pop		rax
   mov		-40[rbp], eax
   mov		eax, -20[rbp]
   push		rax
   pop		rax
   mov		-36[rbp], eax
   lea		rax, -48[rbp]
   push		rax
   jmp		L108
L108:
   pop		rax
   mov		rbx, -8[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   mov		rax, -8[rbp]
   add		rsp, 80
   pop		rbp
   ret

; bytes locals   : 1744
; bytes temp     : 40
; bytes total    : 1824
main:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 1824

   ; Ln 244: $COLOR_WHITE : Color = -4
   lea		rcx, -4[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -4[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -3[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -2[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -1[rbp], al

   ; Ln 246: $window_width : int = -8
   mov		rax, 1200
   push		rax
   pop		rax
   mov		DWORD -8[rbp], eax

   ; Ln 247: $window_height : int = -12
   mov		rax, 800
   push		rax
   pop		rax
   mov		DWORD -12[rbp], eax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, CS0
   push		rax
   pop		rax
   mov		r8 , rax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		ecx, eax
   call		InitWindow
   mov		rax, 90
   push		rax
   pop		rax
   mov		ecx, eax
   call		SetTargetFPS

   ; Ln 253: $text : string = -24
   mov		rax, CS1
   push		rax
   pop		rax
   mov		QWORD -24[rbp], rax

   ; Ln 254: $text_len : int = -28
   mov		rax, QWORD -24[rbp]
   push		rax
   mov		rax, 24
   push		rax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		rcx, rax
   call		MeasureText
   push		rax
   pop		rax
   mov		DWORD -28[rbp], eax

   ; Ln 256: $ball : Ball = -48
   lea		rcx, -48[rbp]
   mov		rdx, 0
   mov		r8, 20
   call		memset
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF2]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-48[rbp], eax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF3]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-44[rbp], eax
   push		20
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-40[rbp], xmm0
   movss		xmm0, [CF4]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-36[rbp], eax
   movss		xmm0, [CF5]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-32[rbp], eax

   ; Ln 264: $bar : Bar = -72
   lea		rcx, -72[rbp]
   mov		rdx, 0
   mov		r8, 24
   call		memset
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF6]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   push		200
   movss		xmm0, [CF7]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-72[rbp], eax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF8]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-68[rbp], eax
   push		200
   pop		rax
   cvtsi2ss	xmm0, rax
   movd		-64[rbp], xmm0
   movss		xmm0, [CF9]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-60[rbp], eax
   movss		xmm0, [CF10]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   movss		xmm0, [CF11]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-52[rbp], eax

   ; Ln 273: $bricks : [88]Brick = -1496
   lea		rax, -1480[rbp]
   mov		QWORD -1496[rbp], rax
   mov		QWORD -1488[rbp], 88
   mov		rcx, rax
   mov		rdx, 0
   mov		r8, 1408
   call		memset

   ; Ln 276: $brick_row_width : int = -1500
   push		80
   push		15
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   push		11
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   mov		rax, 1
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -1500[rbp], eax
   ; Ln 277: Ranged for-loop
   mov		rax, 0
   push		rax
   push		8
   pop		rbx
   pop		rax
   mov		QWORD -1520[rbp], rax
   mov		QWORD -1528[rbp], rbx
   mov		eax, DWORD -1520[rbp]
   mov		-1512[rbp], eax
L110:
   mov		eax, -1528[rbp]
   cmp		-1512[rbp], eax
   jge		L112
   ; Ln 278: Ranged for-loop
   mov		rax, 0
   push		rax
   push		11
   pop		rbx
   pop		rax
   mov		QWORD -1544[rbp], rax
   mov		QWORD -1552[rbp], rbx
   mov		eax, DWORD -1544[rbp]
   mov		-1536[rbp], eax
L113:
   mov		eax, -1552[rbp]
   cmp		-1536[rbp], eax
   jge		L115

   ; Ln 279: $index : s32 = -1556
   mov		eax, DWORD -1512[rbp]
   movsx		rax, eax
   push		rax
   push		11
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   mov		eax, DWORD -1536[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -1556[rbp], eax

   ; Ln 281: $x : int = -1560
   push		80
   push		15
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   mov		eax, DWORD -1536[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   push		rax
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   mov		eax, DWORD -1500[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 2
   push		rax
   pop		rbx
   pop		rax
   cqo
   idiv		rbx
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -1560[rbp], eax

   ; Ln 282: $y : int = -1564
   push		20
   push		15
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   mov		eax, DWORD -1512[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   pop		rax
   imul		rax, rbx
   push		rax
   push		80
   pop		rbx
   pop		rax
   add		rax, rbx
   push		rax
   pop		rax
   mov		DWORD -1564[rbp], eax

   ; Ln 284: $color : Color = -1568
   lea		rcx, -1568[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 33
   push		rax
   pop		rax
   mov		BYTE -1568[rbp], al
   mov		rax, 186
   push		rax
   pop		rax
   mov		BYTE -1567[rbp], al
   mov		rax, 10
   push		rax
   pop		rax
   mov		BYTE -1566[rbp], al
   mov		rax, 255
   push		rax
   pop		rax
   mov		BYTE -1565[rbp], al

   ; Ln 286: $brick : Brick = -1584
   lea		rcx, -1584[rbp]
   mov		rdx, 0
   mov		r8, 16
   call		memset
   mov		eax, DWORD -1560[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -1584[rbp], eax
   mov		eax, DWORD -1564[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   mov		DWORD -1580[rbp], eax
   mov		eax, -1568[rbp]
   push		rax
   pop		rax
   ; Copy struct
   mov		-1576[rbp], eax
   push		0
   pop		rax
   mov		BYTE -1572[rbp], al
   ; Ln 293: Assignment
   lea		rax, -1584[rbp]
   push		rax
   mov		eax, DWORD -1556[rbp]
   movsx		rax, eax
   push		rax
   pop		rax
   imul		rax, 16
   push		rax
   pop		rax
   mov		rbx, QWORD -1496[rbp]   ; load pointer to .data
   add		rbx, rax              ; add element offset
   pop		rax
   ; Copy struct
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
L114:
   inc		DWORD -1536[rbp]
   jmp		L113
L115:
L111:
   inc		DWORD -1512[rbp]
   jmp		L110
L112:
L116:
   call		WindowShouldClose
   push		rax
   pop		rax
   test		rax, rax
   sete		al
   movzx		rax, al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L117
   ; While body

   ; Ln 299: $dt : f32 = -1588
   call		GetFrameTime
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-1588[rbp], eax
   ; Ln 302: Assignment
   movss		xmm0, [CF12]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   push		65
   pop		rax
   mov		ecx, eax
   call		IsKeyDown
   push		rax
   push		263
   pop		rax
   mov		ecx, eax
   call		IsKeyDown
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L118
   ; block of if
   ; Ln 305: Assignment
   mov		eax, -56[rbp]
   push		rax
   mov		eax, -52[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   jmp L118
; done
L118:
   push		68
   pop		rax
   mov		ecx, eax
   call		IsKeyDown
   push		rax
   push		262
   pop		rax
   mov		ecx, eax
   call		IsKeyDown
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L119
   ; block of if
   ; Ln 308: Assignment
   mov		eax, -56[rbp]
   push		rax
   mov		eax, -52[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   jmp L119
; done
L119:

   ; Ln 312: $next_ball : Ball = -1608
   lea		rax, -48[rbp]
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -1608[rbp]
   mov		r8, 20
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   ; Ln 313: Assignment
   mov		eax, -48[rbp]
   push		rax
   mov		eax, -36[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-1608[rbp], eax
   ; Ln 314: Assignment
   mov		eax, -44[rbp]
   push		rax
   mov		eax, -32[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-1604[rbp], eax

   ; Ln 316: $next_bar : Bar = -1632
   lea		rax, -72[rbp]
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -1632[rbp]
   mov		r8, 24
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   ; Ln 317: Assignment
   mov		eax, -72[rbp]
   push		rax
   mov		eax, -56[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-1632[rbp], eax
   mov		eax, -1608[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   setb		al
   push		rax
   mov		eax, -1608[rbp]
   push		rax
   mov		eax, -1600[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   seta		al
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L120
   ; block of if
   ; Ln 321: Assignment
   mov		eax, -36[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   xorpd		xmm1, xmm1
   subss		xmm1, xmm0
   movd		eax, xmm1
   push		rax
   pop		rax
   mov		-36[rbp], eax
   jmp L120
; done
L120:
   mov		eax, -1604[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   setb		al
   push		rax
   mov		eax, -1604[rbp]
   push		rax
   mov		eax, -1600[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, DWORD -12[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   seta		al
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L121
   ; block of if
   ; Ln 325: Assignment
   mov		eax, -32[rbp]
   push		rax
   pop		rax
   movd		xmm0, eax
   xorpd		xmm1, xmm1
   subss		xmm1, xmm0
   movd		eax, xmm1
   push		rax
   pop		rax
   mov		-32[rbp], eax
   jmp L121
; done
L121:

   ; Ln 329: $next_ball_rect : Rectangle = -1648
   lea		rax, -1608[rbp]
   push		rax
   pop		rax
   mov		rdx, rax
   lea		rcx, -1760[rbp]		; Return value 0
   call		get_ball_rect
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -1648[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy

   ; Ln 330: $next_bar_rect : Rectangle = -1664
   lea		rax, -1632[rbp]
   push		rax
   pop		rax
   mov		rdx, rax
   lea		rcx, -1760[rbp]		; Return value 0
   call		get_bar_rect
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -1664[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy
   lea		rax, -1648[rbp]
   push		rax
   lea		rax, -1664[rbp]
   push		rax
   pop		rax
   mov		rdx, rax
   pop		rax
   mov		rcx, rax
   call		CheckCollisionRecs
   push		rax
   pop		rax
   cmp		al, 0
   jz			L122
   ; block of if
   lea		rax, -48[rbp]
   push		rax
   lea		rax, -1664[rbp]
   push		rax
   pop		rax
   mov		rdx, rax
   pop		rax
   mov		rcx, rax
   call		bounce_ball
   jmp L122
; done
L122:
   mov		eax, -1632[rbp]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   setb		al
   push		rax
   mov		eax, -1632[rbp]
   push		rax
   mov		eax, -64[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   seta		al
   push		rax

   pop		rbx
   pop		rax
   or 		al, bl
   push		rax
   pop		rax
   cmp		al, 0
   jz			L123
   ; block of if
   ; Ln 356: Assignment
   movss		xmm0, [CF13]
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-56[rbp], eax
   jmp L123
; done
L123:
   ; Ln 360: Assignment
   mov		eax, -48[rbp]
   push		rax
   mov		eax, -36[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-48[rbp], eax
   ; Ln 361: Assignment
   mov		eax, -44[rbp]
   push		rax
   mov		eax, -32[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-44[rbp], eax
   ; Ln 362: Assignment
   mov		eax, -72[rbp]
   push		rax
   mov		eax, -56[rbp]
   push		rax
   mov		eax, -1588[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-72[rbp], eax
   call		BeginDrawing
   lea		rcx, -1748[rbp]
   mov		rdx, 0
   mov		r8, 4
   call		memset
   mov		rax, 30
   push		rax
   pop		rax
   mov		BYTE -1748[rbp], al
   mov		rax, 30
   push		rax
   pop		rax
   mov		BYTE -1747[rbp], al
   mov		rax, 30
   push		rax
   pop		rax
   mov		BYTE -1746[rbp], al
   mov		rax, 30
   push		rax
   pop		rax
   mov		BYTE -1745[rbp], al
   mov		eax, -1748[rbp]
   push		rax
   pop		rax
   mov		ecx, eax
   call		ClearBackground
   ; Ln 372: For-loop
   lea		rax, -1496[rbp]
   push		rax
   pop		rax
   mov		rbx, 0[rax]
   mov		rcx, 8[rax]
   mov		-1688[rbp], rbx     ; data
   mov		-1696[rbp], rcx     ; count
   mov		QWORD -1704[rbp], 0 ; index
L124:
   mov		rbx, -1696[rbp]
   mov		rax, -1704[rbp]
   cmp		rax, rbx
   jge		L126
   mov		rbx, -1688[rbp]
   imul		rax, 16
   add		rbx, rax
   mov		rax, 0[rbx]
   mov		-1680[rbp], rax
   mov		rax, 8[rbx]
   mov		-1672[rbp], rax
   mov		al, BYTE -1668[rbp]
   push		rax
   pop		rax
   cmp		al, 0
   jz			L127
   ; block of if
   jmp		L125
   jmp L127
; done
L127:
   mov		eax, DWORD -1680[rbp]
   movsx		rax, eax
   push		rax
   mov		eax, DWORD -1676[rbp]
   movsx		rax, eax
   push		rax
   push		80
   push		20
   mov		eax, -1672[rbp]
   push		rax
   pop		rax
   mov		-1752[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		ecx, eax
   mov		rax, QWORD -1752[rbp]
   mov		32[rsp], eax
   call		DrawRectangle
L125:
   inc		QWORD -1704[rbp]
   jmp		L124
L126:
   mov		eax, -48[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -44[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -40[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -40[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -4[rbp]
   push		rax
   pop		rax
   mov		-1752[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		ecx, eax
   mov		rax, QWORD -1752[rbp]
   mov		32[rsp], eax
   call		DrawRectangle
   mov		eax, -72[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -68[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -64[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -60[rbp]
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		eax, -4[rbp]
   push		rax
   pop		rax
   mov		-1752[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		ecx, eax
   mov		rax, QWORD -1752[rbp]
   mov		32[rsp], eax
   call		DrawRectangle
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   mov		rax, 100
   push		rax
   pop		rbx
   pop		rax
   sub		rax, rbx
   push		rax
   mov		rax, 16
   push		rax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		ecx, eax
   call		DrawFPS
   mov		rax, QWORD -24[rbp]
   push		rax
   mov		eax, DWORD -8[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF14]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, DWORD -28[rbp]
   movsx		rax, eax
   push		rax
   movss		xmm0, [CF15]
   movd		eax, xmm0
   push		rax
   pop		rbx
   movd		xmm1, ebx
   pop		rax
   cvtsi2ss	xmm0, eax
   mulss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   movq		xmm0, rax
   cvttss2si		eax, xmm0
   push		rax
   mov		rax, 24
   push		rax
   mov		rax, 24
   push		rax
   mov		eax, -4[rbp]
   push		rax
   pop		rax
   mov		-1752[rbp], rax
   pop		rax
   mov		r9d, eax
   pop		rax
   mov		r8d, eax
   pop		rax
   mov		edx, eax
   pop		rax
   mov		rcx, rax
   mov		rax, QWORD -1752[rbp]
   mov		32[rsp], eax
   call		DrawText
   call		EndDrawing
   jmp			L116
L117:
   call		CloseWindow
L109:
   mov		rax, 0
   add		rsp, 1824
   pop		rbp
   ret

; bytes locals   : 48
; bytes temp     : 40
; bytes total    : 128
bounce_ball:
   push		rbp
   mov		rbp, rsp
   sub		rsp, 128
   mov		16[rbp], rcx 	; Save arg 0 
   mov		24[rbp], rdx 	; Save arg 1 
   ; Copy ball -> -8
   mov		rax, 16[rbp]
   mov		-8[rbp], rax
   ; Copy b -> -24
   mov		rax, 24[rbp]
   lea		rbx, -24[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy

   ; Ln 397: $a : Rectangle = -40
   mov		rax, -8[rbp]
   push		rax
   pop		rbx
   lea		rax, [rbx]
   push		rax
   pop		rax
   mov		rdx, rax
   lea		rcx, -64[rbp]		; Return value 0
   call		get_ball_rect
   push		rax
   pop		rax
   ; Copy struct
   lea		rbx, -40[rbp]
   mov		r8, 16
   lea		rdx, 0[rax]
   lea		rcx, 0[rbx]
   call		memcpy

   ; Ln 399: $overlap : Vector2 = -48
   lea		rcx, -48[rbp]
   mov		rdx, 0
   mov		r8, 8
   call		memset
   mov		rbx, -8[rbp]
   add		rbx, 12
   mov		eax, [rbx]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   seta		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L130
   ; block of if
   ; Ln 403: Assignment
   mov		eax, -40[rbp]
   push		rax
   mov		eax, -32[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, -24[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-48[rbp], eax
   jmp L129
; else
L130:
   ; Ln 405: Assignment
   mov		eax, -24[rbp]
   push		rax
   mov		eax, -16[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, -40[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-48[rbp], eax
   jmp L129
; done
L129:
   mov		rbx, -8[rbp]
   add		rbx, 16
   mov		eax, [rbx]
   push		rax
   mov		rax, 0
   push		rax
   pop		rbx
   cvtsi2ss	xmm1, ebx
   pop		rax
   movd		xmm0, eax
   comiss	xmm0, xmm1
   seta		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L132
   ; block of if
   ; Ln 409: Assignment
   mov		eax, -36[rbp]
   push		rax
   mov		eax, -28[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, -20[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-44[rbp], eax
   jmp L131
; else
L132:
   ; Ln 411: Assignment
   mov		eax, -20[rbp]
   push		rax
   mov		eax, -12[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		eax, -36[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   subss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   pop		rax
   mov		-44[rbp], eax
   jmp L131
; done
L131:
   mov		eax, -48[rbp]
   push		rax
   mov		eax, -44[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   comiss	xmm0, xmm1
   setb		al
   push		rax
   pop		rax
   cmp		al, 0
   jz			L134
   ; block of if
   ; Ln 416: Assignment
   mov		rbx, -8[rbp]
   add		rbx, 12
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   xorpd		xmm1, xmm1
   subss		xmm1, xmm0
   movd		eax, xmm1
   push		rax
   mov		rbx, -8[rbp]
   add		rbx, 12
   pop		rax
   mov		[rbx], eax
   ; Ln 417: Assignment
   mov		rbx, -8[rbp]
   add		rbx, 0
   mov		eax, [rbx]
   push		rax
   mov		eax, -48[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		rbx, -8[rbp]
   add		rbx, 0
   pop		rax
   mov		[rbx], eax
   jmp L133
; else
L134:
   ; Ln 419: Assignment
   mov		rbx, -8[rbp]
   add		rbx, 16
   mov		eax, [rbx]
   push		rax
   pop		rax
   movd		xmm0, eax
   xorpd		xmm1, xmm1
   subss		xmm1, xmm0
   movd		eax, xmm1
   push		rax
   mov		rbx, -8[rbp]
   add		rbx, 16
   pop		rax
   mov		[rbx], eax
   ; Ln 420: Assignment
   mov		rbx, -8[rbp]
   add		rbx, 4
   mov		eax, [rbx]
   push		rax
   mov		eax, -44[rbp]
   push		rax
   pop		rbx
   pop		rax
   movd		xmm1, ebx
   movd		xmm0, eax
   addss		xmm0, xmm1
   movd		eax, xmm0
   push		rax
   mov		rbx, -8[rbp]
   add		rbx, 4
   pop		rax
   mov		[rbx], eax
   jmp L133
; done
L133:
   ; Ln 423 Print
   push		rax
   ; Pop print arguments
   pop		rax
   mov		rdx, rax
   mov		rcx, CS16
   call		printf
L128:
   mov		rax, 0
   add		rsp, 128
   pop		rbp
   ret
