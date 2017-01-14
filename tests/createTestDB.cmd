set FB_PATH=C:\Program Files\Firebird\Firebird_2_5\bin\
set BUILD_DIR=%~dp0..\build\
set CREATE_SCRIPT=%~dp0..\tests\createTestDB.sql
set DB_DIR=%BUILD_DIR%test_db\
set TEST_DB=%DB_DIR%TEST.FDB
IF EXIST %DB_DIR% GOTO RECREATE_DB
cd %BUILD_DIR%
mkdir test_db
:RECREATE_DB
rem delete old db
cd %DB_DIR%
if NOT EXIST %TEST_DB% GOTO CREATE_DB
del %TEST_DB%
:CREATE_DB
"%FB_PATH%isql" -input %CREATE_SCRIPT%

IF EXIST %BUILD_DIR%Release\fbclient.dll GOTO FINISHED
copy "%FB_PATH%fbclient.dll" %BUILD_DIR%Release\ 
:FINISHED


