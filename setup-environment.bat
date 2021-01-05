kecho "setup-environment.bat"
echo %userprofile%

cd %userprofile%\esp
git clone --recursive https://github.com/espressif/esp-adf.git
git checkout v2.2b

set IDF_PATH=%userprofile%\esp\esp-adf\esp-idf
set ADF_PATH=%userprofile%\esp\esp-adf

echo %IDF_PATH%
echo %ADF_PATH%

