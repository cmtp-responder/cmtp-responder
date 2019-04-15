#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

param($mtpDeviceName,$storeName)

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

$shell.open($storeRoot)
Start-Sleep -s 3
