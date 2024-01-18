@echo off

pushd ..

set shippingOutput=bin/windows-Release-x86_64/NormalsRotator/
set elysiumContent=Elysium/Content/
set projContent=Content/

set destination=Output/binaries/

call Robocopy "%shippingOutput%" "%destination%/" /E
call Robocopy "%elysiumContent%" "%destination%/Content/" /E
call Robocopy "%projContent%" "%destination%/Content/" /E

popd

PAUSE