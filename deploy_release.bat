@echo off
set usage_str=用法：deploy_release.bat 目标文件夹（必须为空） 版本号
set count=0
for %%i in (%*) do (
  set /a count+=1
)

if %count% LSS 2 (
    @echo %usage_str%
    pause
    exit /B
)

set winrar_exe="D:\00.Prog\WinRAR\winrar.exe"
set QT_DEPLOY_EXE=%Qt5152PathBin%\windeployqt.exe

set exe_file_base_name=X-exposer
set exe_folder=..\Desktop_Qt_5_15_2_MSVC2019_64bit-Release\release
set dest_folder=%1
set ver_str=%2

xcopy .\configs %dest_folder%\configs /Y /E /I
xcopy .\app_images %dest_folder%\app_images /Y /E /I
xcopy .\*.csv %dest_folder%\ /Y /I
xcopy .\*.docx %dest_folder%\ /Y /I
xcopy ".\PD list.xls" %dest_folder%\ /Y /I
copy %exe_folder%\%exe_file_base_name%.exe %dest_folder%\
%QT_DEPLOY_EXE% %dest_folder%\%exe_file_base_name%.exe
%winrar_exe% a -df -r -ep1 -tk %dest_folder%\%exe_file_base_name%-%ver_str%.zip %dest_folder%\*

@echo 完成！
pause
