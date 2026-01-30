Read through this first: https://docs.zephyrproject.org/latest/develop/getting_started/index.html

For MAC users;
Create a new virtual environment:
python3 -m venv ~/zephyrproject/.venv

Activate the virtual environment:
source ~/zephyrproject/.venv/bin/activate

For WINDOWS users;
Create a new virtual environment:
BatchfilePowerShell
cd %HOMEPATH%
python -m venv zephyrproject\.venv

Activate the virtual environment:
Python’s virtual environment activation in PowerShell requires running a script itself, which needs to be allowed.
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
BatchfilePowerShell
zephyrproject\.venv\Scripts\activate.bat
