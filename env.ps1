param(
    [switch] $init,
    [switch] $upgrade
)

function Add-ToPath([string] $path)
{
	if (-not (Test-Path -path $path))
	{
		throw "ERROR_NONEXISTENT_PATH"
	}
	
	$env:Path = "$Path;" + $env:Path
}

try {
    $output = . choco
    Write-Host -ForegroundColor Green "Chocolatey is installed"
}
catch {
    Write-Host -ForegroundColor Orange "Chocolatey is not installed"
    # TODO: try to auto install chocolatey
    Write-Host -ForegroundColor Red "Installing Chocolatey is not supported. Please install Chocolatey and re-run this script"
    exit
}

$packages = @("git", "python", "ninja", "dtc-msys2", "gperf")

# Setup for first-time use
if ($init)
{
    . choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'
    . choco install git python ninja dtc-msys2 gperf nuget.commandline

    # install ARM GCC toolchain

    # run:
    # pip3 install --user -r scripts/requirements.txt
    # from zephyr project
}

nuget install autom8ed.com.compilers.arm-none-eabi-gcc -OutputDirectory "$PSScriptRoot\pkg"

$env:ZEPHYR_TOOLCHAIN_VARIANT="gccarmemb"
$env:GCCARMEMB_TOOLCHAIN_PATH="$PSScriptRoot\pkg\autom8ed.com.compilers.arm-none-eabi-gcc.6.3.1"

# Try to detect a J-Link installation
# Check Program Files (x86) first
$seggerPath = $null
if (Test-Path -Path "C:\Program Files (x86)" -PathType Container)
{
	$seggerPath = ((Get-ChildItem -Path "C:\Program Files (x86)") | Where-Object {$_.Name -eq "SEGGER"})
}

if ($seggerPath -eq $null)
{
	if (Test-Path -Path "C:\Program Files" -PathType Container)
	{
		$seggerPath = ((Get-ChildItem -Path "C:\Program Files") | Where-Object {$_.Name -eq "SEGGER"})
	}
}

if ($seggerPath -ne $null)
{
	$jlinkPath = $null

	# found a SEGGER install folder. See if there are any J-Link installs there
	if (($seggerPath -ne $null) -and ($seggerPath.Count -eq 1))
	{
		$jlinkVersions = ((Get-ChildItem -Path $seggerPath.FullName) | Where-Object {$_.Name.StartsWith("JLink_") })

		$recentVersion = $null
		foreach($version in $jlinkVersions)
		{
			if ($recentVersion -eq $null)
			{
				$recentVersion = $version
				continue
			}

			if ($recentVersion -lt $version)
			{
				$recentVersion = $version
			}
		}
		
		if ($recentVersion -ne $null)
		{
			$jlinkPath = $recentVersion.FullName
		}
	}
	elseif ($seggerPath.Count -gt 1)
	{
		Write-Error "Multiple SEGGER folders found in C:\Program Files (x86)"
	}

	if ($jlinkPath -ne $null)
	{
		Write-Host "Using SEGGER install from $jlinkPath"
		Add-ToPath -path $jlinkPath
	}
	else 
	{
		Write-Warning "Could not find JLink install under $seggerPath. Live debugging and MCU Flashing commands will be unavailable."
		Write-Warning "Install J-Link from SEGGER Website"
	}
}
else
{
	Write-Warning "Could not find SEGGER folder. Live debugging and MCU Flashing commands will be unavailable."
	Write-Warning "Install J-Link from SEGGER Website"
}

$env:ZEPHYR_BASE="C:\Users\skall\Documents\GitHub\zephyr"
Set-Alias -Name west -Value ". py -3 $env:ZEPHYR_BASE\scripts\west-win.py" -Scope 1


