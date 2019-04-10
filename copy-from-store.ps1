#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$parentName,$remoteFileName,$localFileName,$tmpDir)

$shell = New-Object -com Shell.Application

$mtpDevice = $shell.NameSpace(0x11).items() | where { $_.name -eq $mtpDeviceName }
if ($mtpDevice -eq $null) {
	Exit 1
}

$arrayList = [System.Collections.ArrayList]@($storeName)
if (-not $parentName -eq "") {
	$pathComponents = @($parentName.Split('/'))
	$arrayList += $pathComponents
}
$source = $mtpDevice
foreach ($component in $arrayList) {
	if ($component) {
		$source = $source.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($source -eq $null -or -not $source.IsFolder) {
	Exit 1
}
$sourceFolder=$shell.Namespace($source).self
$file = $sourceFolder.GetFolder.Items() | where { $_.Name -eq $remoteFileName }
if ($file -eq $null) {
	Exit 1
}

$tmpFolder = $shell.Namespace($tmpDir).self
$tmpFilename = $tmpDir + '\' + $remoteFileName
if (Test-Path $tmpFilename) {
	Remove-Item -Confirm:$false $tmpFilename -Force | Out-Null
}
$tmpFolder.GetFolder.CopyHere($file, 1564)
$file = $tmpFolder.GetFolder.Items() | where { $_.Name -eq $remoteFileName }
if ($file -eq $null) {
	Exit 1
}

Rename-Item -path $tmpFilename -newname $localFileName
$file = $tmpFolder.GetFolder.Items() | where { $_.Name -eq $localFileName }
if ($file -eq $null) {
	Exit 1
}

if (Test-Path $localFileName) {
	Remove-Item -Confirm:$false $localFileName -Force | Out-Null
}
$destinationFolder = $shell.Namespace($(Get-Location).toString()).self
$destinationFolder.GetFolder.MoveHere($file, 1564)
