:::::::::::::::::::::::::::::
:: tacDeleteExtraFiles.bat ::
:::::::::::::::::::::::::::::
cls


:: Remove the folder used to zip the game
rmdir Tac /S /Q


:: Remove \Debug folders
rmdir TacGraphics\Debug /S /Q
rmdir TacLibrary\Debug /S /Q
rmdir TacGame\Debug /S /Q
rmdir TacMain\Debug /S /Q
rmdir TacModelConverter\Debug /S /Q
rmdir TacMainWin32\ModelConverter\Debug /S /Q
rmdir TacMainWin32\Game\Debug /S /Q


:: Delete libs
del tacLibrary.lib
del tacGraphics.lib
del tacGame.lib


:: Delete dlls
del tacGame.dll
del tacGameOpen.dll
del tacModelConverter.dll
del tacModelConverterOpen.dll


:: Delete .exe's
del Game.exe
del tacSDLMain.exe


:: Delete all other temp files we don't need
del tacStatus.txt
del AssertLog.txt
del tacErrors.txt
del Tac.zip
del tac.sdf
del *.ilk
del *.pdb
del *.exp