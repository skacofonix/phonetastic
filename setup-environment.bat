echo "get-started.bat"
echo %userprofile%

cd %userprofile%\esp
git clone --recursive https://github.com/espressif/esp-adf.git

set IDF_PATH=%userprofile%\esp\esp-adf\esp-idf
set ADF_PATH=%userprofile%\esp\esp-adf

echo %IDF_PATH%
echo %ADF_PATH%

%userprofile%\esp\esp-adf\esp-idf\export.bat