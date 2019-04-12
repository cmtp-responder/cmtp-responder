#!/usr/bin/env bats
#
# SPDX-License-Identifier: Apache-2.0
#
# (c) Collabora Ltd. 2019
# Author: Andrzej Pietrasiewicz <andrzej.p@collabora.com>
#

declare -a os_cookie

#
# executed before each test case
#
setup()
{
	load config
	load mtp-device-id
	load os-setup
	load mtp-utilities
	load file-utilities

	os_setup os_cookie
}

#
# executed after each test case
#
teardown()
{
	rm -rf *.bin
	os_teardown os_cookie
}

store_empty()
{
	local FILES=`mtp_list_files os_cookie "$STORE_NAME" ""`
	[ x"$FILES" == "x" ] || { echo $FILES; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ x"$FOLDERS" == "x" ] || { echo $FOLDERS; return 1; }

	return 0
}

add_file_to_store()
{
	local PARENT="${1}"
	local NAME="${2}"
	random_file "$NAME"

	local ORIG_MD5=`md5sum "$NAME" | cut -f1 -d' '`

	run mtp_copy_to_store os_cookie "$STORE_NAME" "$PARENT" "$NAME"
	[ $status -eq 0 ] || { echo "Copying to store failed"; return 1; }

	run mtp_retrieve_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME" r_"$NAME"
	[ $status -eq 0 ] || { echo "Retrieving from store failed"; return 1; }

	local R_MD5=`md5sum r_"$NAME" | cut -f1 -d' '`
	[ "$ORIG_MD5" == "$R_MD5" ] || { echo "md5 mismatch"; return 1; }

	return 0
}

remove_file_from_store()
{
	local PARENT="${1}"
	local NAME="${2}"

	run mtp_remove_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME"

	[ $status -eq 0 ] || { echo "Removing from store failed"; return 1; }

	run mtp_retrieve_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME" r_"$NAME"
	[ $status -eq 1 ] || { echo "Unexpectedly retrieved file"; return 1; }

	return 0
}

add_single_folder_to_store_and_verify()
{
	local PARENT="${1#p:}"
	local NAME="${2#n:}"

	run mtp_create_folder os_cookie s:"$STORE_NAME" p:"$PARENT" n:"$NAME"
	[ $status -eq 0 ] || { echo "Creating a folder failed"; return 1; }

	local FILES=`mtp_list_files os_cookie "$STORE_NAME" ""`
	[ x"$FILES" == "x" ] || { echo "Unexpected files in store"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Prepare tests" {
	run mtp_prepare_tests os_cookie "$STORE_NAME"
	[ $status -eq 0 ]
}

@test "Verify empty store" {
	run store_empty
	[ $status -eq 0 ]
}

@test "Add a file to store" {
	run add_file_to_store / test.bin
	[ $status -eq 0 ]
}

@test "Remove a file from store" {
	run remove_file_from_store / test.bin
	[ $status -eq 0 ]

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Add and remove a file to/from store a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store / test.bin
		[ $status -eq 0 ]

		run remove_file_from_store / test.bin
		[ $status -eq 0 ]

		run store_empty
		[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }

		count=$(($count + 1))
	done
}

@test "Add multiple files to store" {
	for i in `seq $MULTIPLE_FILES`; do
		run add_file_to_store / test-$i.bin
		[ $status -eq 0 ]
	done
}

@test "Remove multiple files from store" {
	for i in `seq $MULTIPLE_FILES`; do
		run remove_file_from_store / test-$i.bin
		[ $status -eq 0 ]
	done

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Add and remove multiple files to/from store a number of times" {
	local count=0

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store / test-$i.bin
			[ $status -eq 0 ]
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store / test-$i.bin
			[ $status -eq 0 ]
		done

		run store_empty
		[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add a folder to store" {
	run add_single_folder_to_store_and_verify p:"" n:folder
	[ $status -eq 0 ] || { echo "Adding a folder failed"; return 1; }
}

@test "Remove a folder from store" {
	run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
	[ $status -eq 0 ]

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Add and remove a folder to/from store a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_single_folder_to_store_and_verify p:"" n:folder
		[ $status -eq 0 ] || { echo "Adding a folder failed"; return 1; }

		run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
		[ $status -eq 0 ]

		run store_empty
		[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add a folder to store and a file to folder" {
	run add_single_folder_to_store_and_verify p:"" n:folder
	[ $status -eq 0 ] || { echo "Adding a folder failed"; return 1; }

	run add_file_to_store folder test.bin
	[ $status -eq 0 ] || { echo "Adding a file to folder failed"; return 1; }
}

@test "Remove a file from a folder" {
	run remove_file_from_store folder test.bin
	[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Add and remove a file to folder a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store folder test.bin
		[ $status -eq 0 ] || { echo "Adding a file to folder failed"; return 1; }

		run remove_file_from_store folder test.bin
		[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add multiple files to folder" {
	for i in `seq $MULTIPLE_FILES`; do
		run add_file_to_store folder test-$i.bin
		[ $status -eq 0 ]
	done
}

@test "Remove multiple files from folder" {
	for i in `seq $MULTIPLE_FILES`; do
		run remove_file_from_store folder test-$i.bin
		[ $status -eq 0 ]
	done

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Add and remove multiple files to/from folder a number of times" {
	local count=0

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store folder test-$i.bin
			[ $status -eq 0 ] || { echo "Adding a file to folder failed"; return 1; }
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store folder test-$i.bin
			[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }
		done
		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
		count=$(($count + 1))
	done
}

@test "Remove a folder from store after transferring files to/from it" {
	run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
	[ $status -eq 0 ]

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Add a folder and a subfolder" {
	run add_single_folder_to_store_and_verify p:"" n:folder
	[ $status -eq 0 ] || { echo "Adding a folder failed"; return 1; }

	run mtp_create_folder os_cookie "$STORE_NAME" folder subfolder
	[ $status -eq 0 ] || { echo "Creating a folder failed"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Remove a folder from subfolder" {
	run mtp_remove_folder os_cookie s:"$STORE_NAME" p:folder n:subfolder
	[ $status -eq 0 ]

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Add and remove a folder to/from folder a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run mtp_create_folder os_cookie "$STORE_NAME" folder subfolder
		[ $status -eq 0 ] || { echo "Creating a folder failed"; return 1; }

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }

		run mtp_remove_folder os_cookie s:"$STORE_NAME" p:folder n:subfolder
		[ $status -eq 0 ]

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folder" ] || { echo "Unexpected folders in store"; return 1; }
		count=$(($count + 1))
	done
}

@test "Remove a folder from store after creating/removing subfolders" {
	run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
	[ $status -eq 0 ]

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Add a folder and a subfolder and a file to it" {
	run add_single_folder_to_store_and_verify p:"" n:folder
	[ $status -eq 0 ] || { echo "Adding a folder failed"; return 1; }

	run mtp_create_folder os_cookie "$STORE_NAME" folder subfolder
	[ $status -eq 0 ] || { echo "Creating a folder failed"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }

	run add_file_to_store folder/subfolder test.bin
	[ $status -eq 0 ] || { echo "Adding a file to subfolder failed"; return 1; }
}

@test "Remove a file from a subfolder" {
	run remove_file_from_store folder/subfolder test.bin
	[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Add and remove a file from a subfolder a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store folder/subfolder test.bin
		[ $status -eq 0 ] || { echo "Adding a file to subfolder failed"; return 1; }

		run remove_file_from_store folder/subfolder test.bin
		[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add multiple files to subfolder" {
	for i in `seq $MULTIPLE_FILES`; do
		run add_file_to_store folder/subfolder test-$i.bin
		[ $status -eq 0 ]
	done
}

@test "Remove multiple files from subfolder" {
	for i in `seq $MULTIPLE_FILES`; do
		run remove_file_from_store folder/subfolder test-$i.bin
		[ $status -eq 0 ]
	done

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }
}

@test "Add and remove multiple files to/from subfolder a number of times" {
	local count=0

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store folder/subfolder test-$i.bin
			[ $status -eq 0 ] || { echo "Adding a file to folder failed"; return 1; }
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store folder/subfolder test-$i.bin
			[ $status -eq 0 ] || { echo "Removing a file from folder failed"; return 1; }
		done
		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
		[ `echo $FOLDERS | tr -d '[:blank:]'` == "folderfolder/subfolder" ] || { echo "Unexpected folders in store"; return 1; }
		count=$(($count + 1))
	done
}

@test "Remove a folder and subfolder from store after transferring files " {
	run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
	[ $status -eq 0 ]

	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Verify empty store at end" {
	mkdir -p /tmp/"$STORE_NAME"
	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty"; return 1; }
}

@test "Finish test cases" {
	run mtp_finish_tests
	[ $status -eq 0 ]
}
