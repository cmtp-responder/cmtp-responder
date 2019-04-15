#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName,$removalsDir)

$shell = New-Object -com Shell.Application

$mtpDevice = $shell.NameSpace(0x11).items() | where { $_.name -eq $mtpDeviceName }
if ($mtpDevice -eq $null) {
	Write-Output "Cannot find $mtpDeviceName"
	Exit 1
}

$pathComponents = @( $storeName.Split([system.io.path]::DirectorySeparatorChar) )
$storeRoot = $mtpDevice
foreach ($component in $pathComponents) {
	if ($component) {
		$storeRoot = $storeRoot.GetFolder.items() | where { $_.Name -eq $component }
	}
}
if ($storeRoot -eq $null -or -not $storeRoot.IsFolder) {
	Write-Output "Cannot find store $storeName"
	Exit 1
}

$storeRootFolder=$storeRoot.GetFolder

if ($storeRootFolder.Items) {
	$items = $storeRootFolder.Items()
	foreach ($i in $items) {
		$destinationFolder = $shell.Namespace($removalsDir).self
		$destinationFolder.GetFolder.MoveHere($i)
		Start-Sleep -m 200
		Remove-Item $removalsDir -Recurse -Confirm:$false -Force
		Start-Sleep -m 200
		$create = New-Item -itemtype directory -path $removalsDir
	}
}
