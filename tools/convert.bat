setlocal enabledelayedexpansion

set argCount=0
for %%x in (%*) do (
    python image-to-3ds.py %%~x
)

