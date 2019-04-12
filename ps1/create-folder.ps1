#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$parentName,$folderName)

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
$destination = $mtpDevice
foreach ($component in $arrayList) {
	if ($component) {
		$destination = $destination.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($destination -eq $null -or -not $destination.IsFolder) {
	Exit 1
}
$destinationFolder=$shell.Namespace($destination).self
$destinationFolder.GetFolder.NewFolder($folderName)

$shell.open($destinationFolder)
Start-Sleep -s 3
