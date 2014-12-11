setlocal enabledelayedexpansion

set argCount=0
for %%x in (%*) do (
    python %~p0\image-to-3ds.py %%~x
)

pause