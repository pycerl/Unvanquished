code

equ memset                                -1
equ memcpy                                -2
equ memcmp                                -3
equ strncpy                               -4
equ sin                                   -5
equ cos                                   -6
equ asin                                  -7
equ tanf                                  -8
equ atanf                                 -9
equ atan2                                 -10
equ sqrt                                  -11
equ floor                                 -12
equ ceil                                  -13
equ testPrintInt                          -225
equ testPrintFloat                        -226
equ trap_SyscallABIVersion                -256
equ trap_Error                            -257
equ trap_Print                            -258
equ trap_Milliseconds                     -259
equ trap_Cvar_Register                    -260
equ trap_Cvar_Update                      -261
equ trap_Cvar_Set                         -262
equ trap_Cvar_VariableValue               -263
equ trap_Cvar_VariableStringBuffer        -264
equ trap_Cvar_LatchedVariableStringBuffer -265
equ trap_Cvar_SetValue                    -266
equ trap_Cvar_Reset                       -267
equ trap_Cvar_Create                      -268
equ trap_Cvar_InfoStringBuffer            -269
equ trap_Argc                             -270
equ trap_Argv                             -271
equ trap_Cmd_ExecuteText                  -272
equ trap_AddCommand                       -273
equ trap_FS_FOpenFile                     -274
equ trap_FS_Read                          -275
equ trap_FS_Write                         -276
equ trap_FS_FCloseFile                    -277
equ trap_FS_Delete                        -278
equ trap_FS_GetFileList                   -279
equ trap_FS_Seek                          -280
equ trap_R_RegisterModel                  -281
equ trap_R_RegisterSkin                   -282
equ trap_R_RegisterShaderNoMip            -283
equ trap_R_ClearScene                     -284
equ trap_R_AddRefEntityToScene            -285
equ trap_R_AddPolyToScene                 -286
equ trap_R_AddPolysToScene                -287
equ trap_R_AddLightToScene                -288
equ trap_R_AddCoronaToScene               -289
equ trap_R_RenderScene                    -290
equ trap_R_SetColor                       -291
equ trap_R_SetClipRegion                  -292
equ trap_R_Add2dPolys                     -293
equ trap_R_DrawStretchPic                 -294
equ trap_R_DrawRotatedPic                 -295
equ trap_R_ModelBounds                    -296
equ trap_UpdateScreen                     -297
equ trap_CM_LerpTag                       -298
equ trap_S_RegisterSound                  -299
equ trap_S_StartLocalSound                -300
equ trap_S_FadeBackgroundTrack            -301
equ trap_S_FadeAllSound                   -302
equ trap_Key_KeynumToStringBuf            -303
equ trap_Key_GetBindingBuf                -304
equ trap_Key_SetBinding                   -305
equ trap_Key_KeysForBinding               -306
equ trap_Key_IsDown                       -307
equ trap_Key_GetOverstrikeMode            -308
equ trap_Key_SetOverstrikeMode            -309
equ trap_Key_ClearStates                  -310
equ trap_Key_GetCatcher                   -311
equ trap_Key_SetCatcher                   -312
equ trap_GetClipboardData                 -313
equ trap_GetClientState                   -314
equ trap_GetGlconfig                      -315
equ trap_GetConfigString                  -316
equ trap_LAN_LoadCachedServers            -317
equ trap_LAN_SaveCachedServers            -318
equ trap_LAN_AddServer                    -319
equ trap_LAN_RemoveServer                 -320
equ trap_LAN_GetPingQueueCount            -321
equ trap_LAN_ClearPing                    -322
equ trap_LAN_GetPing                      -323
equ trap_LAN_GetPingInfo                  -324
equ trap_LAN_GetServerCount               -325
equ trap_LAN_GetServerAddressString       -326
equ trap_LAN_GetServerInfo                -327
equ trap_LAN_GetServerPing                -328
equ trap_LAN_MarkServerVisible            -329
equ trap_LAN_ServerIsVisible              -330
equ trap_LAN_UpdateVisiblePings           -331
equ trap_LAN_ResetPings                   -332
equ trap_LAN_ServerStatus                 -333
equ trap_LAN_ServerIsInFavoriteList       -334
equ trap_GetNews                          -335
equ trap_LAN_CompareServers               -336
equ trap_MemoryRemaining                  -337
equ trap_R_RegisterFont                   -338
equ trap_Parse_AddGlobalDefine            -339
equ trap_PC_AddGlobalDefine               -339
equ trap_Parse_LoadSource                 -340
equ trap_PC_LoadSource                    -340
equ trap_Parse_FreeSource                 -341
equ trap_PC_FreeSource                    -341
equ trap_Parse_ReadToken                  -342
equ trap_PC_ReadToken                     -342
equ trap_Parse_SourceFileAndLine          -343
equ trap_PC_SourceFileAndLine             -343
equ trap_PC_RemoveAllGlobalDefines        -345
equ trap_PC_UnReadToken                   -350
equ trap_S_StopBackgroundTrack            -351
equ trap_S_StartBackgroundTrack           -352
equ trap_RealTime                         -353
equ trap_CIN_PlayCinematic                -354
equ trap_CIN_StopCinematic                -355
equ trap_CIN_RunCinematic                 -356
equ trap_CIN_DrawCinematic                -357
equ trap_CIN_SetExtents                   -358
equ trap_R_RemapShader                    -359
equ trap_GetLimboString                   -360
equ trap_openURL                          -361
equ trap_GetHunkData                      -362
equ trap_QuoteString                      -363
equ trap_R_RegisterAnimation              -364
equ trap_R_BuildSkeleton                  -365
equ trap_R_BlendSkeleton                  -366
equ trap_R_BoneIndex                      -367
equ trap_R_AnimNumFrames                  -368
equ trap_R_AnimFrameRate                  -369
equ trap_Gettext                          -370
equ trap_R_Glyph                          -371
equ trap_R_GlyphChar                      -372
equ trap_R_UnregisterFont                 -373
equ trap_Pgettext                         -374
