@echo off
setlocal

call ./clean.bat

pushd "%~dp0"

rem Prefer calling make.bat if present, otherwise try an executable named make
if exist "%~dp0make.bat" (
	call "%~dp0make.bat" %*
) else if exist "%~dp0make" (
	"%~dp0make" %*
) else (
	echo No make script found in "%~dp0"
	popd
	endlocal
	exit /b 1
)

if not exist "SuperFX-CG.g3a" (
	echo Build did not produce SuperFX-CG.g3a
	popd
	endlocal
	exit /b 2
)

echo Copying "SuperFX-CG.g3a" to "E:\\SuperFX-CG.g3a"
copy /Y "SuperFX-CG.g3a" "E:\\SuperFX-CG.g3a" >nul
if errorlevel 1 (
	echo Failed to copy artifact to E:\\SuperFX-CG.g3a
	popd
	endlocal
	exit /b 3
)

echo Done.
popd
endlocal
exit /b 0