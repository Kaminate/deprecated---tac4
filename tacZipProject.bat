:: tacZipProject.bat ::
:::::::::::::::::::::::



mkdir Tac
xcopy /Y ModelConverter.exe Tac
xcopy /Y assimp.dll Tac
xcopy /Y FreeImage.dll Tac
xcopy /Y SDL2.dll Tac
xcopy /Y tacModelConverter.dll Tac
xcopy /Y D3DCOMPILER_46.dll Tac
xcopy /Y ModelConverter.pdb Tac
xcopy /Y tacModelConverter.pdb Tac
:: xcopy /Y tacGame.dll Tac

:: /E copies everything
:: /I assumes destination is a directory
xcopy /Y TacData Tac\tacData /E /I

:: -r recursive
zip -r Tac.zip Tac
