#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$parentName,$remoteFileName,$removalsDir)

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
$source = $mtpDevice
foreach ($component in $arrayList) {
	if ($component) {
		$source = $source.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($source -eq $null -or -not $source.IsFolder) {
	Write-Output "Cannot find parent folder $parentName"
	Exit 1
}
$sourceFolder=$shell.Namespace($source).self
$file = $sourceFolder.GetFolder.Items() | where { $_.Name -eq $remoteFileName }
if ($file -eq $null) {
	Write-Output "Cannot find victim file $remoteFileName"
	Exit 1
}

$removalsFolder = $shell.Namespace($removalsDir).self
$removalsFolder.GetFolder.MoveHere($file, 1564)
Start-Sleep -m 200
Remove-Item $removalsDir -Recurse -Confirm:$false -Force
Start-Sleep -m 200
$create = New-Item -itemtype directory -path $removalsDir
