#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$parentName)

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
$findRoot = $mtpDevice
foreach ($component in $arrayList) {
	if ($component) {
		$findRoot = $findRoot.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($findRoot -eq $null -or -not $findRoot.IsFolder) {
	Write-Output "Cannot find parent folder $parentName"
	Exit 1
}

$findRootFolder=$findRoot.GetFolder

Function FindFiles($entry, $path) {
	if ($entry.Items) {
		$items = $entry.Items()
		foreach ($i in $items) {
			if ($path -eq "") {
				$nextPath = $i.Name
			} else {
				$nextPath = $path + '/' + $i.Name
			}
			if ($i.IsFolder) {
				$folder = $i.GetFolder()
				FindFiles $folder $nextPath
			} else {
				$nextPath
			}
		}
	}
}

FindFiles $findRootFolder ""
