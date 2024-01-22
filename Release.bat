@echo off

pushd "Scripts\"
call Win-GenerateProjects.bat
popd

IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
	set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
) ELSE (
	IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
		set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
	) ELSE (
		IF EXIST "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
			set msbuild="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
		) ELSE (
			echo Couldn't Find MSBuild VS2022, Exiting..
			goto end
		)
	)
)

%msbuild% --version
%msbuild% SVisualizer.sln /t:Build /p:Configuration=Release;Platform=x64

set shippingOutput=Binaries/windows-Release-x86_64/SVisualizer/
set elysiumContent=Elysium/Content/
set projContent=Content/

set destination=Output/

robocopy "%shippingOutput%" "%destination%/binaries/" /mir
robocopy "%elysiumContent%" "%destination%/binaries/Content/" /E
robocopy "%projContent%" "%destination%/binaries/Content/" /E

xcopy /y ".\Scripts\Shader Visualizer.bat" ".\Output"

PAUSE