#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$parentName,$folderName,$removalsDir)

$shell = New-Object -com Shell.Application

$mtpDevice = $shell.NameSpace(0x11).items() | where { $_.name -eq $mtpDeviceName }
if ($mtpDevice -eq $null) {
	Write-Output "Cannot find $mtpDeviceName"
	Exit 1
}

$arrayList = [System.Collections.ArrayList]@($storeName)
if (-not $parentName -eq "") {
	$pathComponents = @($parentName.Split('/'))
	$arrayList += $pathComponents
}
$parent = $mtpDevice
foreach ($component in $arrayList) {
	if ($component) {
		$parent = $parent.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($parent -eq $null -or -not $parent.IsFolder) {
	Write-Output "Cannot find parent folder $parentName"
	Exit 1
}
$parentFolder=$shell.Namespace($parent).self
$victimFolder=$parentFolder.GetFolder.Items() | where { $_.Name -eq $folderName }
if ($victimFolder -eq $null -or -not $victimFolder.IsFolder) {
	Write-Output "Cannot find victim folder $folderName"
	Exit 1
}

$destinationFolder = $shell.Namespace($removalsDir).self
$destinationFolder.GetFolder.MoveHere($victimFolder)
Start-Sleep -m 200
Remove-Item $removalsDir -Recurse -Confirm:$false -Force
Start-Sleep -m 200
$create = New-Item -itemtype directory -path $removalsDir
