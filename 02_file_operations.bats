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
	[ $status -eq 0 ] || { echo "Copying to store failed: $output"; return 1; }

	run mtp_retrieve_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME" r_"$NAME"
	[ $status -eq 0 ] || { echo "Retrieving from store failed: $output"; return 1; }

	local R_MD5=`md5sum r_"$NAME" | cut -f1 -d' '`
	[ "$ORIG_MD5" == "$R_MD5" ] || { echo "md5 mismatch"; return 1; }

	return 0
}

remove_file_from_store()
{
	local PARENT="${1}"
	local NAME="${2}"

	run mtp_remove_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME"
	[ $status -eq 0 ] || { echo "Removing from store failed: $output"; return 1; }

	run mtp_retrieve_from_store os_cookie "$STORE_NAME" "$PARENT" "$NAME" r_"$NAME"
	[ $status -eq 1 ] || { echo "Unexpectedly retrieved file: $output"; return 1; }

	return 0
}

add_single_folder_to_store_and_verify()
{
	local PARENT="${1#p:}"
	local NAME="${2#n:}"

	run mtp_create_folder os_cookie s:"$STORE_NAME" p:"$PARENT" n:"$NAME"
	[ $status -eq 0 ] || { echo "Creating a folder failed: $output"; return 1; }

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	echo "$FOLDERS" | grep "$NAME"
	[ $? -eq 0 ] || { echo "Can't find folder $NAME in store"; return 1; }
}

@test "Prepare tests" {
	run mtp_prepare_tests os_cookie "$STORE_NAME"
	[ $status -eq 0 ] || { echo "Preparing the tests failed: $output"; return 1; }
}

@test "Verify empty store" {
	run store_empty
	[ $status -eq 0 ] || { echo "Verifying empty store failed: $output"; return 1; }
}

@test "Add and remove a file to/from store a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store / "filename with spaces.bin"
		[ $status -eq 0 ] || { echo "Adding file to store failed [$count iteration]: $output"; return 1; }

		run remove_file_from_store / "filename with spaces.bin"
		[ $status -eq 0 ] || { echo "Removing file from store failed [$count iteration]: $output"; return 1; }

		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "" | grep "filename with spaces.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly store not empty: $FILES"; return 1; }

		count=$(($count + 1))
	done
}

@test "Add and remove multiple files to/from store a number of times" {
	local count=0

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store / "filename with spaces-$i.bin"
			[ $status -eq 0 ] || { echo "Adding file #$count to store failed: $output"; return 1; }
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store / "filename with spaces-$i.bin"
			[ $status -eq 0 ] || { echo "Removing file #$count from store failed: $output"; return 1; }
		done

		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "" | grep "filename with spaces-[[:digit:]]*.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly some files still present: $FILES"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove a folder to/from store a number of times" {
	local count=0

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_single_folder_to_store_and_verify p:"" n:folder
		[ $status -eq 0 ] || { echo "Adding a folder failed [$count iteration]: $output"; return 1; }

		run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:folder
		[ $status -eq 0 ] || { echo "Removing a folder from store failed [$count iteration]: $output"; return 1; }

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" "" | grep "folder"`
		[ $status -eq 0 ] || { echo "Unexpectedly folder still present: $FOLDERS"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove a file to/from folder a number of times" {
	local count=0

	run add_single_folder_to_store_and_verify p:"" n:folder2
	[ $status -eq 0 ] || { echo "Adding a folder failed: $output"; return 1; }

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store folder2 "another one.bin"
		[ $status -eq 0 ] || { echo "Adding a file to folder failed [$count iteration]: $output"; return 1; }

		run remove_file_from_store folder2 "another one.bin"
		[ $status -eq 0 ] || { echo "Removing a file from folder failed [$count iteration]: $output"; return 1; }

		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "folder2" | grep "another one.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly file still present: $FILES"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove multiple files to/from folder a number of times" {
	local count=0

	run add_single_folder_to_store_and_verify p:"" n:folder3
	[ $status -eq 0 ] || { echo "Adding a folder failed: $output"; return 1; }

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store folder3 "multiple files-$i.bin"
			[ $status -eq 0 ] || { echo "Adding file #$count to folder failed: $output"; return 1; }
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store folder3 "multiple files-$i.bin"
			[ $status -eq 0 ] || { echo "Removing file #$count from folder failed: $output"; return 1; }
		done
		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "folder3" | grep "multiple files-[[:digit:]]*.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly some files still present: $FILES"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove a folder to/from folder a number of times" {
	local count=0

	run add_single_folder_to_store_and_verify p:"" n:folder4
	[ $status -eq 0 ] || { echo "Adding a folder failed: $output"; return 1; }

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_single_folder_to_store_and_verify p:folder4 n:subfolder
		[ $status -eq 0 ] || { echo "Creating subfolder failed [$count iteration]: $output"; return 1; }

		run mtp_remove_folder os_cookie s:"$STORE_NAME" p:folder4 n:subfolder
		[ $status -eq 0 ] || { echo "Removing subfolder from folder failed [$count iteration]: $output"; return 1; }

		local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" "folder4" | grep "subfolder"`
		[ $status -eq 0 ] || { echo "Unexpectedly folder still present: $FOLDERS"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove a file to/from a subfolder a number of times" {
	local count=0

	run add_single_folder_to_store_and_verify p:"" n:folder5
	[ $status -eq 0 ] || { echo "Adding a folder failed: $output"; return 1; }

	run add_single_folder_to_store_and_verify p:folder5 n:subdir
	[ $status -eq 0 ] || { echo "Adding a subfolder failed: $output"; return 1; }

	while [ $count -lt $ADD_REMOVE_COUNT ]; do
		run add_file_to_store folder5/subdir "for subdir.bin"
		[ $status -eq 0 ] || { echo "Adding a file to subfolder failed [$count iteration]: $output"; return 1; }

		run remove_file_from_store folder5/subdir "for subdir.bin"
		[ $status -eq 0 ] || { echo "Removing a file from folder failed [$count iteration]: $output"; return 1; }

		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "folder5/subdir" | grep "for subdir.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly file still present: $FILES"; return 1; }
		count=$(($count + 1))
	done
}

@test "Add and remove multiple files to/from subfolder a number of times" {
	local count=0

	run add_single_folder_to_store_and_verify p:"" n:folder6
	[ $status -eq 0 ] || { echo "Adding a folder failed: $output"; return 1; }

	run add_single_folder_to_store_and_verify p:folder6 n:child
	[ $status -eq 0 ] || { echo "Adding a subfolder failed: $output"; return 1; }

	while [ $count -lt $MULTIPLE_ADD_REMOVE_COUNT ]; do
		for i in `seq $MULTIPLE_FILES`; do
			run add_file_to_store folder6/child "for child-$i.bin"
			[ $status -eq 0 ] || { echo "Adding file #$count to subfolder failed: $output"; return 1; }
		done

		for i in `seq $MULTIPLE_FILES`; do
			run remove_file_from_store folder6/child "for child-$i.bin"
			[ $status -eq 0 ] || { echo "Removing file #$count from folder failed: $output"; return 1; }
		done
		local FILES=`mtp_list_files os_cookie "$STORE_NAME" "folder6/child" | grep "for child-[[:digit:]]*.bin"`
		[ -z "$FILES" ] || { echo "Unexpectedly some files still present: $FILES"; return 1; }
		count=$(($count + 1))
	done
}

@test "Remove all folders and files" {

	local FOLDERS=`mtp_list_folders os_cookie "$STORE_NAME" ""`
	local OLD_IFS="$IFS"
	IFS=$'\n'
	for i in $FOLDERS; do
		run mtp_remove_folder os_cookie s:"$STORE_NAME" p:"/" n:"$i"
		[ $status -eq 0 ] || { echo "Removing a folder failed: $output"; IFS="$OLD_IFS"; return 1; }
	done
	IFS="$OLD_IFS"

	local FILES=`mtp_list_files os_cookie "$STORE_NAME" ""`
	local OLD_IFS="$IFS"
	IFS=$'\n'
	for i in $FILES; do
		run remove_file_from_store / "$i"
		[ $status -eq 0 ] || { echo "Removing file from folder failed: $output"; IFS="$OLD_IFS"; return 1; }
	done
	IFS="$OLD_IFS"
}

@test "Verify empty store at end" {
	run store_empty
	[ $status -eq 0 ] || { echo "Unexpectedly store not empty: $output"; return 1; }
}

@test "Finish test cases" {
	run mtp_finish_tests os_cookie
	[ $status -eq 0 ] || { echo "Finishing test cases failed: $output"; return 1; }
}
